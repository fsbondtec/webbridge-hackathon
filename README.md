
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

Every class to be exposed to the web must inherit from `WebBridge::Object`. The API is inspired by Qt and provides the following mechanisms for JavaScript integration:

* **Methods** – Public C++ methods are automatically available in JavaScript
* **Properties** – Exposed as Svelte-compatible stores (read-only)
* **Events** – Trigger custom event listeners in JavaScript

### Methods

All public methods of a `WebBridge::Object` class are automatically published to JavaScript.

A function marked with the `[[async]]` attribute is executed in a separate worker thread. This prevents blocking the main thread on the C++ side. On the JavaScript side, both synchronous and asynchronous methods always return a `Promise` and never block the main thread.

### Properties

Properties are similar to primitive data types but require access via the parenthesis operator `()` in C++. In JavaScript, properties are exposed as Svelte-compatible, reactive stores. They are read-only in JavaScript; changes to the property value in C++ are automatically and immediately propagated to JavaScript.

### Events

Events are the WebBridge equivalent of the Qt signal/slot mechanism.

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
- `4000-4999`: JavaScript errors (e.g., 4001 = TypeError during parameter deserialization)
- `5000-5999`: C++ errors (e.g., 5000 = RuntimeError during runtime errors)

Inspired by JSON-RPC 2.0, GraphQL, and HTTP status codes. Promises are automatically rejected on error, enabling clean exception handling with async/await syntax.

## Minimal Example

The following example shows how to define a C++ class with methods, properties, and events for web integration with WebBridge.

```cpp
#include "WebBridge/Object.h"

class MyObject : public WebBridge::Object
{
    Property<bool> aBool = false;
    Property<std::string> strProp;
    Event<int, bool> aEvent;

public:
    [[async]] void foo(std::string_view val) {
        // long-running action
        strProp = val;
        aEvent.emit(42, false);
    }

    bool bar() const {
        // Parenthesis operator accesses value
        return !aBool();
    }
};
```

### Tracking JavaScript Properties

```js
const myObj = await MyObject.create();
// ...
myObj.aBool.subscribe(value => {
    console.log('aBool updated:', value);
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
myObj.aEvent.on((intValue, boolValue) => {
    console.log('Event received:', intValue, boolValue);
});

// Alternatively, one-time event
myObj.aEvent.once((intValue, boolValue) => {
    console.log('One-time event:', intValue, boolValue);
});
```

## Registration and Publish

To make C++ classes available in JavaScript, they must be explicitly registered. The code generator creates the necessary binding files, which are then included in CMake. In your `main.cpp`, you must call the generated registration function:


```cpp
#include "MyObject_registration.h"

int main() {
    // Register the class for JavaScript
    webbridge::registerType<MyObject>();

    // Optionally publish an instance to JavaScript
    auto obj = std::make_shared<MyObject>();
    webbridge::publishObject<MyObject>(nullptr, "myObject", obj);

    // ... initialize and run your webview ...
}
```

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

Third-party licenses can be found in [THIRD-PARTY-NOTICES.txt](THIRD-PARTY-NOTICES.txt).
