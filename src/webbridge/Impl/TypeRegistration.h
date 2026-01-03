#pragma once

#include <string>
#include <vector>

namespace webbridge::Impl {

// Returns JS code to initialize the WebBridge global registry 
// (idempotent, only once per window).
std::string generateJsGlobalRegistry();

// Returns JS code for a class wrapper for a C++ type.
std::string generateJsClassWrapper(
	std::string_view typeName,
	const std::vector<std::string>& syncMethods,
	const std::vector<std::string>& asyncMethods,
	const std::vector<std::string>& properties,
	const std::vector<std::string>& events);

// Returns JS code to publish a C++ object instance to JS.
std::string generateJsPublishedObject(
	std::string_view typeName,
	std::string_view varName,
	std::string_view objectId,
	const std::vector<std::string>& syncMethods,
	const std::vector<std::string>& asyncMethods,
	const std::vector<std::string>& properties,
	const std::vector<std::string>& events);

} // namespace webbridge::Impl