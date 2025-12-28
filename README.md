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
- Maximum recipes: 65535 (uint16_t)

## Notes

- Recipes are automatically sorted alphabetically by name
- IDs are assigned automatically when recipes are created
- The file is rewritten on update/delete operations
