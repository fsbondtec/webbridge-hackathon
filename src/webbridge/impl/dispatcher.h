#pragma once

/**
 * WebBridge Dispatcher - Central binding dispatch system
 * 
 * Instead of creating N bind() calls per class (which causes O(n²) performance),
 * we use a single set of dispatcher bindings that route to class-specific handlers.
 * 
 * This reduces startup time from ~440ms to ~20ms for 37 classes.
 */

#include <string>
#include <functional>
#include <unordered_map>
#include <memory>
#include <nlohmann/json.hpp>
#include "webview/webview.h"

namespace webbridge::impl {

// Forward declaration
class object_registry;

// =============================================================================
// Handler Types
// =============================================================================

// Sync handler: Called on main thread, returns result immediately
// Parameters: webview&, registry&, req_id, object_id, operation, member, args_json
using sync_handler_t = std::function<void(
    webview::webview&,
    object_registry&,
    const std::string& req_id,
    const std::string& object_id,
    const std::string& operation,
    const std::string& member,
    const nlohmann::json& args
)>;

// Async handler: Called on background thread
// Parameters: webview&, registry&, req_id, object_id, method, args_json
using async_handler_t = std::function<void(
    webview::webview&,
    object_registry&,
    const std::string& req_id,
    const std::string& object_id,
    const std::string& method,
    const nlohmann::json& args
)>;

// Create handler: Creates new instance
// Parameters: webview&, registry&, args_json
// Returns: object_id
using create_handler_t = std::function<std::string(
    webview::webview&,
    object_registry&,
    const nlohmann::json& args
)>;

// =============================================================================
// Class Handler - Contains all handlers for a single C++ class
// =============================================================================

struct class_handler {
    std::string class_name;
    sync_handler_t sync;
    async_handler_t async;
    create_handler_t create;
};

// =============================================================================
// Dispatcher Registry - Singleton that holds all class handlers
// =============================================================================

class dispatcher_registry {
public:
    static dispatcher_registry& instance() {
        static dispatcher_registry inst;
        return inst;
    }

    // Register handlers for a class
    void register_class(const std::string& class_name, class_handler handler) {
        handlers_[class_name] = std::move(handler);
    }

    // Check if a class is registered
    bool has_class(const std::string& class_name) const {
        return handlers_.count(class_name) > 0;
    }

    // Get handler for a class (throws if not found)
    const class_handler& get_handler(const std::string& class_name) const {
        auto it = handlers_.find(class_name);
        if (it == handlers_.end()) {
            throw std::runtime_error("Unknown class: " + class_name);
        }
        return it->second;
    }

    // Get all registered class names (for debugging)
    std::vector<std::string> get_class_names() const {
        std::vector<std::string> names;
        names.reserve(handlers_.size());
        for (const auto& [name, _] : handlers_) {
            names.push_back(name);
        }
        return names;
    }

private:
    dispatcher_registry() = default;
    std::unordered_map<std::string, class_handler> handlers_;
};

// =============================================================================
// Convenience function to register a class handler
// =============================================================================

inline void register_class_handler(
    const std::string& class_name,
    sync_handler_t sync,
    async_handler_t async,
    create_handler_t create)
{
    dispatcher_registry::instance().register_class(class_name, {
        class_name,
        std::move(sync),
        std::move(async),
        std::move(create)
    });
}

} // namespace webbridge::impl
