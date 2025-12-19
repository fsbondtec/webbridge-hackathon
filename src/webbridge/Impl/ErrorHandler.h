#pragma once

#include <functional>
#include <nlohmann/json.hpp>

namespace webbridge {

struct Error;

/**
 * Callback-Typ für benutzerdefinierten Error-Handler
 * 
 * Wird aufgerufen, wenn ein C++-Fehler auftritt.
 * Kann den Error modifizieren (z.B. Details, Stack hinzufügen).
 */
using ErrorHandler = std::function<void(Error& error, const std::exception& ex)>;

namespace Impl {

void setErrorHandler(ErrorHandler handler);
void clearErrorHandler();
bool hasErrorHandler();

Error fromJsonException(const nlohmann::json::exception& ex);
Error fromCppException(const std::exception& ex, int code);
Error unknownError();

} // namespace Impl
} // namespace webbridge
