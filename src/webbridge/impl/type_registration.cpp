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
		instanceConstants: {4}
	}});
)",
		type_name,
		to_json_array(properties),
		to_json_array(events),
		to_json_array(methods),
		to_json_array(instance_constants));

	// Add static constants directly with values
	for (const auto& [name, json_value] : static_constants) {
		js += std::format("\tcls.{0} = {1};\n", name, json_value);
	}

	js += std::format(R"(	window.{0} = cls;
	console.log('[WebBridge] Registered: {0}');
}})();
)", type_name);

	return js;
}

std::string generate_js_published_object(
	std::string_view type_name,
	std::string_view var_name,
	std::string_view object_id,
	const std::vector<std::string>& methods,
	const std::vector<std::string>& properties,
	const std::vector<std::string>& events,
	const std::vector<std::string>& instance_constants)
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

	// Generate JS with polling for WebbridgeRuntime
	std::string js = std::format(R"(
(async function __webbridge_publish_{2}() {{
	if (!window.WebbridgeRuntime) {{
		setTimeout(__webbridge_publish_{2}, 5);
		return;
	}}
	const obj = window.WebbridgeRuntime.createPublishedObject("{0}", "{1}", {{
		properties: {3},
		events: {4},
		methods: {5},
		instanceConstants: {6}
	}});
)",
		type_name,
		object_id,
		var_name,
		to_json_array(properties),
		to_json_array(events),
		to_json_array(methods),
		to_json_array(instance_constants));

	// Fetch instance constants
	for (const auto& constant : instance_constants) {
		js += std::format("\tobj.{0} = await __get_{1}_{0}(obj.handle);\n", constant, type_name);
	}

	js += std::format(R"(	window.{0} = obj;
	console.log('[WebBridge] Published: {0}');
}})();
)", var_name);

	return js;
}

} // namespace webbridge::impl
