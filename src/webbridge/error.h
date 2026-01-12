#pragma once

/**
 * WebBridge Error Handling
 *
 * Defines a unified error format for C++/JavaScript communication.
 *
 * Error codes:
 * - 4000-4999: JavaScript/Client errors (deserialization, type errors)
 * - 5000-5999: C++/Server errors (runtime, custom)
 */

#include <string>
#include <optional>
#include <functional>
#include <nlohmann/json.hpp>
#include "impl/error_handler.h"

namespace webbridge {

enum error_code : int {
	// =========================================
	// 4xxx: JavaScript/Client errors (JSON deserialization)
	// These codes are fixed and indicate JS bugs
	// =========================================
	JSON_PARSE_ERROR		= 4001,  // Invalid JSON syntax
	JSON_TYPE_ERROR			= 4002,  // Wrong JSON type (e.g. string instead of int)
	JSON_ACCESS_ERROR		= 4003,  // Missing key or array index
	INVALID_ARGUMENT		= 4004,  // Invalid argument from JS
	OBJECT_NOT_FOUND		= 4005,  // Object ID not found in registry

	// =========================================
	// 5xxx: C++/Server errors (runtime)
	// These codes can be extended by custom handlers
	// =========================================
	RUNTIME_ERROR			= 5000,  // Generic runtime error
	NETWORK_ERROR			= 5001,  // Network/connection error
	FILE_ERROR				= 5002,  // File I/O error
	TIMEOUT_ERROR			= 5003,  // Timeout
	PERMISSION_ERROR		= 5004,  // Permission error
	CUSTOM_ERROR			= 5500,  // Custom error (start)
};

enum class error_origin {
	JAVASCRIPT,  // Error originated from JS side (deserialization)
	CPP,         // Error originated from C++ side (runtime)
	UNKNOWN      // Error origin unknown
};

inline std::string to_string(error_origin origin) {
	switch (origin) {
		case error_origin::JAVASCRIPT: return "javascript";
		case error_origin::CPP: return "cpp";
		case error_origin::UNKNOWN: return "unknown";
		default: return "unknown";
	}
}

struct error {
	int code;                                 // Error code (4xxx or 5xxx)
	std::string message;                      // Human-readable description
	error_origin origin;                      // Origin of the error
	std::optional<nlohmann::json> details;    // Additional structured data
	std::optional<std::string> stack;         // Callstack (if available)

	error(int code, std::string message, error_origin origin = error_origin::UNKNOWN)
		: code(code), message(std::move(message)), origin(origin) {}

	error& with_details(nlohmann::json d) {
		details = std::move(d);
		return *this;
	}

	error& with_stack(std::string s) {
		stack = std::move(s);
		return *this;
	}

	error& with_origin(error_origin o) {
		origin = o;
		return *this;
	}

	nlohmann::json to_json() const {
		nlohmann::json j;
		j["code"] = code;
		j["message"] = message;
		j["details"] = details.has_value() ? details.value() : nullptr;
		j["stack"] = stack.has_value() ? nlohmann::json(stack.value()) : nlohmann::json();
		j["origin"] = to_string(origin);
		return j;
	}
	
	std::string dump() const {
		return nlohmann::json{{"error", to_json()}}.dump();
	}
};

// =========================================
// Error Handler API (Public)
// =========================================

/**
 * Sets the global error handler
 *
 * Example:
 * webbridge::set_error_handler([](webbridge::error& err, const std::exception& ex) {
 *     err.with_stack(getCallstack());
 *     err.with_details({{"thread_id", std::this_thread::get_id()}});
 *     log_error(err);
 * });
 */
inline void set_error_handler(error_handler handler) {
	impl::set_error_handler(std::move(handler));
}

inline void clear_error_handler() {
	impl::clear_error_handler();
}

} // namespace webbridge
