#include "type_registration.h"
#include "object_registry.h"
#include "thread_pool.h"
#include <format>
#include <unordered_set>
#include <iostream>

namespace webbridge::impl {

// Track which webviews have been initialized
static std::unordered_set<webview::webview*> initialized_webviews;

// JavaScript runtime code - injected directly into webview
// OPTIMIZED: Uses universal dispatcher functions instead of per-class bindings
static constexpr const char* WEBBRIDGE_RUNTIME_JS = R"JS(
// WebBridge Runtime - Injected from C++
// V8-Optimized: Monomorphic shapes, cached lookups, inline-friendly
// DISPATCHER VERSION: Uses 4 universal bindings instead of 3*N per class

// Object registry: objectId -> object instance
const __webbridge_objects = {};

// Class metadata registry: className -> config
const __webbridge_class_configs = {};

// =============================================================================
// Universal Dispatcher Functions (bound once, used by all classes)
// =============================================================================

// These are bound by C++ init_webview() - called for ALL classes
// window.__webbridge_create(className, ...args) -> objectId
// window.__webbridge_sync(className, objectId, op, member, ...args) -> result
// window.__webbridge_async(className, objectId, method, ...args) -> Promise
// window.__webbridge_destroy(objectId) -> void

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

class PropertyStore {
    constructor(objectId, className, propName) {
        this.objectId = objectId;
        this.className = className;
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
            // Use universal sync dispatcher
            window.__webbridge_sync(this.className, this.objectId, "prop", this.propName).then((v) => {
                this.currentValue = v;
                this.loaded = true;
                callback(v);
            });
        }
        const subscribers = this.subscribers;
        return () => { subscribers.delete(callback); };
    }

    async get() {
        if (!this.loaded) {
            this.currentValue = await window.__webbridge_sync(this.className, this.objectId, "prop", this.propName);
            this.loaded = true;
        }
        return this.currentValue;
    }

    _notify(value) {
        this.currentValue = value;
        this.loaded = true;
        for (const fn of this.subscribers) {
            fn(value);
        }
    }
}

function __webbridge_createProperty(objectId, className, propName) {
    return new PropertyStore(objectId, className, propName);
}

// =============================================================================
// Event: on/once pattern (V8-optimized)
// =============================================================================

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
// Class Factory (V8-optimized - uses universal dispatchers)
// =============================================================================

function __webbridge_createClass(config) {
    const { className, properties, events, syncMethods, asyncMethods, instanceConstants, staticConstants } = config;

    console.log(`[WebBridge] createClass: ${className}`);

    // Store config for later reference
    __webbridge_class_configs[className] = config;

    // Pre-compute counts
    const propCount = properties.length;
    const eventCount = events.length;
    const syncMethodCount = syncMethods.length;
    const asyncMethodCount = asyncMethods.length;
    const instanceConstCount = instanceConstants.length;
    const staticKeys = Object.keys(staticConstants);
    const staticCount = staticKeys.length;

    // Pre-build sync method wrappers using universal dispatcher
    const syncMethodWrappers = {};
    for (let i = 0; i < syncMethodCount; i++) {
        const methodName = syncMethods[i];
        syncMethodWrappers[methodName] = function(...args) {
            return window.__webbridge_sync(className, this.__id, "call", methodName, ...args);
        };
    }

    // Pre-build async method wrappers using universal dispatcher
    const asyncMethodWrappers = {};
    for (let i = 0; i < asyncMethodCount; i++) {
        const methodName = asyncMethods[i];
        asyncMethodWrappers[methodName] = function(...args) {
            return window.__webbridge_async(className, this.__id, methodName, ...args);
        };
    }

    const factory = {
        async create(...args) {
            // Use universal create dispatcher
            const objectId = await window.__webbridge_create(className, ...args);

            // Build property descriptors for all members at once
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
                        window.__webbridge_destroy(this.__id);
                    },
                    writable: false,
                    enumerable: true,
                    configurable: false
                }
            };

            // Add all properties
            for (let i = 0; i < propCount; i++) {
                const propName = properties[i];
                descriptors[propName] = {
                    value: __webbridge_createProperty(objectId, className, propName),
                    writable: false,
                    enumerable: true,
                    configurable: false
                };
            }
            
            // Add all events
            for (let i = 0; i < eventCount; i++) {
                descriptors[events[i]] = {
                    value: __webbridge_createEvent(),
                    writable: false,
                    enumerable: true,
                    configurable: false
                };
            }
            
            // Add all sync methods
            for (let i = 0; i < syncMethodCount; i++) {
                const methodName = syncMethods[i];
                descriptors[methodName] = {
                    value: syncMethodWrappers[methodName],
                    writable: false,
                    enumerable: true,
                    configurable: false
                };
            }
            
            // Add all async methods
            for (let i = 0; i < asyncMethodCount; i++) {
                const methodName = asyncMethods[i];
                descriptors[methodName] = {
                    value: asyncMethodWrappers[methodName],
                    writable: false,
                    enumerable: true,
                    configurable: false
                };
            }
            
            // Fetch all instance constants in parallel
            if (instanceConstCount > 0) {
                const constPromises = new Array(instanceConstCount);
                for (let i = 0; i < instanceConstCount; i++) {
                    constPromises[i] = window.__webbridge_sync(className, objectId, "const", instanceConstants[i]);
                }
                const constValues = await Promise.all(constPromises);
                for (let i = 0; i < instanceConstCount; i++) {
                    descriptors[instanceConstants[i]] = {
                        value: constValues[i],
                        writable: false,
                        enumerable: true,
                        configurable: false
                    };
                }
            }
            
            // Add all static constants
            for (let i = 0; i < staticCount; i++) {
                const key = staticKeys[i];
                descriptors[key] = {
                    value: staticConstants[key],
                    writable: false,
                    enumerable: true,
                    configurable: false
                };
            }

            const obj = Object.create(Object.prototype, descriptors);
            __webbridge_objects[objectId] = obj;
            return obj;
        }
    };

    // Assign static constants to factory
    for (let i = 0; i < staticCount; i++) {
        const key = staticKeys[i];
        factory[key] = staticConstants[key];
    }

    window[className] = factory;
}

console.log('[WebBridge] Runtime loaded (Dispatcher Version)');
)JS";

void init_webview(webview::webview* ptr, obj_deleter_fun fun) {
	if (!ptr || initialized_webviews.count(ptr)) {
		return;
	}

	auto& registry = object_registry::instance();
	auto& dispatcher = dispatcher_registry::instance();

	// Inject the WebBridge runtime
	ptr->init(WEBBRIDGE_RUNTIME_JS);

	// ==========================================================================
	// UNIVERSAL DISPATCHER BINDINGS (only 4 bind() calls total!)
	// ==========================================================================

	// 1. Universal CREATE dispatcher
	ptr->bind("__webbridge_create",
		[&registry, &dispatcher, ptr](const std::string& req_id, const std::string& req, void*) {
			try {
				auto args = nlohmann::json::parse(req);
				auto class_name = args.at(0).get<std::string>();
				
				// Remove className from args, pass rest to handler
				nlohmann::json create_args = nlohmann::json::array();
				for (size_t i = 1; i < args.size(); ++i) {
					create_args.push_back(args[i]);
				}
				
				const auto& handler = dispatcher.get_handler(class_name);
				auto object_id = handler.create(*ptr, registry, create_args);
				ptr->resolve(req_id, 0, nlohmann::json(object_id).dump());
			} catch (const std::exception& e) {
				ptr->resolve(req_id, 1, nlohmann::json{{"error", e.what()}}.dump());
			}
		}, nullptr);

	// 2. Universal SYNC dispatcher
	ptr->bind("__webbridge_sync",
		[&registry, &dispatcher, ptr](const std::string& req_id, const std::string& req, void*) {
            auto args = nlohmann::json::parse(req);
            auto class_name = args.at(0).get<std::string>();
            auto object_id = args.at(1).get<std::string>();
            auto operation = args.at(2).get<std::string>();
            auto member = args.at(3).get<std::string>();
            
            const auto& handler = dispatcher.get_handler(class_name);
            handler.sync(*ptr, registry, req_id, object_id, operation, member, args);
		}, nullptr);

	// 3. Universal ASYNC dispatcher (uses thread pool instead of std::thread)
	ptr->bind("__webbridge_async",
		[&registry, &dispatcher, ptr](const std::string& req_id, const std::string& req, void*) {
            auto args = nlohmann::json::parse(req);
            auto class_name = args.at(0).get<std::string>();
            auto object_id = args.at(1).get<std::string>();
            auto method = args.at(2).get<std::string>();
            
            const auto& handler = dispatcher.get_handler(class_name);
            
            // Submit to thread pool instead of creating new thread each time
            // This saves ~50-100µs per async call!
            get_thread_pool().submit([handler, ptr, &registry, req_id, object_id, method, args]() {
                handler.async(*ptr, registry, req_id, object_id, method, args);
            });
		}, nullptr);

	// 4. Universal DESTROY dispatcher
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
	// Runtime is already injected via init_webview
	// JS now uses universal __webbridge_* functions instead of per-class bindings
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
