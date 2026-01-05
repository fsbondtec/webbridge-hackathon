#pragma once

#include "impl/property_impl.h"
#include "impl/event_impl.h"
#include <webview/webview.h>
#include <memory>

namespace webbridge {

/**
 * Base object for all web-exposed classes
 */
class object
{
public:
	template<typename T>
	using property = impl::property<T>;
	
	template<typename... Args>
	using event = impl::event<Args...>;
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
void register_type(webview::webview* w) {
	static_assert(sizeof(T) == 0, "register_type<T> must be specialized. Include the generated _registration.h file.");
}

}
