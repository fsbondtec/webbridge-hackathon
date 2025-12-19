#pragma once

#include <type_traits>
#include "../Object.h"

namespace webbridge::Impl {

/// Type must derive from webbridge::Object
template<typename T>
concept IsWebBridgeObject = std::is_base_of_v<Object, T>;

} // namespace webbridge::Impl