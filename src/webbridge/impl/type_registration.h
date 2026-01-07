#pragma once

#include <string>
#include <vector>
#include <functional>
#include <nlohmann/json.hpp>
#include "webview/webview.h"
#include "dispatcher.h"

namespace webbridge::impl {

using obj_deleter_fun = std::function<void(const std::string&)>;

// Initialize webview with WebBridge runtime and dispatcher bindings.
// This creates only 4 bind() calls total instead of 3*N for N classes.
// Must be called once per webview instance.
void init_webview(webview::webview* ptr, obj_deleter_fun fun);

// Query if init_webview was called for this webview
bool is_webview_initialized(webview::webview* ptr);

// Returns JS code for a class wrapper for a C++ type.
// The JS code uses universal dispatcher functions instead of per-class bindings.
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