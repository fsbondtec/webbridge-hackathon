#pragma once

#include <string>
#include <vector>

namespace webbridge::Impl {

std::string generateJsGlobalRegistry();

std::string generateJsClassWrapper(
	std::string_view typeName,
	const std::vector<std::string>& syncMethods,
	const std::vector<std::string>& asyncMethods,
	const std::vector<std::string>& properties,
	const std::vector<std::string>& events);

std::string generateJsPublishedObject(
	std::string_view typeName,
	std::string_view varName,
	std::string_view objectId,
	const std::vector<std::string>& syncMethods,
	const std::vector<std::string>& asyncMethods,
	const std::vector<std::string>& properties,
	const std::vector<std::string>& events);

} // namespace webbridge::Impl