#pragma once

#include <string>
#include <vector>

namespace webbridge::impl {

// Returns JS code to initialize the WebBridge global registry 
// (idempotent, only once per window).
std::string generate_js_global_registry();

// Returns JS code for a class wrapper for a C++ type.
std::string generate_js_class_wrapper(
	std::string_view type_name,
	const std::vector<std::string>& sync_methods,
	const std::vector<std::string>& async_methods,
	const std::vector<std::string>& properties,
	const std::vector<std::string>& events);

// Returns JS code to publish a C++ object instance to JS.
std::string generate_js_published_object(
	std::string_view type_name,
	std::string_view var_name,
	std::string_view object_id,
	const std::vector<std::string>& sync_methods,
	const std::vector<std::string>& async_methods,
	const std::vector<std::string>& properties,
	const std::vector<std::string>& events);

} // namespace webbridge::impl