#include "ErrorHandler.h"
#include "../Error.h"

namespace webbridge::Impl {

// Globaler Error-Handler
static ErrorHandler g_errorHandler;

void setErrorHandler(ErrorHandler handler) {
	g_errorHandler = std::move(handler);
}

void clearErrorHandler() {
	g_errorHandler = nullptr;
}

bool hasErrorHandler() {
	return g_errorHandler != nullptr;
}

Error fromJsonException(const nlohmann::json::exception& ex) {
	ErrorCode code;
	
	if (ex.id >= 100 && ex.id < 200) {
		code = ErrorCode::JsonParseError;
	} else if (ex.id >= 300 && ex.id < 400) {
		code = ErrorCode::JsonTypeError;
	} else if (ex.id >= 400 && ex.id < 500) {
		code = ErrorCode::JsonAccessError;
	} else {
		code = ErrorCode::InvalidArgument;
	}
	
	return Error(code, ex.what())
		.withOrigin(ErrorOrigin::JavaScript)
		.withDetails({{"nlohmann_id", ex.id}});
}

Error fromCppException(const std::exception& ex, int code) {
	if (g_errorHandler) {
		Error error(code, ex.what());
		g_errorHandler(error, ex);
		return error;
	}
	else {
		Error error(code, ex.what());
		error.withOrigin(ErrorOrigin::Cpp);
		return error;	
	}

}

Error unknownError() {
	return Error(ErrorCode::RuntimeError, "Unknown error")
		.withOrigin(ErrorOrigin::Cpp);
}

} // namespace webbridge::Impl
