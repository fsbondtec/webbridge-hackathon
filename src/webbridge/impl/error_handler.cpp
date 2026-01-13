#include "error_handler.h"
#include "../error.h"

namespace webbridge::impl {

// Globaler Error-Handler
static error_handler g_error_handler;

void set_error_handler(error_handler handler) {
	g_error_handler = std::move(handler);
}

void clear_error_handler() {
	g_error_handler = nullptr;
}

bool has_error_handler() {
	return g_error_handler != nullptr;
}

error from_json_exception(const nlohmann::json::exception& ex) {
	error_code code;
	
	if (ex.id >= 100 && ex.id < 200) {
		code = JSON_PARSE_ERROR;
	} else if (ex.id >= 300 && ex.id < 400) {
		code = JSON_TYPE_ERROR;
	} else if (ex.id >= 400 && ex.id < 500) {
		code = JSON_ACCESS_ERROR;
	} else {
		code = INVALID_ARGUMENT;
	}
	
	return error(code, ex.what())
		.with_origin(error_origin::JAVASCRIPT);
}

error from_cpp_exception(const std::exception& ex, int code, const char* function) {
	if (g_error_handler) {
		error err(code, ex.what());
		err.with_origin(error_origin::CPP);
		if (function)
			err.with_cpp_function(function);
		g_error_handler(err, ex);
		return err;
	}
	else {
		error err(code, ex.what());
		err.with_origin(error_origin::CPP);
		if (function)
			err.with_cpp_function(function);
		return err;	
	}
}

error unknown_error() {
	return error(RUNTIME_ERROR, "Unknown error")
		.with_origin(error_origin::CPP);
}

} // namespace webbridge::impl
