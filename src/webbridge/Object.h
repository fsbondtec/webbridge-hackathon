#pragma once

#include "Impl/PropertyImpl.h"
#include "Impl/EventImpl.h"
#include <webview/webview.h>
#include <memory>

namespace webbridge {

/**
 * Base object for all web-exposed classes
 */
class Object
{
public:
	template<typename T>
	using Property = Impl::Property<T>;
	
	template<typename... Args>
	using Event = Impl::Event<Args...>;
};

// =========================================
// Type Registration API
// =========================================

/**
 * Registers a type as instantiable from JavaScript.
 * Must be specialized for each type (see generated _registration.h files).
 * 
 * @param w WebView instance
 */
template<typename T>
void registerType(webview::webview* w) {
	static_assert(sizeof(T) == 0, "registerType<T> must be specialized. Include the generated _registration.h file.");
}

/**
 * Publishes an existing C++ object under a name in JavaScript.
 * Must be specialized for each type (see generated _registration.h files).
 *
 * @param w WebView instance
 * @param name JavaScript variable name (e.g. "myGlobalObject")
 * @param obj Shared pointer to the object to publish
 */
template<typename T>
void publishObject(webview::webview* w, std::string_view name, std::shared_ptr<T> obj) {
	static_assert(sizeof(T) == 0, "publishObject<T> must be specialized. Include the generated _registration.h file.");
}

}
