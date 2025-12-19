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
#include "Impl/ErrorHandler.h"

namespace webbridge {

enum ErrorCode : int {
	// =========================================
	// 4xxx: JavaScript/Client errors (JSON deserialization)
	// These codes are fixed and indicate JS bugs
	// =========================================
	JsonParseError		= 4001,  // Invalid JSON syntax
	JsonTypeError		= 4002,  // Wrong JSON type (e.g. string instead of int)
	JsonAccessError		= 4003,  // Missing key or array index
	InvalidArgument		= 4004,  // Invalid argument from JS
	ObjectNotFound		= 4005,  // Object ID not found in registry

	// =========================================
	// 5xxx: C++/Server errors (runtime)
	// These codes can be extended by custom handlers
	// =========================================
	RuntimeError		= 5000,  // Generic runtime error
	NetworkError		= 5001,  // Network/connection error
	FileError			= 5002,  // File I/O error
	TimeoutError		= 5003,  // Timeout
	PermissionError		= 5004,  // Permission error
	CustomError			= 5500,  // Custom error (start)
};

enum class ErrorOrigin {
	JavaScript,  // Error originated from JS side (deserialization)
	Cpp,         // Error originated from C++ side (runtime)
	Unknown      // Error origin unknown
};

inline std::string to_string(ErrorOrigin origin) {
	switch (origin) {
		case ErrorOrigin::JavaScript: return "javascript";
		case ErrorOrigin::Cpp: return "cpp";
		case ErrorOrigin::Unknown: return "unknown";
		default: return "unknown";
	}
}

struct Error {
	int code;                                 // Error code (4xxx or 5xxx)
	std::string message;                      // Human-readable description
	ErrorOrigin origin;                       // Origin of the error
	std::optional<nlohmann::json> details;    // Additional structured data
	std::optional<std::string> stack;         // Callstack (if available)

	Error(int code, std::string message, ErrorOrigin origin = ErrorOrigin::Unknown)
		: code(code), message(std::move(message)), origin(origin) {}

	Error& withDetails(nlohmann::json d) {
		details = std::move(d);
		return *this;
	}

	Error& withStack(std::string s) {
		stack = std::move(s);
		return *this;
	}

	Error& withOrigin(ErrorOrigin o) {
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
 * webbridge::setErrorHandler([](webbridge::Error& error, const std::exception& ex) {
 *     error.withStack(getCallstack());
 *     error.withDetails({{"thread_id", std::this_thread::get_id()}});
 *     logError(error);
 * });
 */
inline void setErrorHandler(ErrorHandler handler) {
	Impl::setErrorHandler(std::move(handler));
}

inline void clearErrorHandler() {
	Impl::clearErrorHandler();
}

} // namespace webbridge
