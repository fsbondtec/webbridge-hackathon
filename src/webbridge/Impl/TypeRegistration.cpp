#include "TypeRegistration.h"
#include <format>

namespace webbridge::Impl {

std::string generateJsGlobalRegistry()
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

std::string generateJsClassWrapper(
	std::string_view typeName,
	const std::vector<std::string>& syncMethods,
	const std::vector<std::string>& asyncMethods,
	const std::vector<std::string>& properties,
	const std::vector<std::string>& events)
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
)", typeName);


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
)", prop, typeName);
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

	for (const auto& method : syncMethods) {
		js += std::format(R"(
		async {0}(...args) {{
			return await __{1}_{0}(this.handle, ...args);
		}}
)", method, typeName);
	}

	for (const auto& method : asyncMethods) {
		js += std::format(R"(
		async {0}(...args) {{
			return await __{1}_{0}(this.handle, ...args);
		}}
)", method, typeName);
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
}})();
)", typeName);

	return js;
}

std::string generateJsPublishedObject(
	std::string_view typeName,
	std::string_view varName,
	std::string_view objectId,
	const std::vector<std::string>& syncMethods,
	const std::vector<std::string>& asyncMethods,
	const std::vector<std::string>& properties,
	const std::vector<std::string>& events)
{
	std::string js = std::format(R"(
(async function() {{
	'use strict';
	
	const obj = new {0}.__PublishedInstance();
	obj.__handle = "{1}";
	
	window.__webbridge_objects[obj.__handle] = obj;
)", typeName, objectId);


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
)", prop, typeName);
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

	for (const auto& method : syncMethods) {
		js += std::format(R"(
	obj.{0} = async function(...args) {{
		return await __{1}_{0}(this.__handle, ...args);
	}};
)", method, typeName);
	}

	for (const auto& method : asyncMethods) {
		js += std::format(R"(
	obj.{0} = async function(...args) {{
		return await __{1}_{0}(this.__handle, ...args);
	}};
)", method, typeName);
	}

	js += std::format(R"(
	window.{0} = obj;
	console.log('[WebBridge] Published "{0}" with handle:', obj.__handle);
}})();
)", varName);

	return js;
}

} // namespace webbridge::Impl
