#pragma once

#include <type_traits>
#include "../object.h"

namespace webbridge::impl {

/// Type must derive from webbridge::object
template<typename T>
concept is_webbridge_object = std::is_base_of_v<object, T>;

} // namespace webbridge::impl