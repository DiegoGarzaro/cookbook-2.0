# Cookbook

Simple command-line recipe manager written in C. Stores recipes in a text file and keeps them sorted alphabetically.

## Build

```bash
gcc main.c -o cookbook
```

## Usage

```bash
./cookbook
```

The program loads recipes from `receipts.txt` and presents a menu with options to:
- Display all recipes
- Add new recipes
- View a specific recipe
- Update existing recipes
- Delete recipes

## File Format

Recipes are stored in `receipts.txt` with this format:

```
Name: Recipe Name
Receipt: Recipe instructions here
```

## Configuration

You can adjust the logging level in `main.c`:

```c
#define MIN_LOG_LEVEL LOG_INFO
```

Available levels:
- `LOG_DEBUG` - Show all messages
- `LOG_INFO` - Default level
- `LOG_WARN` - Warnings and errors only
- `LOG_ERROR` - Errors only

## Limits

- Recipe names: 30 characters
- Recipe text: 1000 characters
- Maximum recipes: 65535 (`uint16_t`).

> _**Note:** If more receipts are needed, update all occurences of **receipt ID** data type from `uint16_t` to `uint32_t`, so the new limit would be **4.2 billion receipts**._

## Optimization Strategies

The objective of this project is to develop Cookbook design with an emphasis on applying optimization techniques to improve performance and efficiency:

### Integer ID Matching
- Recipe structures use integer IDs (`uint16_t`) for matching and lookup operations
- Integer comparison consumes less CPU power and is significantly faster than string comparison
- While search operations are still O(n) due to the linked list structure, integer comparison is approximately 5-10x faster per comparison than string matching

**Implementation** (`main.c:45-46`):
```c
typedef struct Receipt {
    uint16_t id;  // Integer ID for fast comparison
    // ...
}
```

**Usage examples:**
- `main.c:493` - `if(current->id == receipt_id)` in `update_receipt()`
- `main.c:539` - `if(current->id == receipt_id)` in `delete_receipt()`
- `main.c:570` - `if(current->id == receipt_id)` in `view_receipt()`

Integer comparison (`==`) is a single CPU instruction, much faster than `strcmp()` which iterates through character arrays.

### Efficient ID Generation
- IDs are managed using a static integer variable within the ID generation function
- This approach maintains the last assigned ID across function calls
- **Avoids O(n) iteration** through all recipes to determine the next available ID
- Each new recipe gets an ID in constant time O(1), regardless of the total number of recipes

**Implementation** (`main.c:322-335`):
```c
uint16_t get_new_id(Receipt *head){
    static uint16_t next_id = 0;  // Persists across function calls

    // On first call, initialize from existing list
    if(next_id == 0 && head != NULL){
        Receipt *current = head;
        while(current != NULL){
            if(current->id >= next_id) next_id = current->id + 1;
            current = current->next;
        }
    }

    return next_id++;  // O(1) constant time
}
```

**Usage** (`main.c:395`):
```c
new_receipt->id = get_new_id(head);  // Constant time O(1)
```

The static variable scans the list **once** on initialization, then generates IDs in **O(1)** time by simple increment.

### Minimal File Storage
- IDs are not stored in `receipts.txt` as they are not essential for persistence
- IDs are regenerated when loading recipes from file, keeping the file format simple and reducing storage overhead
- This design separates runtime optimization (IDs) from persistent data (recipe names and instructions)

**Saving to file** (`main.c:351-352`):
```c
// IDs are NOT written to file
fprintf(fptr, "Name: %s\n", r->name);
fprintf(fptr, "Receipt: %s\n", r->receipt);  // Only name and recipe content
```

**Regeneration during load** (`main.c:300`):
```c
// IDs are regenerated sequentially when loading from file
tmp_node->id = num_rec;
```

File format only stores essential data (name and content). IDs are runtime-only constructs regenerated on load, keeping the persistent storage simple and minimal.

### Memory and Processing Efficiency
- Chained receipt structures use minimal memory footprint
- File operations are optimized to minimize I/O overhead
- Alphabetical sorting is maintained to enable future binary search optimizations

**Doubly linked list structure** (`main.c:45-51`):
```c
typedef struct Receipt {
    uint16_t id;           // 2 bytes (vs 4 for uint32_t)
    char name[LEN_NAME];   // 30 bytes
    char receipt[LEN_REC]; // 1000 bytes
    struct Receipt *next;  // Next node pointer
    struct Receipt *prev;  // Previous node pointer
} Receipt;
```
- Uses `uint16_t` (2 bytes) instead of `uint32_t` (4 bytes), saving 2 bytes per recipe
- Dynamic allocation prevents memory waste
- Doubly linked list enables efficient insertion/deletion without array reallocation

**File I/O optimization**:
```c
// Append mode for new recipes (main.c:344)
FILE *fptr = fopen(FILE_NAME, "a");

// Full rewrite only for updates/deletes (main.c:358-375)
uint8_t rewrite_receipts_to_file(Receipt *head){
    // Only called when necessary to minimize I/O overhead
}
```

**Alphabetical sorting** (`main.c:409-453`):
```c
// Case-insensitive comparison for sorting
int8_t case_insensitive_compare(const char *s1, const char *s2){
    while(*s1 && *s2){
        if(tolower((unsigned char) *s1) != tolower((unsigned char) *s2)){
            return (tolower((unsigned char) *s1) - tolower((unsigned char) *s2));
        }
        s1++; s2++;
    }
    return tolower((unsigned char) *s1) - tolower((unsigned char) *s2);
}

// Maintains sorted order on insertion
Receipt *insert_alphabetically(Receipt *head, Receipt *new_receipt);
```

**Current Limitation:** The doubly linked list structure limits search and insertion operations to **O(n)** time complexity. Even though recipes are sorted, binary search cannot be performed on linked lists because they lack random access - we must traverse nodes sequentially to reach any position.

**To achieve O(log n) or O(1) performance**, the code would need to be redesigned using different data structures:
- **Array-based storage** → Enables binary search O(log n) for lookups
- **Hash table** → Provides O(1) average-case lookups by ID
- **Binary Search Tree** → Offers O(log n) search/insert (requires balancing for guaranteed performance)

For a personal cookbook with hundreds or even thousands of recipes, the current O(n) performance is acceptable - modern CPUs handle these operations in microseconds. The sorted order primarily provides better user experience with organized display.

## Notes

- Recipes are automatically sorted alphabetically by name
- IDs are assigned automatically when recipes are created
- The file is rewritten on update/delete operations
