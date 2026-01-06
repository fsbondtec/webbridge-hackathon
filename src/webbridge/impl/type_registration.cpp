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
// V8-Optimized: Monomorphic shapes, cached lookups, inline-friendly

// Object registry: objectId -> object instance
const __webbridge_objects = {};

// Cache for sync/async function lookups (avoid template string construction)
const __webbridge_class_bindings = {};

// =============================================================================
// C++ -> JS Handlers (called by C++ via webview eval)
// =============================================================================

window.__webbridge_notify = (objectId, propName, value) => {
    const obj = __webbridge_objects[objectId];
    if (obj) {
        const prop = obj[propName];
        if (prop && prop._notify) {
            prop._notify(value);
        }
    }
};

window.__webbridge_emit = (objectId, eventName, ...args) => {
    const obj = __webbridge_objects[objectId];
    if (obj) {
        const evt = obj[eventName];
        if (evt && evt._dispatch) {
            evt._dispatch(...args);
        }
    }
};

// =============================================================================
// Property: Svelte-compatible store (V8-optimized)
// =============================================================================

// Pre-define shape for monomorphic property objects
class PropertyStore {
    constructor(objectId, syncFn, propName) {
        this.objectId = objectId;
        this.syncFn = syncFn;
        this.propName = propName;
        this.subscribers = new Set();
        this.currentValue = undefined;
        this.loaded = false;
    }

    subscribe(callback) {
        this.subscribers.add(callback);
        if (this.loaded) {
            callback(this.currentValue);
        } else {
            // Use cached sync function reference
            this.syncFn(this.objectId, "prop", this.propName).then((v) => {
                this.currentValue = v;
                this.loaded = true;
                callback(v);
            });
        }
        // Return unsubscribe function
        const subscribers = this.subscribers;
        return () => { subscribers.delete(callback); };
    }

    async get() {
        if (!this.loaded) {
            this.currentValue = await this.syncFn(this.objectId, "prop", this.propName);
            this.loaded = true;
        }
        return this.currentValue;
    }

    _notify(value) {
        this.currentValue = value;
        this.loaded = true;
        // Use for-of for better V8 optimization
        for (const fn of this.subscribers) {
            fn(value);
        }
    }
}

function __webbridge_createProperty(objectId, syncFn, propName) {
    return new PropertyStore(objectId, syncFn, propName);
}

// =============================================================================
// Event: on/once pattern (V8-optimized)
// =============================================================================

// Pre-define shape for listener objects (monomorphic)
class EventListener {
    constructor(fn, once) {
        this.fn = fn;
        this.once = once;
    }
}

class EventEmitter {
    constructor() {
        this.listeners = [];
    }

    on(callback) {
        const listener = new EventListener(callback, false);
        this.listeners.push(listener);
        // Return unsubscribe function
        const listeners = this.listeners;
        return () => {
            const idx = listeners.indexOf(listener);
            if (idx !== -1) {
                listeners.splice(idx, 1);
            }
        };
    }

    once(callback) {
        this.listeners.push(new EventListener(callback, true));
    }

    _dispatch(...args) {
        // Iterate backwards to safely remove one-time listeners
        const listeners = this.listeners;
        for (let i = listeners.length - 1; i >= 0; i--) {
            const listener = listeners[i];
            listener.fn(...args);
            if (listener.once) {
                listeners.splice(i, 1);
            }
        }
    }
}

function __webbridge_createEvent() {
    return new EventEmitter();
}

// =============================================================================
// Class Factory (V8-optimized)
// =============================================================================

function __webbridge_createClass(config) {
    const { className, properties, events, syncMethods, asyncMethods, instanceConstants, staticConstants } = config;

    console.log(`[WebBridge] createClass: ${className}`);

    // Cache binding function references (avoid template string lookups in hot path)
    const createFn = window['__create_' + className];
    const syncFn = window['__' + className + '_sync'];
    const asyncFn = window['__' + className + '_async'];
    const destroyFn = window.__webbridge_destroy;
    
    // Store in cache for property creation
    __webbridge_class_bindings[className] = { syncFn, asyncFn };

    // Pre-build method wrappers (monomorphic call sites)
    const syncMethodWrappers = {};
    for (let i = 0; i < syncMethods.length; i++) {
        const methodName = syncMethods[i];
        syncMethodWrappers[methodName] = function(...args) {
            return syncFn(this.__id, "call", methodName, ...args);
        };
    }

    const asyncMethodWrappers = {};
    for (let i = 0; i < asyncMethods.length; i++) {
        const methodName = asyncMethods[i];
        asyncMethodWrappers[methodName] = function(...args) {
            return asyncFn(this.__id, methodName, ...args);
        };
    }

    const factory = {
        async create(...args) {
            const objectId = await createFn(...args);

            // Build property descriptors for all members at once
            // V8 optimization: Define entire object shape in one operation
            const descriptors = {
                __id: {
                    value: objectId,
                    writable: false,
                    enumerable: false,
                    configurable: false
                },
                __className: {
                    value: className,
                    writable: false,
                    enumerable: false,
                    configurable: false
                },
                handle: {
                    get() { return this.__id; },
                    enumerable: false,
                    configurable: false
                },
                destroy: {
                    value: function() {
                        delete __webbridge_objects[this.__id];
                        if (destroyFn) destroyFn(this.__id);
                    },
                    writable: false,
                    enumerable: true,
                    configurable: false
                }
            };

            // Add all properties
            for (let i = 0; i < properties.length; i++) {
                const propName = properties[i];
                descriptors[propName] = {
                    value: __webbridge_createProperty(objectId, syncFn, propName),
                    writable: false,
                    enumerable: true,
                    configurable: false
                };
            }
            
            // Add all events
            for (let i = 0; i < events.length; i++) {
                descriptors[events[i]] = {
                    value: __webbridge_createEvent(),
                    writable: false,
                    enumerable: true,
                    configurable: false
                };
            }
            
            // Add all sync methods
            for (let i = 0; i < syncMethods.length; i++) {
                const methodName = syncMethods[i];
                descriptors[methodName] = {
                    value: syncMethodWrappers[methodName],
                    writable: false,
                    enumerable: true,
                    configurable: false
                };
            }
            
            // Add all async methods
            for (let i = 0; i < asyncMethods.length; i++) {
                const methodName = asyncMethods[i];
                descriptors[methodName] = {
                    value: asyncMethodWrappers[methodName],
                    writable: false,
                    enumerable: true,
                    configurable: false
                };
            }
            
            // Add all instance constants (await first, then add to descriptors)
            for (let i = 0; i < instanceConstants.length; i++) {
                const constName = instanceConstants[i];
                const constValue = await syncFn(objectId, "const", constName);
                descriptors[constName] = {
                    value: constValue,
                    writable: false,
                    enumerable: true,
                    configurable: false
                };
            }
            
            // Add all static constants
            const staticKeys = Object.keys(staticConstants);
            for (let i = 0; i < staticKeys.length; i++) {
                const key = staticKeys[i];
                descriptors[key] = {
                    value: staticConstants[key],
                    writable: false,
                    enumerable: true,
                    configurable: false
                };
            }

            // Create object with all properties defined at once
            // V8 can now optimize the entire shape immediately
            const obj = Object.create(Object.prototype, descriptors);

            __webbridge_objects[objectId] = obj;
            return obj;
        }
    };

    // Assign static constants to factory (class level)
    const staticKeys = Object.keys(staticConstants);
    for (let i = 0; i < staticKeys.length; i++) {
        const key = staticKeys[i];
        factory[key] = staticConstants[key];
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
