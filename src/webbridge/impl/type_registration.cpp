#include "type_registration.h"
#include <format>

namespace webbridge::impl {

std::string generate_js_global_registry()
{
	return R"(
(function() {
	'use strict';

	// Nur einmal initialisieren
	if (window.__webbridge_initialized) {
		return;
	}

	window.__webbridge_objects = {};
	window.__webbridge_initialized = true;

	window.__webbridge_notify = function(objectId, propName, value) {
		const obj = window.__webbridge_objects[objectId];
		if (obj && obj[propName] && obj[propName]._notify) {
			obj[propName]._notify(value);
		} else {
			console.warn('[WebBridge] Object or property not found for notify:', objectId, propName);
		}
	};

	window.__webbridge_emit = function(objectId, eventName, ...args) {
		const obj = window.__webbridge_objects[objectId];
		if (obj && obj[eventName] && obj[eventName]._emit) {
			obj[eventName]._emit(...args);
		} else {
			console.warn('[WebBridge] Object or event not found for emit:', objectId, eventName);
		}
	};

	console.log('[WebBridge] Global registry initialized');
})();
)";
}

std::string generate_js_class_wrapper(
	std::string_view type_name,
	const std::vector<std::string>& sync_methods,
	const std::vector<std::string>& async_methods,
	const std::vector<std::string>& properties,
	const std::vector<std::string>& events,
	const std::vector<std::string>& instance_constants,
	const std::vector<std::string>& static_constants)
{
	std::string js = std::format(R"(
(function() {{
	'use strict';

	const _pointers = new FinalizationRegistry(handle => {{
		console.warn('[WebBridge] {0} wurde vom GC eingesammelt ohne destroy() - cleanup wird ausgeführt');
		delete window.__webbridge_objects[handle];
		__WebBridge_destroy(handle);
	}});

	class {0} {{
		#handle = null;
		#destroyed = false;

		constructor() {{}}

		static async create(...args) {{
			const obj = new {0}();
			obj.#handle = await __create_{0}(...args);
			// Objekt im globalen Registry registrieren für C++ → JS Kommunikation
			window.__webbridge_objects[obj.#handle] = obj;

			// Properties als Svelte-Stores initialisieren
)", type_name);


	for (const auto& prop : properties) {
		js += std::format(R"(
			obj.{0} = {{
				_subscribers: new Set(),
				subscribe(fn) {{
					this._subscribers.add(fn);
					__get_{1}_{0}(obj.#handle).then(fn);
					return () => this._subscribers.delete(fn);
				}},
				_notify(value) {{
					this._subscribers.forEach(fn => fn(value));
				}}
			}};
)", prop, type_name);
	}

	for (const auto& evt : events) {
		js += std::format(R"(
			obj.{0} = {{
				_listeners: [],
				on(fn) {{
					this._listeners.push({{ fn, once: false }});
					return () => this._listeners = this._listeners.filter(l => l.fn !== fn);
				}},
				once(fn) {{
					this._listeners.push({{ fn, once: true }});
				}},
				_emit(...args) {{
					this._listeners = this._listeners.filter(l => {{
						l.fn(...args);
						return !l.once;
					}});
				}}
			}};
)", evt);
	}

	// Fetch instance constants eagerly
	for (const auto& constant : instance_constants) {
		js += std::format(R"(
			obj.{0} = await __get_{1}_{0}(obj.#handle);
)", constant, type_name);
	}

	// Copy static constants to instance for convenience (obj.cppversion)
	for (const auto& constant : static_constants) {
		js += std::format(R"(
			obj.{0} = window.{1}.{0};
)", constant, type_name);
	}

	js += R"(
			// FinalizationRegistry als Safety-Net
			_pointers.register(obj, obj.#handle, obj);

			return obj;
		}

		get handle() {
			if (this.#destroyed) throw new Error('Object already destroyed');
			return this.#handle;
		}

		destroy() {
			if (this.#destroyed) return;
			this.#destroyed = true;
			// Aus Registry entfernen
			delete window.__webbridge_objects[this.#handle];
			_pointers.unregister(this);
			__WebBridge_destroy(this.#handle);
			this.#handle = null;
		}
)";

	for (const auto& method : sync_methods) {
		js += std::format(R"(
		async {0}(...args) {{
			return await __{1}_{0}(this.handle, ...args);
		}}
)", method, type_name);
	}

	for (const auto& method : async_methods) {
		js += std::format(R"(
		async {0}(...args) {{
			return await __{1}_{0}(this.handle, ...args);
		}}
)", method, type_name);
	}

	js += std::format(R"(
	}}

	// Inner class for published instances (created from C++)
	{0}.__PublishedInstance = class {{
		__handle = null;

		constructor() {{}}

		get handle() {{
			return this.__handle;
		}}
	}};

	window.{0} = {0};
)", type_name);


	js += R"(
})();
)";

	return js;
}

std::string generate_js_published_object(
	std::string_view type_name,
	std::string_view var_name,
	std::string_view object_id,
	const std::vector<std::string>& sync_methods,
	const std::vector<std::string>& async_methods,
	const std::vector<std::string>& properties,
	const std::vector<std::string>& events,
	const std::vector<std::string>& instance_constants,
	const std::vector<std::string>& static_constants)
{
	std::string js = std::format(R"(
(async function() {{
	'use strict';
	
	const obj = new {0}.__PublishedInstance();
	obj.__handle = "{1}";
	
	window.__webbridge_objects[obj.__handle] = obj;
)", type_name, object_id);


	for (const auto& prop : properties) {
		js += std::format(R"(
	obj.{0} = {{
		_subscribers: new Set(),
		subscribe(fn) {{
			this._subscribers.add(fn);
			__get_{1}_{0}(obj.__handle).then(fn);
			return () => this._subscribers.delete(fn);
		}},
		_notify(value) {{
			this._subscribers.forEach(fn => fn(value));
		}}
	}};
)", prop, type_name);
	}

	// Events
	for (const auto& evt : events) {
		js += std::format(R"(
	obj.{0} = {{
		_listeners: [],
		on(fn) {{
			this._listeners.push({{ fn, once: false }});
			return () => this._listeners = this._listeners.filter(l => l.fn !== fn);
		}},
		once(fn) {{
			this._listeners.push({{ fn, once: true }});
		}},
		_emit(...args) {{
			this._listeners = this._listeners.filter(l => {{
				l.fn(...args);
				return !l.once;
			}});
		}}
	}};
)", evt);
	}

	// Fetch instance constants
	for (const auto& constant : instance_constants) {
		js += std::format(R"(
	obj.{0} = await __get_{1}_{0}(obj.__handle);
)", constant, type_name);
	}

	// Copy static constants to instance for convenience
	for (const auto& constant : static_constants) {
		js += std::format(R"(
	obj.{0} = await __get_{1}_static_{0}(obj.__handle);
)", constant, type_name);
	}

	for (const auto& method : sync_methods) {
		js += std::format(R"(
	obj.{0} = async function(...args) {{
		return await __{1}_{0}(this.__handle, ...args);
	}};
)", method, type_name);
	}

	for (const auto& method : async_methods) {
		js += std::format(R"(
	obj.{0} = async function(...args) {{
		return await __{1}_{0}(this.__handle, ...args);
	}};
)", method, type_name);
	}

	js += std::format(R"(
	window.{0} = obj;
	console.log('[WebBridge] Published "{0}" with handle:', obj.__handle);
}})();
)", var_name);

	return js;
}

} // namespace webbridge::impl
