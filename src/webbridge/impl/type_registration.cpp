#include "type_registration.h"
#include <format>

namespace webbridge::impl {

std::string generate_js_class_wrapper(
	std::string_view type_name,
	const std::vector<std::string>& methods,
	const std::vector<std::string>& properties,
	const std::vector<std::string>& events,
	const std::vector<std::string>& instance_constants,
	const std::vector<static_constant>& static_constants)
{
	auto to_json_array = [](const std::vector<std::string>& items) -> std::string {
		if (items.empty()) return "[]";
		std::string result = "[";
		for (size_t i = 0; i < items.size(); ++i) {
			if (i > 0) result += ",";
			result += "\"" + items[i] + "\"";
		}
		return result + "]";
	};

	// Build static constants JSON object
	auto to_json_object = [](const std::vector<static_constant>& constants) -> std::string {
		if (constants.empty()) return "{}";
		std::string result = "{";
		for (size_t i = 0; i < constants.size(); ++i) {
			if (i > 0) result += ",";
			result += "\"" + constants[i].first + "\":" + constants[i].second;
		}
		return result + "}";
	};

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
		to_json_array(properties),
		to_json_array(events),
		to_json_array(methods),
		to_json_array(instance_constants),
		to_json_object(static_constants));

	return js;
}

} // namespace webbridge::impl
