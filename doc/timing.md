# WebBridge Type Registration Performance Analysis

## Executive Summary

The `register_type<MyObject>()` function takes approximately **54 milliseconds** to complete. The performance bottleneck is **JavaScript initialization and evaluation**, which accounts for **39% of the total execution time**.

## Detailed Timing Breakdown

### Total Execution Time: 54 ms

| Component | Time (ms) | Percentage | Details |
|-----------|-----------|------------|---------|
| **JS Initialization** | 21 | 39% | Global registry + class wrapper generation and evaluation |
| **Property Getters Binding** | 11 | 20% | 6 properties × ~1.8ms each |
| **Sync Methods Binding** | 9 | 17% | 4 sync methods × ~2.3ms each |
| **Async Methods Binding** | 4 | 7% | 2 async methods × ~2ms each |
| **Object Destruction Binding** | 3 | 6% | `__webbridge_destroy` function binding |
| **Instance Constants** | 1 | 2% | 1 instance constant binding |
| **Object Creation Binding** | 0 | 0% | `__create_MyObject` function binding |
| **Static Constants** | 0 | 0% | 2 static constants definition |

### Test Configuration

- **Class**: MyObject
- **Properties**: 6 (aBool, strProp, counter, numbers, status, pod)
- **Events**: 1 (aEvent)
- **Sync Methods**: 4 (bar, testVectors, throwError, multiParamTest)
- **Async Methods**: 2 (foo, file)
- **Instance Constants**: 1 (version)
- **Static Constants**: 2 (appversion, CPP_VERSION)

## Root Cause Analysis

### Primary Bottleneck: JavaScript Evaluation (21ms / 39%)

The largest time consumer is the JavaScript code generation and evaluation through `w_ref.init()` calls:

1. **Global Registry Initialization**: Setup of `window.__webbridge_objects`, `__webbridge_notify`, and `__webbridge_emit` functions
2. **Class Wrapper Generation**: Extensive JavaScript class definition with:
   - Property accessors with Svelte store-like behavior
   - Event listener management
   - Instance and static constant handling
   - Method wrappers for both sync and async calls

Each JavaScript evaluation in the WebView incurs significant overhead due to:
- JavaScript engine initialization
- AST parsing
- JIT compilation
- Context switching between C++ and JavaScript

### Secondary Bottleneck: Binding Operations (24ms / 44%)

Individual `w_ref.bind()` calls for each property and method:

- **Property Getters** (11ms): Each property binding creates a separate callable interface to JavaScript
- **Sync Methods** (9ms): Method dispatch setup with argument serialization/deserialization
- **Async Methods** (4ms): Thread pool allocation and async execution context setup
- **Destruction Binding** (3ms): Cleanup handler registration

## Performance Characteristics

### Time per Binding

- **Property Getter**: ~1.8ms
- **Sync Method**: ~2.3ms  
- **Async Method**: ~2ms
- **Constant Binding**: ~0.5ms
- **Destruction Handler**: ~3ms

### Scaling Considerations

Linear complexity: Adding n properties adds ~1.8n milliseconds to total time.

Example projections:
- 10 properties: ~70ms total
- 20 properties: ~90ms total
- 50 properties: ~130ms total

## Recommendations for Optimization

### High Priority

1. **Batch JavaScript Initialization** (Potential savings: 5-10ms)
   - Combine multiple `w_ref.init()` calls into a single large JavaScript evaluation
   - Generate complete class definition as one block instead of incremental additions
   - Reduces context switching overhead

2. **Lazy Property/Method Binding** (Potential savings: 10-15ms)
   - Defer binding of less-frequently-used properties and methods
   - Implement on-demand binding when properties are first accessed from JavaScript
   - Suitable for large objects with many properties

### Medium Priority

3. **WebView Caching** (Potential savings: 5ms)
   - Cache compiled JavaScript class definitions
   - Reuse boilerplate code for similar types
   - Useful when registering multiple classes with similar structure

4. **Async Registration** (Potential savings: 0-10ms dependent on usage)
   - Make `register_type()` async to allow non-blocking UI
   - Requires adjustments to caller code
   - May reduce perceived latency

### Low Priority

5. **Constant Pooling** (Potential savings: 1-2ms)
   - Pre-compute static constant values
   - Reduce repetitive serialization

6. **Method Dispatch Optimization** (Potential savings: 2-3ms)
   - Use direct pointer-to-function calls where possible
   - Avoid JSON serialization for simple types

## Implementation Notes

### Instrumentation

Timing traces have been added to the generated registration code via the `registration.h.j2` template:

```cpp
[Timing] register_type JS initialization: 21 ms
[Timing] register_type object creation binding: 0 ms
[Timing] register_type object destruction binding: 3 ms
[Timing] bind_all property getters: 11 ms
[Timing] bind_all instance constants: 1 ms
[Timing] bind_all static constants: 0 ms
[Timing] bind_all sync methods: 9 ms
[Timing] bind_all async methods: 4 ms
[Timing] register_type total: 54 ms
```

Output is logged to `timing_trace.log` and console for debugging purposes.

### Measurement Environment

- **OS**: Windows 11
- **WebView**: Microsoft WebView2
- **Build**: Release configuration
- **Compilation**: Optimized for performance (-O2)

## Conclusion

The current 54ms startup time for type registration is acceptable for typical GUI applications. However, for scenarios with:

- **Multiple types registration** (>5 classes)
- **Large objects** (>20 properties/methods)
- **Performance-critical initialization paths**

Implementing optimization recommendations 1-2 could reduce total initialization time by 30-50%, achieving target times of 25-35ms per type.

The most impactful optimization would be **batching JavaScript initialization** (recommendation #1), which could deliver immediate 10-20% improvement with minimal code changes.

---

## References

- [WebBridge Type Registration Template](../tools/templates/registration.h.j2)
- [Binding Helpers Implementation](../src/webbridge/impl/binding_helpers.h)
- [Type Registration Implementation](../src/webbridge/impl/type_registration.cpp)
