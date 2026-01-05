#include "type_registration.h"
#include <format>

namespace webbridge::impl {

std::string generate_js_global_registry()
{
	// The runtime is loaded from frontend/src/webbridge/webbridge-runtime.ts
	// This function does minimal setup - no fallback needed for production
	return R"(
(function() {
	'use strict';

	// Runtime should be initialized by import in main.ts
	// This is just a safety check
	if (!window.WebbridgeRuntime) {
		console.warn('[WebBridge] Runtime not found - make sure webbridge-runtime is imported in main.ts');
		return;
	}

	console.log('[WebBridge] Global registry initialized via runtime');
})();
)";
}

std::string generate_js_class_wrapper(
	std::string_view type_name,
	const std::vector<std::string>& sync_methods,
	const std::vector<std::string>& async_methods,
	const std::vector<std::string>& properties,
	const std::vector<std::string>& events,
	const std::vector<std::string>& instance_constants,
	const std::vector<std::string>& static_constants)
{
	// Helper to format array as JSON
	auto to_json_array = [](const std::vector<std::string>& items) -> std::string {
		if (items.empty()) return "[]";
		std::string result = "[";
		for (size_t i = 0; i < items.size(); ++i) {
			if (i > 0) result += ", ";
			result += "\"" + items[i] + "\"";
		}
		result += "]";
		return result;
	};

	// Generate only metadata - Runtime must be loaded via Vite
	// No fallback code - this is production-only and Runtime is guaranteed
	std::string js = std::format(R"(
(function() {{
	'use strict';

	// Runtime MUST be loaded - no fallback
	if (!window.WebbridgeRuntime || !window.WebbridgeRuntime.createClass) {{
		throw new Error('[WebBridge] Runtime not initialized. Make sure webbridge-runtime is imported in main.ts');
	}}

	// Register class using runtime metadata
	window.{0} = window.WebbridgeRuntime.createClass({{
		className: "{0}",
		properties: {1},
		events: {2},
		syncMethods: {3},
		asyncMethods: {4},
		instanceConstants: {5},
		staticConstants: {{)",
		type_name,
		to_json_array(properties),
		to_json_array(events),
		to_json_array(sync_methods),
		to_json_array(async_methods),
		to_json_array(instance_constants));

	// Add static constants
	bool first = true;
	for (const auto& constant : static_constants) {
		if (!first) js += ",";
		first = false;
		js += std::format("\n\t\t\t\"{0}\": null", constant);
	}

	js += R"(
		}
	});

	console.log('[WebBridge] Class registered: ')";
	js += std::string(type_name);
	js += R"(');
}})();
)";

	return js;
}

std::string generate_js_published_object(
	std::string_view type_name,
	std::string_view var_name,
	std::string_view object_id,
	const std::vector<std::string>& sync_methods,
	const std::vector<std::string>& async_methods,
	const std::vector<std::string>& properties,
	const std::vector<std::string>& events,
	const std::vector<std::string>& instance_constants,
	const std::vector<std::string>& static_constants)
{
	// Helper to format array as JSON
	auto to_json_array = [](const std::vector<std::string>& items) -> std::string {
		if (items.empty()) return "[]";
		std::string result = "[";
		for (size_t i = 0; i < items.size(); ++i) {
			if (i > 0) result += ", ";
			result += "\"" + items[i] + "\"";
		}
		result += "]";
		return result;
	};

	// Generate only metadata - Runtime must be loaded via Vite
	// No fallback code
	std::string js = std::format(R"(
(async function() {{
	'use strict';

	// Runtime MUST be loaded - no fallback
	if (!window.WebbridgeRuntime || !window.WebbridgeRuntime.createPublishedObject) {{
		throw new Error('[WebBridge] Runtime not initialized. Make sure webbridge-runtime is imported in main.ts');
	}}

	// Create published object using runtime
	const obj = window.WebbridgeRuntime.createPublishedObject(
		"{0}",
		"{1}",
		{{
			properties: {2},
			events: {3},
			syncMethods: {4},
			asyncMethods: {5},
			instanceConstants: {6},
			staticConstants: {{}}
		}}
	);
)",
		type_name,
		object_id,
		to_json_array(properties),
		to_json_array(events),
		to_json_array(sync_methods),
		to_json_array(async_methods),
		to_json_array(instance_constants));

	// Fetch instance constants
	for (const auto& constant : instance_constants) {
		js += std::format(R"(
	obj.{0} = await __get_{1}_{0}(obj.handle);
)", constant, type_name);
	}

	js += std::format(R"(
	window.{0} = obj;
	console.log('[WebBridge] Published "{0}" with handle:', obj.handle);
}})();
)", var_name);

	return js;
}

} // namespace webbridge::impl
