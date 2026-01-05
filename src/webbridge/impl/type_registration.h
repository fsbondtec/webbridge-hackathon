#pragma once

#include <string>
#include <vector>
#include <utility>

namespace webbridge::impl {

// Static constant: name + JSON value
using static_constant = std::pair<std::string, std::string>;

// Returns JS code for a class wrapper for a C++ type.
// Includes polling mechanism to wait for WebbridgeRuntime.
std::string generate_js_class_wrapper(
	std::string_view type_name,
	const std::vector<std::string>& methods,
	const std::vector<std::string>& properties,
	const std::vector<std::string>& events,
	const std::vector<std::string>& instance_constants,
	const std::vector<static_constant>& static_constants);

} // namespace webbridge::impl