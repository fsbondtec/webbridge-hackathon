#include "type_registration.h"
#include <format>

namespace webbridge::impl {

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

	// Generate JS with polling for WebbridgeRuntime
	std::string js = std::format(R"(
(function __webbridge_init_{0}() {{
	if (!window.WebbridgeRuntime) {{
		setTimeout(__webbridge_init_{0}, 5);
		return;
	}}
	const cls = window.WebbridgeRuntime.createClass({{
		className: "{0}",
		properties: {1},
		events: {2},
		methods: {3},
		instanceConstants: {4},
		staticConstants: {5}
	}});
	window.{0} = cls;
	console.log('[WebBridge] Registered: {0}');
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
