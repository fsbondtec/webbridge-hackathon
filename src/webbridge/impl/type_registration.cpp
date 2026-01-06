#include "type_registration.h"
#include <format>
#include <unordered_set>
#include <chrono>
#include <thread>
#include <iostream>


using namespace std::chrono_literals;

namespace webbridge::impl {

// Track which webviews have been initialized
static std::unordered_set<webview::webview*> initialized_webviews;

void init_webview(webview::webview* ptr, obj_deleter_fun fun) {
	if (!ptr || initialized_webviews.count(ptr)) {
		return;
	}

	// Bind the destroy handler
	ptr->bind("__webbridge_destroy", [fun](const std::string& req) -> std::string {
		auto args = nlohmann::json::parse(req);
		auto object_id = args.at(0).get<std::string>();
		fun(object_id);
		return "null";
	});

	initialized_webviews.insert(ptr);
}

bool is_webview_initialized(webview::webview* ptr) {
	return ptr && initialized_webviews.count(ptr) > 0;
}

std::string generate_js_class_wrapper(
	std::string_view type_name,
	const std::vector<std::string>& sync_methods,
	const std::vector<std::string>& async_methods,
	const std::vector<std::string>& properties,
	const std::vector<std::string>& events,
	const std::vector<std::string>& instance_constants,
	const nlohmann::json& static_constants)
{
	// Combine sync and async methods for the JS interface
	std::vector<std::string> all_methods;
	all_methods.insert(all_methods.end(),
		sync_methods.begin(), sync_methods.end());
	all_methods.insert(all_methods.end(),
		async_methods.begin(), async_methods.end());

	std::string js = std::format(R"(
(function __webbridge_init_{0}() {{
	if (!window.WebbridgeRuntime) {{
		setTimeout(__webbridge_init_{0}, 5);
		return;
	}}
	console.log('[Webbridge] Start creating class: {0}');
	try {{
		window.WebbridgeRuntime.createClass({{
			className: "{0}",
			properties: {1},
			events: {2},
			methods: {3},
			instanceConstants: {4},
			staticConstants: {5}
		}});
		console.log('[Webbridge] Successfully created class: {0}');
	}} catch (error) {{
		console.error('[Webbridge] Error creating class {0}:', error);
		throw error;
	}}
}})();
)",
		type_name,
		nlohmann::json(properties).dump(),
		nlohmann::json(events).dump(),
		nlohmann::json(all_methods).dump(),
		nlohmann::json(instance_constants).dump(),
		static_constants.dump());

	return js;
}

} // namespace webbridge::impl
