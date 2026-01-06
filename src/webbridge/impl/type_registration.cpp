#include "type_registration.h"
#include <format>
#include <unordered_set>
#include <iostream>

namespace webbridge::impl {

// Track which webviews have been initialized
static std::unordered_set<webview::webview*> initialized_webviews;

// JavaScript runtime code - injected directly into webview
static constexpr const char* WEBBRIDGE_RUNTIME_JS = R"JS(
// WebBridge Runtime - Injected from C++

// Object registry: objectId -> object instance
const __webbridge_objects = {};

// =============================================================================
// C++ -> JS Handlers (called by C++ via webview eval)
// =============================================================================

window.__webbridge_notify = (objectId, propName, value) => {
    __webbridge_objects[objectId]?.[propName]?._notify?.(value);
};

window.__webbridge_emit = (objectId, eventName, ...args) => {
    __webbridge_objects[objectId]?.[eventName]?._dispatch?.(...args);
};

// =============================================================================
// Property: Svelte-compatible store
// =============================================================================

function __webbridge_createProperty(objectId, className, propName) {
    const subscribers = new Set();
    let currentValue;
    let loaded = false;

    return {
        subscribe(callback) {
            subscribers.add(callback);
            if (loaded) {
                callback(currentValue);
            } else {
                // Call consolidated sync binding: [objectId, "prop", propName]
                window[`__${className}_sync`](objectId, "prop", propName).then((v) => {
                    currentValue = v;
                    loaded = true;
                    callback(v);
                });
            }
            return () => subscribers.delete(callback);
        },
        async get() {
            if (!loaded) {
                // Call consolidated sync binding: [objectId, "prop", propName]
                currentValue = await window[`__${className}_sync`](objectId, "prop", propName);
                loaded = true;
            }
            return currentValue;
        },
        _notify(value) {
            currentValue = value;
            loaded = true;
            subscribers.forEach(fn => fn(value));
        }
    };
}

// =============================================================================
// Event: on/once pattern
// =============================================================================

function __webbridge_createEvent(objectId, className, eventName) {
    const listeners = [];

    return {
        on(callback) {
            listeners.push({ fn: callback, once: false });
            return () => {
                const idx = listeners.findIndex(l => l.fn === callback);
                if (idx !== -1) listeners.splice(idx, 1);
            };
        },
        once(callback) {
            listeners.push({ fn: callback, once: true });
        },
        _dispatch(...args) {
            for (let i = listeners.length - 1; i >= 0; i--) {
                listeners[i].fn(...args);
                if (listeners[i].once) listeners.splice(i, 1);
            }
        }
    };
}

// =============================================================================
// Class Factory
// =============================================================================

function __webbridge_createClass(config) {
    const { className, properties, events, syncMethods, asyncMethods, instanceConstants, staticConstants } = config;

    console.log(`[WebBridge] createClass: ${className}`);

    const factory = {
        async create(...args) {
            const objectId = await window[`__create_${className}`](...args);

            const obj = {
                __id: objectId,
                get handle() { return this.__id; },
                destroy() {
                    delete __webbridge_objects[objectId];
                    window.__webbridge_destroy?.(objectId);
                }
            };

            // Properties via consolidated sync binding
            for (const p of properties) obj[p] = __webbridge_createProperty(objectId, className, p);
            
            // Events (unchanged - C++ -> JS direction)
            for (const e of events) obj[e] = __webbridge_createEvent(objectId, className, e);
            
            // Sync methods via consolidated sync binding: [objectId, "call", methodName, ...args]
            for (const m of syncMethods) {
                obj[m] = (...a) => window[`__${className}_sync`](objectId, "call", m, ...a);
            }
            
            // Async methods via consolidated async binding: [objectId, methodName, ...args]
            for (const m of asyncMethods) {
                obj[m] = (...a) => window[`__${className}_async`](objectId, m, ...a);
            }
            
            // Instance constants via consolidated sync binding: [objectId, "const", constName]
            for (const c of instanceConstants) {
                obj[c] = await window[`__${className}_sync`](objectId, "const", c);
            }
            
            // Copy static constants to instance for convenience
            for (const [key, value] of Object.entries(staticConstants)) {
                obj[key] = value;
            }

            __webbridge_objects[objectId] = obj;
            return obj;
        }
    };

    // Assign static constants to factory (class level)
    for (const [key, value] of Object.entries(staticConstants)) {
        factory[key] = value;
    }

    // Assign factory to window
    window[className] = factory;
    console.log(`[WebBridge] Registered: ${className}`);
}

console.log('[WebBridge] Runtime loaded');
)JS";

void init_webview(webview::webview* ptr, obj_deleter_fun fun) {
	if (!ptr || initialized_webviews.count(ptr)) {
		return;
	}

	// Inject the WebBridge runtime
	ptr->init(WEBBRIDGE_RUNTIME_JS);

	// Bind the destroy handler
	ptr->bind("__webbridge_destroy", [fun](const std::string& req) -> std::string {
		auto args = nlohmann::json::parse(req);
		auto object_id = args.at(0).get<std::string>();
		fun(object_id);
		return "null";
	});

	initialized_webviews.insert(ptr);
}

bool is_webview_initialized(webview::webview* ptr) {
	return ptr && initialized_webviews.count(ptr) > 0;
}

std::string generate_js_class_wrapper(
	std::string_view type_name,
	const std::vector<std::string>& sync_methods,
	const std::vector<std::string>& async_methods,
	const std::vector<std::string>& properties,
	const std::vector<std::string>& events,
	const std::vector<std::string>& instance_constants,
	const nlohmann::json& static_constants)
{
	// Runtime is already injected via init_webview, no polling needed
	std::string js = std::format(R"(
(function() {{
	console.log('[Webbridge] Creating class: {0}');
	try {{
		__webbridge_createClass({{
			className: "{0}",
			properties: {1},
			events: {2},
			syncMethods: {3},
			asyncMethods: {4},
			instanceConstants: {5},
			staticConstants: {6}
		}});
		console.log('[Webbridge] Successfully created class: {0}');
	}} catch (error) {{
		console.error('[Webbridge] Error creating class {0}:', error);
		throw error;
	}}
}})();
)",
		type_name,
		nlohmann::json(properties).dump(),
		nlohmann::json(events).dump(),
		nlohmann::json(sync_methods).dump(),
		nlohmann::json(async_methods).dump(),
		nlohmann::json(instance_constants).dump(),
		static_constants.dump());

	return js;
}

} // namespace webbridge::impl
