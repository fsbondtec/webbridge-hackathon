# webbridge Tools

Code generation tools for the webbridge framework.

## generate.py

Generates C++ registration headers (`_registration.h`) and/or TypeScript type definitions (`.types.d.ts`) from C++ header files that inherit from `webbridge::object`.

### Prerequisites

```bash
pip install tree-sitter tree-sitter-cpp jinja2
```

### Usage

```bash
# Generate C++ registration
python tools/generate.py src/MyObject.h --class-name MyObject --cpp_out=build/src

# Generate TypeScript types
python tools/generate.py src/MyObject.h --class-name MyObject --ts_out=frontend/src

# Generate both
python tools/generate.py src/MyObject.h --class-name MyObject --cpp_out=build/src --ts_out=frontend/src
```

**Important:** `--class-name` is required and must specify the exact class name.

### What is Detected

- **Properties**: `property<T>` fields
- **Events**: `event<Args...>` fields
- **Enums**: `enum` and `enum class` definitions
- **Async Methods**: Methods with `[[async]]` attribute
- **Sync Methods**: All other public methods

### CMake Integration

Using the `webbridge_generate()` function from `cmake/webbridge.cmake`:

#### Mode 1: AUTO Discovery (recommended)
Automatically finds all classes that inherit from `webbridge::object`:

```cmake
# Automatically process all .h/.hpp files in the target
webbridge_generate(
    TARGET your_target
    AUTO
)

# With TypeScript types
webbridge_generate(
    TARGET your_target
    AUTO
    LANGUAGE ts
    OUTPUT_DIR ${CMAKE_SOURCE_DIR}/frontend/src
)
```

#### Mode 2: FILES Discovery
Processes only specified files, discovers classes automatically:

```cmake
# Multiple files
webbridge_generate(
    TARGET your_target
    FILES MyObject.h OtherObject.h
)

# With custom output directory
webbridge_generate(
    TARGET your_target
    FILES MyObject.h
    LANGUAGE cpp
    OUTPUT_DIR ${CMAKE_BINARY_DIR}/generated
)
```

#### Mode 3: Explicit Class (for edge cases)
When automatic discovery doesn't work (e.g., with namespace aliasing):

```cmake
# Explicit specification of file and class name
webbridge_generate(
    TARGET your_target
    FILE SpecialObject.h
    CLASS_NAME SpecialObject
)
```

**Parameters:**
- `TARGET` (required): The CMake target
- `AUTO` / `FILES` / `FILE`: Exactly one must be specified
- `CLASS_NAME` (only with FILE): Explicit class name
- `LANGUAGE`: `cpp` (default) or `ts`
- `OUTPUT_DIR`: Output directory (default: `CMAKE_CURRENT_BINARY_DIR`)

### Example

Input (`MyObject.h`):
```cpp
class MyObject : public webbridge::object
{
public:
    property<bool> a_bool = false;
    property<std::string> str_prop;
    event<int, bool> a_event;

    [[async]] void foo(std::string_view val);
    bool bar() const;
};
```

Generates:
- C++ registration with factory, bindings, and property getters
- TypeScript interfaces with proper types and Svelte stores

Output TypeScript (`MyObject.types.d.ts`):
- TypeScript interface for the class
- Property types as Svelte stores
- Event types
- Method signatures with correct TypeScript types
- Native bindings: `__MyObject_destroy`
- Sync method binding: `__MyObject_bar`
- Async method binding: `__MyObject_foo`
- Property getters: `__get_MyObject_a_bool`, `__get_MyObject_str_prop`
- JavaScript class wrapper

## webbridge_parser.py

Parses C++ header files and extracts metadata from a specific class.

### Usage

```bash
# Parse class and output report
python tools/webbridge_parser.py src/MyObject.h --class-name MyObject

# Save report to file
python tools/webbridge_parser.py src/MyObject.h --class-name MyObject -o report.txt
```

Used internally by `generate.py`.

## webbridge_discoverer.py

Scans header files and finds all classes that inherit from `webbridge::object`.

### Usage

```bash
python tools/webbridge_discoverer.py src/*.h
```

**Output:** `filename|classname` per line (one line per discovered class)

Used internally by `webbridge_generate()` in AUTO and FILES modes.

## Future

With C++26 Reflection, these tools will become obsolete - registration will be generated at compile-time.
