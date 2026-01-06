#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace webbridge::impl {

// Returns JS code for a class wrapper for a C++ type.
// Includes polling mechanism to wait for WebbridgeRuntime.
std::string generate_js_class_wrapper(
	std::string_view type_name,
	const std::vector<std::string>& sync_methods,
	const std::vector<std::string>& async_methods,
	const std::vector<std::string>& properties,
	const std::vector<std::string>& events,
	const std::vector<std::string>& instance_constants,
	const nlohmann::json& static_constants);

} // namespace webbridge::impl