#pragma once

#include <functional>
#include <nlohmann/json.hpp>

namespace webbridge {

struct error;

/**
 * Callback-Typ für benutzerdefinierten Error-Handler
 * 
 * Wird aufgerufen, wenn ein C++-Fehler auftritt.
 * Kann den Error modifizieren (z.B. Details, Stack hinzufügen).
 */
using error_handler = std::function<void(error& err, const std::exception& ex)>;

namespace impl {

void set_error_handler(error_handler handler);
void clear_error_handler();
bool has_error_handler();

error from_json_exception(const nlohmann::json::exception& ex);
error from_cpp_exception(const std::exception& ex, int code);
error unknown_error();

} // namespace impl
} // namespace webbridge
