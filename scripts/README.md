# Cute Framework Scripts

This directory contains utility scripts for maintaining the Cute Framework project.

## docs_parser.py / docs_parser.rb

Documentation parser that generates markdown documentation from header file comments.

### Features

- **Improved maintainability**: The parser is written in a high-level scripting language (Python/Ruby) making it easier to modify and extend compared to the C++ version
- **Clear separation of concerns**: Uses object-oriented design with dedicated classes for scanning, parsing, and document generation
- **Better error handling**: Provides clearer error messages and handles edge cases more gracefully
- **Modular design**: Each documentation type (enum, function, struct) has its own parsing method
- **Configurable**: Related links and categories are defined in easy-to-modify dictionaries/hashes

### Usage

```bash
# Python version (recommended)
python3 scripts/docs_parser.py

# Ruby version (requires Ruby installed)
ruby scripts/docs_parser.rb
```

### How It Works

1. **Scanning Phase**: Reads all `.h` files from the `include/` directory
2. **Parsing Phase**: Extracts documentation comments marked with `@enum`, `@function`, and `@struct`
3. **Link Generation**: Automatically creates cross-references between related documentation
4. **Output Phase**: Generates markdown files in the `docs/` directory organized by category
5. **Cleanup Phase**: Removes outdated documentation files that are no longer referenced

### Documentation Format

The parser recognizes special comment blocks in header files:

```cpp
/**
 * @function cf_example_function
 * @category example
 * @brief    Brief description of the function
 * @param    param_name    Description of parameter
 * @return   Description of return value
 * @remarks  Additional remarks about the function
 * @related  cf_related_function cf_another_function
 * @example > Example description
 *     // Example code here
 */
```

### Benefits Over C++ Version

1. **Easier to modify**: No compilation required, changes take effect immediately
2. **Better debugging**: Stack traces and error messages are more informative
3. **Cross-platform**: Works on any system with Python/Ruby installed
4. **Faster iteration**: Can quickly test changes without rebuilding
5. **More readable**: High-level language constructs make the logic clearer

### Requirements

- **Python version**: Python 3.6 or higher
- **Ruby version**: Ruby 2.5 or higher

### Output

The parser generates:
- Individual markdown files for each documented function, struct, and enum
- An `api_reference.md` file that indexes all documentation by category
- Automatic cross-linking between related documentation pages