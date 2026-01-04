
# C++ to JavaScript Bridge [webbridge]

This repository is a demonstration project created during the fsbondtec [Christmas Hackathon 2025](https://www.fsbondtec.at/).

C++ objects are seamlessly integrated into modern web applications as an alternative to **Qt**. The solution is based on **webview** (C++ wrapper for Microsoft WebView2/Chromium) and a **Python code generator** that uses **tree-sitter** to analyze C++ classes and automatically generate JavaScript bindings and TypeScript type definitions. Code generation is required because C++26 reflection is not yet available.

## Getting Started

### Prerequisites

- **Visual Studio 2022** with C++ Desktop Development
- **Conan 2** (`pip install conan`)
- **CMake 3.26+**
- **Anaconda3** or Miniconda
- **Node.js** (for frontend build)
- **Microsoft Edge WebView2 Runtime** (usually preinstalled on Windows 10/11)

### Setup

**1. Create Conda environment**

The environment includes Python 3.12 and the required packages for the code generator (tree-sitter, jinja2):

```bash
conda env create -f environment.yml
# or, if the environment already exists:
conda env update --file environment.yml --name webbridge_hackathon --prune
```

**2. Configure and build the backend**

The `configure.bat` script will:
- Activate the Conda environment
- Install C++ dependencies via Conan (fmt, nlohmann_json, httplib, portable-file-dialogs, etc.)
- Generate CMake build configuration for all build types (Debug, Release, RelWithDebInfo, MinSizeRel)

```bash
configure.bat
```

Then open the Visual Studio solution:

```bash
# Open in Visual Studio:
start build\webbridge_hackathon.sln

# Or build directly:
cmake --build build --config Release
```

**3. Build the frontend**

The frontend uses Vite + Svelte 5 and TypeScript:

```bash
cd frontend
npm install
npm run build
```

The compiled assets are embedded into the C++ application via CMakeRC and served over HTTP by the `ResourceServer`.

**4. Run the application**

After a successful build:

```bash
# Debug build (with DevTools):
build\Debug\webbridge_hackathon.exe

# Release build (without DevTools):
build\Release\webbridge_hackathon.exe
```

## Concepts

Every class to be exposed to the web must inherit from `webbridge::object`. The API is inspired by Qt and provides the following mechanisms for JavaScript integration:

* **Methods** – Public C++ methods are automatically available in JavaScript
* **Properties** – Exposed as Svelte-compatible stores (read-only)
* **Events** – Trigger custom event listeners in JavaScript
* **Constants** - Readonly JS values
### Methods

All public methods of a `webbridge::object` class are automatically published to JavaScript.

A function marked with the `[[async]]` attribute is executed in a separate worker thread. This prevents blocking the main thread on the C++ side. On the JavaScript side, both synchronous and asynchronous methods always return a `Promise` and never block the main thread.

### Properties

Properties are similar to primitive data types but require access via the parenthesis operator `()` in C++. In JavaScript, properties are exposed as Svelte-compatible, reactive stores. They are read-only in JavaScript; changes to the property value in C++ are automatically and immediately propagated to JavaScript.

### Events

Events are the WebBridge equivalent of the Qt signal/slot mechanism.

### Constants

WebBridge supports exposing constants as both **static** (class-wide) and **non-static** (instance-specific). Both variants are automatically exported to JavaScript and are available there as read-only values.

### Error Handling

WebBridge implements robust error handling, distinguishing between JavaScript client errors (4xxx) and C++ server errors (5xxx). Errors are serialized as JSON objects and, for asynchronous operations, are propagated as rejected Promises.

**Error format:**
```json
{
  "error": {
    "code": 4001,
    "message": "Invalid argument type",
    "details": { "param": "value", "expected": "string" },
    "stack": "at function (file.js:10:5)",
    "origin": "javascript"
  }
}
```

**Error codes:**
- `4000-4999`: JavaScript errors (e.g., 4001 = JSON_PARSE_ERROR during parameter deserialization)
- `5000-5999`: C++ errors (e.g., 5000 = RUNTIME_ERROR during runtime errors)

Inspired by JSON-RPC 2.0, GraphQL, and HTTP status codes. Promises are automatically rejected on error, enabling clean exception handling with async/await syntax.

## Minimal Example

The following example shows how to define a C++ class with methods, properties, and events for web integration with WebBridge.

```cpp
#include "webbridge/Object.h"

class MyObject : public webbridge::object
{
public:
    property<bool> a_bool = false;
    property<std::string> str_prop;
    event<int, bool> a_event;
    inline static constexpr auto PI = 3.141592654;
    const std::string version = "1.0";

public:
    [[async]] void foo(std::string_view val) {
        // long-running action
        str_prop = val;
        a_event.emit(42, false);
    }

    bool bar() const {
        // Parenthesis operator accesses value
        return !a_bool();
    }
};
```

### Tracking JavaScript Properties

```js
const myObj = await MyObject.create();
// ...
myObj.a_bool.subscribe(value => {
    console.log('a_bool updated:', value);
});
```

### Calling a C++ Method from JavaScript

```js
const myObj = await MyObject.create();

// Example: call a synchronous method
// Blocks the main thread in C++, but JavaScript waits asynchronously
const result = await myObj.bar();
console.log('Result of bar():', result);

// Example: call an asynchronous method ([[async]] = worker thread in C++)
// Does not block the main thread in either C++ or JavaScript
myObj.foo('new value').then(() => {
    console.log('foo() completed');
});
```

### Handling Events in JavaScript

```js
const myObj = await MyObject.create();

// Register event listener (similar to Node.js EventEmitter)
myObj.a_event.on((intValue, boolValue) => {
    console.log('Event received:', intValue, boolValue);
});

// Alternatively, one-time event
myObj.a_event.once((intValue, boolValue) => {
    console.log('One-time event:', intValue, boolValue);
});
```

### Accessing Constants in JavaScript

```js
const myObj = await MyObject.create();
console.log(myObj.version); // Instance constant: "1.0"
console.log(MyObject.PI);   // Static constant: 3.141592654
```


## Registration and Publish

To make C++ classes available in JavaScript, they must be explicitly registered. The code generator creates the necessary binding files, which are then included in CMake. In your `main.cpp`, you must call the generated registration function:


```cpp
#include "MyObject_registration.h"

int main() {
    // Register the class for JavaScript
    webbridge::register_type<MyObject>();

    // Optionally publish an instance to JavaScript
    auto obj = std::make_shared<MyObject>();
    webbridge::publish_object<MyObject>(nullptr, "myObject", obj);

    // ... initialize and run your webview ...
}
```

## Known Limitations

The current implementation has the following limitations that are not yet supported:

- overloaded constructors and methods are not supported. 
- ~~**Enum Resolution in TypeScript**: C++ enums are not automatically converted to TypeScript enum types. They currently require manual type definitions in the frontend.~~

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

Third-party licenses can be found in [THIRD-PARTY-NOTICES.txt](THIRD-PARTY-NOTICES.txt).
