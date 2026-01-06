#pragma once

#include <string>
#include <vector>
#include <functional>
#include <nlohmann/json.hpp>
#include "webview/webview.h"

namespace webbridge::impl {

using obj_deleter_fun = std::function<void(const std::string&)>;

// Initialize webview with WebBridge runtime (injects JS runtime code)
// deleter fun: method to delete objects from JS
void init_webview(webview::webview* ptr, obj_deleter_fun fun);

// query if init_webview was called
bool is_webview_initialized(webview::webview* ptr);


// Returns JS code for a class wrapper for a C++ type.
// Requires init_webview to be called first to inject the runtime.
std::string generate_js_class_wrapper(
	std::string_view type_name,
	const std::vector<std::string>& sync_methods,
	const std::vector<std::string>& async_methods,
	const std::vector<std::string>& properties,
	const std::vector<std::string>& events,
	const std::vector<std::string>& instance_constants,
	const nlohmann::json& static_constants);

} // namespace webbridge::impl