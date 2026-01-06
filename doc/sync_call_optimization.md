# Sync-Call Optimierungen für WebBridge

## Ist-Zustand
- **1000 Sync-Calls: ~200ms**
- **Performance: ~0.2ms (200µs) pro Call**

Dies ist für ein Cross-Language-Binding akzeptabel, aber **optimierbar**.

## Identifizierte Bottlenecks

### 1. JSON-Parsing bei jedem Call
```cpp
// Aktuell: Jeder Call parst JSON neu
auto args = nlohmann::json::parse(req);           // ~50-100µs
auto object_id = args.at(0).get<std::string>();   // String-Allokation
auto op = args.at(1).get<std::string>();          // String-Allokation
auto member = args.at(2).get<std::string>();      // String-Allokation
```

**Problem:** 
- JSON-Parsing kostet ~50-100µs pro Call
- String-Allokationen für `object_id`, `op`, `member`
- Bei 1000 Calls = 50-100ms nur für Parsing!

### 2. String-Vergleiche in der Hot-Path
```cpp
if (op == "prop") {
    if (member == "aBool") { ... }
    else if (member == "strProp") { ... }
    // ... weitere if/else-Kaskade
}
```

**Problem:**
- String-Vergleiche sind teurer als Integer-Vergleiche
- Keine Jump-Table-Optimierung möglich

### 3. Object-Lookup bei jedem Call
```cpp
auto obj = get_object_or_throw<MyObject>(registry, object_id);
```

**Problem:**
- Map-Lookup (std::unordered_map) kostet ~20-50µs
- Bei wiederholten Calls auf dasselbe Objekt unnötig

## Optimierungsvorschläge

### ✅ **Empfehlung 1: Method-ID statt Strings** (Erwartete Verbesserung: **30-50%**)

**Konzept:** Verwende numerische IDs statt Strings für `op` und `member`.

#### Vor (aktuell):
```javascript
// JavaScript
window.__MyObject_sync(objectId, "call", "bar", ...args)
```

#### Nach (optimiert):
```javascript
// JavaScript - Verwende numerische IDs
const METHOD_IDS = {
    PROP_ABOOL: 1,
    PROP_STRPROP: 2,
    // ...
    CALL_BAR: 100,
    CALL_TESTVECTORS: 101,
    // ...
};

window.__MyObject_sync(objectId, METHOD_IDS.CALL_BAR, ...args)
```

#### C++ Implementation:
```cpp
// In type_registration.cpp: Generiere Enum
enum class MyObjectMemberId : uint16_t {
    PROP_ABOOL = 1,
    PROP_STRPROP = 2,
    PROP_COUNTER = 3,
    PROP_NUMBERS = 4,
    PROP_STATUS = 5,
    PROP_POD = 6,
    
    CONST_VERSION = 50,
    
    CALL_BAR = 100,
    CALL_TESTVECTORS = 101,
    CALL_THROWERROR = 102,
    CALL_MULTIPARAMTEST = 103,
};

inline void bind_MyObject_sync(webview::webview& w_ref, object_registry& registry)
{
    w_ref.bind("__MyObject_sync",
        [&registry](const std::string& req_id, const std::string& req, void* wPtr) {
            auto& w_ref = *static_cast<webview::webview*>(wPtr);
            auto args = nlohmann::json::parse(req);
            auto object_id = args.at(0).get<std::string>();
            auto member_id = static_cast<MyObjectMemberId>(args.at(1).get<uint16_t>());
            
            auto obj = get_object_or_throw<MyObject>(registry, object_id);

            // Switch statt if/else-Kaskade
            switch (member_id) {
                case MyObjectMemberId::PROP_ABOOL:
                    w_ref.resolve(req_id, 0, nlohmann::json(obj->aBool()).dump());
                    return;
                
                case MyObjectMemberId::CALL_BAR: {
                    auto [status, json] = invoke_and_serialize([&]() {
                        return obj->bar();
                    });
                    w_ref.resolve(req_id, status, json);
                    return;
                }
                
                // ... weitere Cases
                
                default:
                    w_ref.resolve(req_id, 1, R"({"error": "Unknown member ID"})");
            }
    }, &w_ref);
}
```

**Vorteile:**
- ✅ Switch-Statement mit Jump-Table (sehr schnell)
- ✅ Keine String-Allokationen/Vergleiche
- ✅ Kompilier-Zeit-Optimierung möglich

**Nachteil:**
- ⚠️ Zusätzlicher Codegen-Schritt für JS-Konstanten

---

### ✅ **Empfehlung 2: Object-Handle-Caching** (Erwartete Verbesserung: **10-20%**)

**Problem:** Bei wiederholten Calls auf dasselbe Objekt wird jedes Mal `registry.get()` ausgeführt.

#### Lösung: Objekt-Pointer in JavaScript speichern

```javascript
// JavaScript - Speichere C++-Pointer als Number
const obj = {
    __id: objectId,
    __ptr: cppPointer,  // Als opaque handle
    // ...
};

// Bei Calls: Sende Pointer statt String-ID
window.__MyObject_sync(__ptr, METHOD_IDS.CALL_BAR, ...args)
```

```cpp
// C++ - Verwende Pointer direkt
inline void bind_MyObject_sync(webview::webview& w_ref, object_registry& registry)
{
    w_ref.bind("__MyObject_sync",
        [&registry](const std::string& req_id, const std::string& req, void* wPtr) {
            auto& w_ref = *static_cast<webview::webview*>(wPtr);
            auto args = nlohmann::json::parse(req);
            
            // Interpretiere erste Argument als Pointer (vorsichtig!)
            auto obj_ptr = reinterpret_cast<MyObject*>(args.at(0).get<uintptr_t>());
            auto member_id = static_cast<MyObjectMemberId>(args.at(1).get<uint16_t>());
            
            // WICHTIG: Validierung dass Pointer noch gültig ist!
            if (!registry.is_valid(obj_ptr)) {
                w_ref.resolve(req_id, 1, R"({"error": "Invalid object pointer"})");
                return;
            }
            
            // Direkter Zugriff - keine Map-Lookup!
            switch (member_id) {
                case MyObjectMemberId::CALL_BAR: {
                    auto [status, json] = invoke_and_serialize([&]() {
                        return obj_ptr->bar();
                    });
                    w_ref.resolve(req_id, status, json);
                    return;
                }
                // ...
            }
    }, &w_ref);
}
```

**Vorteile:**
- ✅ Kein Map-Lookup bei jedem Call
- ✅ Direkter Pointer-Zugriff

**Nachteile:**
- ⚠️ Sicherheitsrisiko (Pointer-Validierung erforderlich)
- ⚠️ Komplexere Lifetime-Management

---

### ✅ **Empfehlung 3: Optimiertes JSON-Parsing** (Erwartete Verbesserung: **20-30%**)

**Aktuell:** nlohmann::json ist flexibel aber relativ langsam.

#### Option A: simdjson verwenden
```cpp
#include <simdjson.h>

// Bis zu 5x schneller als nlohmann::json
simdjson::dom::parser parser;
auto doc = parser.parse(req);
auto object_id = doc.at(0).get_string();
```

#### Option B: Custom Parser für bekanntes Format
```cpp
// Für Format: ["obj_123", 100, arg1, arg2, ...]
// Einfacher custom parser (unsicher, nur für Prototyping!)
struct FastArgs {
    std::string_view object_id;
    uint16_t member_id;
    const char* args_start;
};

FastArgs quick_parse(std::string_view req) {
    // Parsing-Logik für bekanntes Format
    // Viel schneller als generisches JSON
}
```

---

### ✅ **Empfehlung 4: Batch-Calls** (Erwartete Verbesserung: **50-80%** für Batch-Szenarien)

**Problem:** Jeder Call ist ein separater Roundtrip C++ ↔ JavaScript.

#### Lösung: Batch-API für mehrere Calls
```javascript
// JavaScript
const results = await window.__MyObject_sync_batch([
    [objId, METHOD_IDS.CALL_BAR],
    [objId, METHOD_IDS.PROP_ABOOL],
    [objId, METHOD_IDS.CALL_TESTVECTORS],
    // ... bis zu 100 Calls
]);
```

```cpp
// C++ - Verarbeite alle Calls in einer Schleife
inline void bind_MyObject_sync_batch(webview::webview& w_ref, object_registry& registry)
{
    w_ref.bind("__MyObject_sync_batch",
        [&registry](const std::string& req_id, const std::string& req, void* wPtr) {
            auto& w_ref = *static_cast<webview::webview*>(wPtr);
            auto batch = nlohmann::json::parse(req);
            
            nlohmann::json results = nlohmann::json::array();
            
            for (const auto& call : batch) {
                // Verarbeite jeden Call
                // ...
                results.push_back(result);
            }
            
            w_ref.resolve(req_id, 0, results.dump());
    }, &w_ref);
}
```

**Vorteile:**
- ✅ Reduziert Roundtrips massiv
- ✅ Bessere CPU-Cache-Nutzung
- ✅ Amortisiert Overhead über mehrere Calls

---

## Priorisierung

### Phase 1: Quick Wins (1-2 Tage)
1. ✅ **Method-ID statt Strings** → Template [registration.h.j2](../tools/templates/registration.h.j2) anpassen
2. ✅ **Switch statt if/else-Kaskade** → Automatisch durch Codegen

**Erwartete Verbesserung:** 30-50% (1000 Calls in ~100-140ms statt 200ms)

### Phase 2: Advanced (3-5 Tage)
3. ✅ **simdjson Integration**
4. ✅ **Object-Handle-Caching** (optional, mit Vorsicht)

**Erwartete Verbesserung:** 50-70% kumulativ (1000 Calls in ~60-100ms)

### Phase 3: Für spezielle Use-Cases
5. ✅ **Batch-API** für Bulk-Operations

**Erwartete Verbesserung:** 50-80% für Batch-Szenarien

---

## Benchmarking

Nach jeder Optimierung sollte der [TestObject-Benchmark](../frontend/src/App.svelte#L200) ausgeführt werden:

```javascript
async function runBenchmark() {
    const iterations = 1000;
    const testObj = await window.TestObject.create();
    
    for (let i = 0; i < iterations; i++) {
        await testObj.benchmarkSync(i);
    }
    
    // Erwartete Zeiten:
    // Aktuell:       ~200ms total (~200µs/call)
    // Nach Phase 1:  ~100-140ms (~100-140µs/call)
    // Nach Phase 2:  ~60-100ms (~60-100µs/call)
}
```

---

## Implementierung Phase 1

Die wichtigsten Änderungen für Phase 1:

### 1. Template anpassen: `tools/templates/registration.h.j2`

```jinja
// Generiere Member-ID Enum
enum class {{ cls.name }}MemberId : uint16_t {
{% for prop in cls.properties %}
    PROP_{{ prop.name | upper }} = {{ loop.index }},
{% endfor %}
{% for const in cls.constants if not const.is_static %}
    CONST_{{ const.name | upper }} = {{ 50 + loop.index }},
{% endfor %}
{% for method in cls.sync_methods %}
    CALL_{{ method.name | upper }} = {{ 100 + loop.index }},
{% endfor %}
};

inline void bind_{{ cls.name }}_sync(webview::webview& w_ref, object_registry& registry)
{
    w_ref.bind("__{{ cls.name }}_sync",
        [&registry](const std::string& req_id, const std::string& req, void* wPtr) {
            auto& w_ref = *static_cast<webview::webview*>(wPtr);
            auto args = nlohmann::json::parse(req);
            auto object_id = args.at(0).get<std::string>();
            auto member_id = static_cast<{{ cls.name }}MemberId>(args.at(1).get<uint16_t>());
            auto obj = get_object_or_throw<{{ cls.name }}>(registry, object_id);

            switch (member_id) {
{% for prop in cls.properties %}
                case {{ cls.name }}MemberId::PROP_{{ prop.name | upper }}:
                    w_ref.resolve(req_id, 0, nlohmann::json(obj->{{ prop.name }}()).dump());
                    return;
{% endfor %}
// ... weitere cases
            }
    }, &w_ref);
}
```

### 2. JavaScript Runtime anpassen: `src/webbridge/impl/type_registration.cpp`

```cpp
// Generiere Member-IDs als JavaScript-Konstanten
std::string generate_js_class_wrapper(...) {
    std::string js = std::format(R"(
(function() {{
    const MemberIds = {{
        // Properties
        {0}
        // Methods
        {1}
    }};
    
    // ...
    
    // Sync methods: Verwende Member-ID
    for (const m of syncMethods) {{
        obj[m] = (...a) => window[`__${{className}}_sync`](
            objectId, 
            MemberIds[`CALL_${{m.toUpperCase()}}`],  // Member-ID statt String!
            ...a
        );
    }}
}})();
)";
```

---

## Fazit

**200ms für 1000 Calls ist legitim**, aber mit den vorgeschlagenen Optimierungen kann man **50-70% Verbesserung** erreichen ohne fundamentale Architektur-Änderungen.

Die **Phase 1 Optimierungen** (Method-IDs + Switch-Statements) sind am einfachsten zu implementieren und bringen den größten Benefit.
