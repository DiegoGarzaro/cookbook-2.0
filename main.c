/*
Cookbook 2.0
Author: Diego Garzaro
Date: Dec 29, 2025
*/

// Libraries
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

// Constants
#define LEN_NAME            30              // Name length
#define LEN_REC             1000            // Receipt length
#define FILE_NAME           "receipts.txt"  // Receipts file name
#define LEN_PREFIX_NAME     6               // Length of "Name: "
#define LEN_PREFIX_RECEIPT  9               // Length of "Receipt: "
#define LEN_DATETIME_FORMAT 26              // Length of "YYYY-MM-DD HH:MM:SS"
#define LEN_INPUT_BUFFER    10              // Input buffer for menu choices
#define LEN_LOG_MSG         50              // Small log message buffer

// Enumerators
typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO = 1,
    LOG_WARN = 2,
    LOG_ERROR = 3
} LogLevel;

typedef enum {
    MENU_DISPLAY_ALL = 1,
    MENU_ADD = 2,
    MENU_VIEW = 3,
    MENU_UPDATE = 4,
    MENU_DELETE = 5,
} MenuChoice;

// Set minimum log level to display (logs below this level will be filtered out)
#define MIN_LOG_LEVEL LOG_INFO

// Struct
typedef struct Receipt {
    uint16_t id;
    char name[LEN_NAME];
    char receipt[LEN_REC];
    struct Receipt *next;
    struct Receipt *prev;
} Receipt;


// Function prototypes
void custom_log(LogLevel level, const char *message);
void trim_newline(char *str);
void free_list(Receipt *head);
void display_receipts(Receipt *head);
void view_receipt(Receipt *head, uint16_t receipt_id);
const char* log_level_to_string(LogLevel level);
uint16_t get_new_id(Receipt *head);
uint8_t parse_receipt_id(const char *input, uint16_t *receipt_id);
int8_t case_insensitive_compare(const char *s1, const char *s2);
// Core logic
Receipt *load_receipts(void);
Receipt *run_menu(Receipt *head);
Receipt *create_receipt(Receipt *head, const char *name, const char *receipt);
Receipt *insert_alphabetically(Receipt *head, Receipt *new_receipt);
Receipt *update_receipt(Receipt *head, uint16_t receipt_id, const char *name, const char *receipt);
Receipt *detach_receipt(Receipt *head, Receipt *node);
Receipt *delete_receipt(Receipt *head, uint16_t receipt_id);
// File I/O
uint8_t save_receipt_to_file(Receipt *r);
uint8_t rewrite_receipts_to_file(Receipt *head);

/**
 * @brief Main entry point of the Cookbook application
 *
 * Initializes the application by loading receipts from file, running the
 * interactive menu loop, and cleaning up resources before exit.
 *
 * @return int Exit status (0 for success)
 */
int main(){
    // Setup
    Receipt *head = load_receipts();
    printf("===== Diego's Cookbook =====\n");

    // Worker
    head = run_menu(head);

    // Cleanup
    free_list(head);
    return 0;
}


/**
 * @brief Runs the interactive menu loop for the cookbook application
 *
 * Displays a menu with options to display, add, view, update, or delete receipts.
 * Continues looping until the user chooses to exit (Q/q).
 *
 * @param head Pointer to the head of the receipt linked list
 * @return Receipt* Updated head pointer of the receipt list
 */
Receipt *run_menu(Receipt *head){
    char input[LEN_INPUT_BUFFER];
    uint8_t choice = 0;

    while(1){
        printf("\n--- MENU ---\n");
        printf("1. Display all\n");
        printf("2. Add receipt\n");
        printf("3. View receipt\n");
        printf("4. Update receipt\n");
        printf("5. Delete receipt\n");
        printf("Q. Exit\n");
        printf("Choice: ");

        if(!fgets(input, sizeof(input), stdin)) break;
        trim_newline(input);
        printf("\n");

        if(input[0] == 'q' || input[0] == 'Q') break;
        
        choice = atoi(input);

        if(choice == MENU_DISPLAY_ALL){
            custom_log(LOG_INFO, "Displaying all receipts...\n");
            display_receipts(head);
        }
        else if(choice == MENU_ADD){
            custom_log(LOG_INFO, "Adding a new receipt...\n\n");
            char name[LEN_NAME], receipt[LEN_REC];

            printf("Name: ");
            fgets(name, sizeof(name), stdin);
            trim_newline(name);

            printf("Receipt: ");
            fgets(receipt, sizeof(receipt), stdin);
            trim_newline(receipt);

            if(name[0] != '\0'){
                head = create_receipt(head, name, receipt);
                custom_log(LOG_INFO, "New receipt saved!\n");
            }
        }
        else if(choice == MENU_VIEW){
            // View receipt
            uint16_t receipt_id = 0;

            display_receipts(head);

            printf("ID of the receipt (int): ");
            if(fgets(input, sizeof(input), stdin) == NULL){
                continue; // handle EOF or read error
            }
            if(!parse_receipt_id(input, &receipt_id)){
                continue;
            }

            view_receipt(head, receipt_id);
        }
        else if(choice == MENU_UPDATE){
            custom_log(LOG_INFO, "Update receipt...\n");
            char name[LEN_NAME], receipt[LEN_REC];
            uint16_t receipt_id = 0;

            display_receipts(head);

            printf("ID of the receipt (int): ");
            if(fgets(input, sizeof(input), stdin) == NULL){
                continue; // handle EOF or read error
            }
            if(!parse_receipt_id(input, &receipt_id)){
                continue;
            }

            printf("Name (Press 'Enter' to keep current): ");
            fgets(name, sizeof(name), stdin);
            trim_newline(name);
            
            printf("Receipt (Press 'Enter' to keep current): ");
            fgets(receipt, sizeof(receipt), stdin);
            trim_newline(receipt);
            
            head = update_receipt(head, receipt_id, name, receipt);
            char msg[LEN_LOG_MSG];
            snprintf(msg, sizeof(msg), "Receipt '%d' is updated.\n", receipt_id);
            custom_log(LOG_INFO, msg);
        }
        else if(choice == MENU_DELETE){
            custom_log(LOG_INFO, "Delete receipt...\n");
            uint16_t receipt_id = 0;

            display_receipts(head);

            printf("ID of the receipt (int): ");
            if(fgets(input, sizeof(input), stdin) == NULL){
                continue; // handle EOF or read error
            }
            if(!parse_receipt_id(input, &receipt_id)){
                continue;
            }

            head = delete_receipt(head, receipt_id);
            char msg[LEN_LOG_MSG];
            snprintf(msg, sizeof(msg), "Receipt '%d' is deleted.\n", receipt_id);
            custom_log(LOG_INFO, msg);
        }
        else{
            printf("Invalid option.\n");
        }
    }
    printf("Saving and exiting... Goodbye!\n");
    // Return head receipt pointer to be properly freed.
    return head;
}

/**
 * @brief Converts a LogLevel enum value to its string representation
 *
 * @param level The log level to convert (LogLevel enum)
 * @return const char* String representation of the log level
 */
const char* log_level_to_string(LogLevel level){
    switch(level){
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO:  return "INFO";
        case LOG_WARN:  return "WARN";
        case LOG_ERROR: return "ERROR";
        default:        return "UNKNOWN";
    }
}

/**
 * @brief Logs a message with timestamp and log level
 *
 * Filters messages based on MIN_LOG_LEVEL. Formats output with current
 * timestamp in YYYY-MM-DD HH:MM:SS format followed by log level and message.
 *
 * @param level The severity level of the log message (LogLevel enum)
 * @param message The message string to log (const char*)
 */
void custom_log(LogLevel level, const char *message){
    // Filter logs based on minimum log level
    if(level < MIN_LOG_LEVEL){
        return;
    }

    time_t raw_time;
    struct tm *local_info;
    char time_buffer[LEN_DATETIME_FORMAT];

    time(&raw_time);
    // Convert raw time to the local timezone
    local_info = localtime(&raw_time);

    if(local_info == NULL){
        // Fallback if localtime fails
        printf("[%s] %s", log_level_to_string(level), message);
        return;
    }

    // Format the time for YYYY-MM-DD HH:MM:SS
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", local_info);

    // Print datetime + log message
    printf("%s - [%s] %s", time_buffer, log_level_to_string(level), message);
}

/**
 * @brief Removes trailing newline characters from a string
 *
 * Removes both '\r' and '\n' characters from the end of the string
 * by replacing the first occurrence with a null terminator.
 *
 * @param str The string to trim (char*)
 */
void trim_newline(char *str){
    str[strcspn(str, "\r\n")] = 0;
}

/**
 * @brief Parses a string input to extract a receipt ID
 *
 * Attempts to parse the input string as an unsigned 16-bit integer.
 * Logs a warning if parsing fails.
 *
 * @param input The input string to parse (const char*)
 * @param receipt_id Pointer to store the parsed ID (uint16_t*)
 * @return uint8_t 1 on success, 0 on failure
 */
uint8_t parse_receipt_id(const char *input, uint16_t *receipt_id){
    uint16_t temp;
    if(sscanf(input, "%hu", &temp) == 1){
        *receipt_id = temp;
        return 1;  // Success
    }
    custom_log(LOG_WARN, "Invalid input.\n");
    return 0;  // Failure
}

/**
 * @brief Loads all receipts from the storage file into memory
 *
 * Reads receipts from FILE_NAME, parses each name/receipt pair, and
 * builds a sorted doubly-linked list. Logs the number of receipts loaded.
 *
 * @return Receipt* Pointer to the head of the loaded receipt list, or NULL if file doesn't exist
 */
Receipt *load_receipts(){
    // Open receipts file
    FILE *fptr = fopen(FILE_NAME, "r");
    uint16_t num_rec = 0;

    // Check file existance
    if(fptr == NULL){
        custom_log(LOG_WARN, "File does not exist, or could not be opened.\n");
        return NULL;
    }
    
    // Declare variables
    Receipt *head = NULL;
    Receipt *tmp_node = NULL;
    char buff[LEN_REC];

    // Read receipts and store into a pointer
    while(fgets(buff, sizeof(buff), fptr)){
        // Clean buffer
        trim_newline(buff);

        // Check if the line starts a new Receipt
        if(strstr(buff, "Name:")){
            tmp_node = malloc(sizeof(Receipt));
            // Check if new_node is created properly
            if(!tmp_node){
                custom_log(LOG_ERROR, "Memory allocation failed during load.\n");
                break;
            }

            memset(tmp_node, 0, sizeof(Receipt));

            // Copy the name skipping "Name: " prefix
            strncpy(tmp_node->name, buff+LEN_PREFIX_NAME, LEN_NAME-1);
            tmp_node->name[LEN_NAME-1] = '\0';  // Ensure null-termination
        }
        // If currently filling a receipt
        else if(tmp_node != NULL && strstr(buff, "Receipt:")){
            // Copy the receipt skipping "Receipt: " prefix
            strncpy(tmp_node->receipt, buff+LEN_PREFIX_RECEIPT, LEN_REC-1);
            tmp_node->receipt[LEN_REC-1] = '\0';  // Ensure null-termination
            tmp_node->id = num_rec;
            head = insert_alphabetically(head, tmp_node);

            tmp_node = NULL;
            num_rec++;
        }
    }

    // Cleanup: free any partially read receipt
    if(tmp_node != NULL){
        free(tmp_node);
        custom_log(LOG_WARN, "Partial receipt data discarded.\n");
    }

    char log_msg[LEN_LOG_MSG];
    snprintf(log_msg, sizeof(log_msg), "%d receipt(s) loaded successfully!\n\n", num_rec);
    custom_log(LOG_INFO, log_msg);

    fclose(fptr);
    return head;
}

/**
 * @brief Generates a unique ID for a new receipt
 *
 * Uses a static counter to track the next available ID. On first call with
 * a non-empty list, initializes from the highest existing ID in the list.
 *
 * @param head Pointer to the head of the receipt list (Receipt*)
 * @return uint16_t A unique ID for a new receipt
 */
uint16_t get_new_id(Receipt *head){
    static uint16_t next_id = 0;

    // If this is the first call and list is not empty, initialize from the list
    if(next_id == 0 && head != NULL){
        Receipt *current = head;
        while(current != NULL){
            if(current->id >= next_id) next_id = current->id + 1;
            current = current->next;
        }
    }

    return next_id++;
}

/**
 * @brief Saves a single receipt to the file in append mode
 *
 * Appends the receipt's name and content to FILE_NAME. Creates the file
 * if it doesn't exist. Logs an error if the operation fails.
 *
 * @param r Pointer to the receipt to save (Receipt*)
 * @return uint8_t 1 on success, 0 on failure
 */
uint8_t save_receipt_to_file(Receipt *r){
    if(r == NULL){
        custom_log(LOG_ERROR, "Receipt is corrupted.\n");
        return 0; // Return 0: Fail
    }

    // 'a' to append. It creates the file if it doesn't exist
    FILE *fptr = fopen(FILE_NAME, "a");

    if(fptr == NULL){
        custom_log(LOG_ERROR, "Could not open file for writing.\n");
        return 0; // Return 0: Fail
    }

    fprintf(fptr, "Name: %s\n", r->name);
    fprintf(fptr, "Receipt: %s\n", r->receipt);

    fclose(fptr);
    return 1;   // Return 1: Success
}

/**
 * @brief Rewrites the entire receipt file with current list contents
 *
 * Opens FILE_NAME in write mode (truncating existing content) and writes
 * all receipts from the linked list. Used after delete or update operations.
 *
 * @param head Pointer to the head of the receipt list (Receipt*)
 * @return uint8_t 1 on success, 0 on failure
 */
uint8_t rewrite_receipts_to_file(Receipt *head){
    FILE *fptr = fopen(FILE_NAME, "w");

    if(!fptr){
        custom_log(LOG_ERROR, "Could not rewrite file.\n");
        return 0;
    }

    Receipt *current = head;
    while(current != NULL){
        fprintf(fptr, "Name: %s\n", current->name);
        fprintf(fptr, "Receipt: %s\n", current->receipt);
        current = current->next;
    }
    fclose(fptr);
    custom_log(LOG_INFO, "File updated.\n");
    return 1;
}

/**
 * @brief Creates a new receipt and adds it to the list
 *
 * Allocates memory for a new receipt, initializes it with the provided
 * name and content, assigns a unique ID, inserts it alphabetically into
 * the list, and saves it to file.
 *
 * @param head Pointer to the head of the receipt list (Receipt*)
 * @param name The name of the recipe (const char*)
 * @param receipt The recipe content/instructions (const char*)
 * @return Receipt* Updated head pointer of the receipt list
 */
Receipt *create_receipt(Receipt *head, const char *name, const char *receipt){
    // Allocate memory
    Receipt *new_receipt = malloc(sizeof(Receipt));
    
    // Check malloc
    if(new_receipt == NULL){
        char error_msg[LEN_LOG_MSG];
        snprintf(error_msg, sizeof(error_msg), "Failed create new receipt %s.", name);
        custom_log(LOG_ERROR, error_msg);
        return head;
    }

    // Initialize memory
    memset(new_receipt, 0, sizeof(Receipt));
    strncpy(new_receipt->name, name, LEN_NAME-1);
    new_receipt->name[LEN_NAME-1] = '\0';  // Ensure null-termination
    strncpy(new_receipt->receipt, receipt, LEN_REC-1);
    new_receipt->receipt[LEN_REC-1] = '\0';  // Ensure null-termination
    new_receipt->id = get_new_id(head);

    // Insert into the list
    head = insert_alphabetically(head, new_receipt);

    // File operation
    if(!save_receipt_to_file(new_receipt)){
        char error_msg[LEN_LOG_MSG];
        snprintf(error_msg, sizeof(error_msg), "Failed to save receipt %s to the file.", new_receipt->name);
        custom_log(LOG_ERROR, error_msg);
    }
    return head;
}

/**
 * @brief Performs case-insensitive string comparison
 *
 * Compares two strings character by character, ignoring case differences.
 * Returns the difference between the first mismatched characters.
 *
 * @param s1 First string to compare (const char*)
 * @param s2 Second string to compare (const char*)
 * @return int8_t Negative if s1 < s2, 0 if equal, positive if s1 > s2
 */
int8_t case_insensitive_compare(const char *s1, const char *s2){
    while(*s1 && *s2){
        if(tolower((unsigned char) *s1) != tolower((unsigned char) *s2)){
            return (tolower((unsigned char) *s1) - tolower((unsigned char) *s2));
        }
        s1++;
        s2++;
    }
    return tolower((unsigned char) *s1) - tolower((unsigned char) *s2);
}

/**
 * @brief Inserts a receipt into the list in alphabetical order by name
 *
 * Maintains a doubly-linked list sorted alphabetically (case-insensitive).
 * Handles insertion at head, middle, or tail positions.
 *
 * @param head Pointer to the head of the receipt list (Receipt*)
 * @param new_receipt Pointer to the receipt to insert (Receipt*)
 * @return Receipt* Updated head pointer of the receipt list
 */
Receipt *insert_alphabetically(Receipt *head, Receipt *new_receipt){
    // Case 1: Empty list
    if(head == NULL){
        new_receipt->next = NULL;
        new_receipt->prev = NULL;
        return new_receipt;
    }
    
    // Case 2: New node goes at first position (head)
    if(case_insensitive_compare(new_receipt->name, head->name) < 0){
        new_receipt->prev = NULL;
        new_receipt->next = head;
        head->prev = new_receipt;
        return new_receipt; // New node is now the head
    }

    // Case 3: Middle or end
    Receipt *current = head;
    while(current->next != NULL && case_insensitive_compare(current->next->name, new_receipt->name) < 0){
        current = current->next;
    }
    
    // Insert new_receipt after current and before the current->next
    new_receipt->prev = current;
    new_receipt->next = current->next;

    // Check if current->next exists
    if(current->next != NULL){
        current->next->prev = new_receipt;
    }
    current->next = new_receipt;

    return head;
}

/**
 * @brief Detaches a receipt node from the linked list
 *
 * Removes a node from the doubly-linked list by updating neighboring
 * nodes' pointers. Does not free the node's memory. Handles edge cases
 * for head, tail, and middle nodes.
 *
 * @param head Pointer to the head of the receipt list (Receipt*)
 * @param node Pointer to the node to detach (Receipt*)
 * @return Receipt* Updated head pointer of the receipt list
 */
Receipt *detach_receipt(Receipt *head, Receipt *node){
    if(head == NULL || node == NULL) return head;

    // 1. Deataching the head
    if(head == node){
        head = node->next;
    }

    // 2. Update NEXT node's 'prev' pointer
    if(node->next != NULL){
        node->next->prev = node->prev;
    }

    // 3. Update PREV node's 'next' pointer
    if(node->prev != NULL){
        node->prev->next = node->next;
    }

    // 4. Cleanup node links
    node->next = NULL;
    node->prev = NULL;

    return head;
}

/**
 * @brief Updates an existing receipt's name and/or content
 *
 * Searches for a receipt by ID and updates its fields. If the name changes,
 * the receipt is detached and re-inserted to maintain alphabetical order.
 * Rewrites the file to persist changes.
 *
 * @param head Pointer to the head of the receipt list (Receipt*)
 * @param receipt_id The ID of the receipt to update (uint16_t)
 * @param name New name for the recipe, or NULL/empty to keep current (const char*)
 * @param receipt New content, or NULL/empty to keep current (const char*)
 * @return Receipt* Updated head pointer of the receipt list
 */
Receipt *update_receipt(Receipt *head, uint16_t receipt_id, const char *name, const char *receipt){
    if(name == NULL && receipt == NULL){
        custom_log(LOG_INFO, "No changes were made.\n");
        return head;
    }

    char msg[LEN_LOG_MSG];
    snprintf(msg, sizeof(msg), "Searching for ID: %d...\n", receipt_id);
    custom_log(LOG_DEBUG, msg);

    Receipt *current = head;
    uint16_t found = 0, name_changed = 0;
    while(current != NULL){
        if(current->id == receipt_id){
            found = 1;
            // Update name
            if(name != NULL && name[0] != '\0'){
                if(strcmp(name, current->name) != 0){
                    strncpy(current->name, name, LEN_NAME-1);
                    current->name[LEN_NAME-1] = '\0';  // Ensure null-termination
                    name_changed = 1;
                }
            }

            // Update receipt
            if(receipt != NULL && receipt[0] != '\0'){
                strncpy(current->receipt, receipt, LEN_REC-1);
                current->receipt[LEN_REC-1] = '\0';  // Ensure null-termination
            }
            break;
        }
        current = current->next;
    }

    if(!found){
        custom_log(LOG_WARN, "Receipt ID not found.\n");
    }

    if(name_changed){
        head = detach_receipt(head, current);
        head = insert_alphabetically(head, current);
        custom_log(LOG_INFO, "Receipt updated and re-sorted.\n");
    }
    else{
        custom_log(LOG_INFO, "Receipt updated (order unchanged).\n");
    }
    rewrite_receipts_to_file(head);
    return head;
}

/**
 * @brief Deletes a receipt from the list by ID
 *
 * Searches for a receipt with the given ID, detaches it from the list,
 * frees its memory, and rewrites the file to persist the deletion.
 *
 * @param head Pointer to the head of the receipt list (Receipt*)
 * @param receipt_id The ID of the receipt to delete (uint16_t)
 * @return Receipt* Updated head pointer of the receipt list
 */
Receipt *delete_receipt(Receipt *head, uint16_t receipt_id){
    if(head == NULL){
        custom_log(LOG_WARN, "List is empty, nothing to delete.\n");
        return NULL;
    }

    Receipt *current = head;

    while(current != NULL){
        if(current->id == receipt_id) break;
        current = current->next;
    }

    if(current == NULL){
        char msg[LEN_LOG_MSG];
        snprintf(msg, sizeof(msg), "Receipt ID %d not found.\n", receipt_id);
        custom_log(LOG_WARN, msg);
        return head;
    }

    // Unlink from neighbors
    head = detach_receipt(head, current);

    // Update file
    rewrite_receipts_to_file(head);

    // Cleanup node
    free(current);

    return head;
}

/**
 * @brief Displays the full details of a specific receipt
 *
 * Searches for a receipt by ID and prints its name and content to stdout.
 * Logs an error if the receipt is not found.
 *
 * @param head Pointer to the head of the receipt list (Receipt*)
 * @param receipt_id The ID of the receipt to view (uint16_t)
 */
void view_receipt(Receipt *head, uint16_t receipt_id){
    if(head == NULL){
        custom_log(LOG_WARN, "Receipt list is empty, nothing to view.\n");
        return;
    }

    Receipt *current = head;
    while(current != NULL){
        if(current->id == receipt_id) break;
        current = current->next;
    }

    if(current == NULL){
        char msg[LEN_LOG_MSG];
        snprintf(msg, sizeof(msg), "Receipt ID '%d' not found.\n", receipt_id);
        custom_log(LOG_ERROR, msg);
        return;
    }

    printf("\n\t[%d] %s\n\n", current->id, current->name);
    printf("\t%s\n", current->receipt);
}

/**
 * @brief Displays a summary list of all receipts
 *
 * Prints a formatted list showing the ID and name of each receipt.
 * Displays a message if the list is empty.
 *
 * @param r Pointer to the head of the receipt list (Receipt*)
 */
void display_receipts(Receipt *r){
    if(r == NULL){
        custom_log(LOG_INFO, "The cookbook is empty!\n");
        return;
    }
    Receipt *current = r;
    printf("[ID] Receipt name\n");
    while(current != NULL){
        printf("- [%d] %s\n", current->id, current->name);
        current = current->next;
    }
}

/**
 * @brief Frees all memory allocated for the receipt linked list
 *
 * Traverses the entire linked list and frees each node's memory.
 * Should be called before program exit to prevent memory leaks.
 *
 * @param head Pointer to the head of the receipt list (Receipt*)
 */
void free_list(Receipt *head){
    Receipt *tmp;
    while(head != NULL){
        tmp = head;
        head = head->next;
        free(tmp);
    }
}
