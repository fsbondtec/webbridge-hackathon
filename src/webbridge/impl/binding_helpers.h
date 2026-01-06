#pragma once

/**
 * WebBridge Binding Helpers
 * 
 * Reusable template functions for binding C++ objects to JavaScript.
 * Used by auto-generated registration files.
 */

#include <webview/webview.h>
#include <memory>
#include <thread>
#include <tuple>
#include <functional>
#include <type_traits>
#include <stdexcept>
#include <typeinfo>
#include <nlohmann/json.hpp>

#include "object_registry.h"
#include "property_impl.h"
#include "event_impl.h"
#include "concepts.h"
#include "../Object.h"
#include "../Error.h"

namespace webbridge::impl {

// =============================================================================
// Central Error Handling
// =============================================================================

/**
 * Executes a callable, serializes result to JSON, catches all exceptions.
 * Returns tuple<status_code, json_string>.
 * 
 * Success: {0, <value>}        - Direct value for clean JS API
 * Error:   {code, {"error":...}} - Structured error object
 */
template<typename Callable, typename... Args>
std::tuple<int, std::string> invoke_and_serialize(Callable&& func, Args&&... args) {
	try {
		using result_type = std::invoke_result_t<Callable, Args...>;
		
		if constexpr (std::is_void_v<result_type>) {
			std::invoke(std::forward<Callable>(func), std::forward<Args>(args)...);
			return {0, "null"};
		} else {
			auto result = std::invoke(std::forward<Callable>(func), std::forward<Args>(args)...);
			return {0, nlohmann::json(result).dump()};
		}
	}
	catch (const nlohmann::json::exception& ex) {
		auto err = from_json_exception(ex);
		return {err.code, err.dump()};
	}
	catch (const std::exception& ex) {
		auto err = from_cpp_exception(ex, RUNTIME_ERROR);
		return {err.code, err.dump()};
	}
	catch (...) {
		auto err = unknown_error();
		return {err.code, err.dump()};
	}
}

// =============================================================================
// Object Lookup Helper
// =============================================================================

template<typename TObj>
std::shared_ptr<TObj> get_object_or_throw(object_registry& registry, const std::string& object_id) {
	auto obj = registry.get<TObj>(object_id);
	if (!obj) {
		throw std::runtime_error("Object not found: " + object_id);
	}
	return obj;
}

// =============================================================================
// Argument Extraction
// =============================================================================

inline std::string extract_object_id(const nlohmann::json& args) {
	return args.at(0).get<std::string>();
}

template<typename... Args, std::size_t... Is>
std::tuple<Args...> extract_args_impl(const nlohmann::json& args, std::index_sequence<Is...>) {
	return std::make_tuple(args.at(Is + 1).get<Args>()...);
}

template<typename... Args>
std::tuple<Args...> extract_args(const nlohmann::json& args) {
	return extract_args_impl<Args...>(args, std::index_sequence_for<Args...>{});
}

// =============================================================================
// Property Subscription
// =============================================================================

template<typename T>
void subscribe_property(
	webview::webview& w_ref,
	const std::string& object_id,
	const std::string& prop_name,
	property<T>& prop)
{
	prop.set_on_changed([&w_ref, object_id, prop_name](const T& val) {
		w_ref.dispatch([&w_ref, object_id, prop_name, val]() {
			w_ref.eval(std::format(
				"window.__webbridge_notify('{}', '{}', {})",
				object_id, prop_name, nlohmann::json(val).dump()
			));
		});
	});
}

// =============================================================================
// Event Subscription
// =============================================================================

template<typename... Args>
void subscribe_event(
	webview::webview& w_ref,
	const std::string& object_id,
	const std::string& event_name,
	event<Args...>& evt)
{
	evt.set_forwarder([&w_ref, object_id, event_name](Args... args) {
		w_ref.dispatch([&w_ref, object_id, event_name, args...]() {
			std::string args_str;
			((args_str += nlohmann::json(args).dump() + ", "), ...);
			if (!args_str.empty()) {
				args_str.resize(args_str.size() - 2);
			}
			w_ref.eval(std::format(
				"window.__webbridge_emit('{}', '{}', {})",
				object_id, event_name, args_str
			));
		});
	});
}

// =============================================================================
// Property Getter Binding
// =============================================================================

template<is_webbridge_object TObj, typename T>
void bind_property_getter(
	webview::webview& w_ref,
	object_registry& registry,
	std::string_view type_name,
	std::string_view prop_name,
	property<T> TObj::* prop_ptr)
{
	auto bind_name = std::format("__get_{}_{}", type_name, prop_name);
	
	w_ref.bind(bind_name,
		[&registry, prop_ptr](const std::string& req_id, const std::string& req, void* w_ptr) {
			auto& w_ref = *static_cast<webview::webview*>(w_ptr);
			auto [status, json] = invoke_and_serialize([&]() {
				auto args = nlohmann::json::parse(req);
				auto obj = get_object_or_throw<TObj>(registry, extract_object_id(args));
				return (obj.get()->*prop_ptr)();
			});
			w_ref.resolve(req_id, status, json);
		}, &w_ref);
}

// =============================================================================
// Sync Method Binding
// =============================================================================

template<is_webbridge_object TObj, typename Ret, typename... Args>
void bind_sync_method(
	webview::webview& w_ref,
	object_registry& registry,
	std::string_view type_name,
	std::string_view method_name,
	Ret (TObj::*method)(Args...))
{
	auto bind_name = std::format("__{}_{}", type_name, method_name);
	
	w_ref.bind(bind_name,
		[&registry, method](const std::string& req_id, const std::string& req, void* w_ptr) {
			auto& w_ref = *static_cast<webview::webview*>(w_ptr);
			auto [status, json] = invoke_and_serialize([&]() {
				auto args = nlohmann::json::parse(req);
				auto obj = get_object_or_throw<TObj>(registry, extract_object_id(args));
				
				if constexpr (sizeof...(Args) == 0) {
					return (obj.get()->*method)();
				} else {
					return std::apply([&](auto&&... a) {
						return (obj.get()->*method)(std::forward<decltype(a)>(a)...);
					}, extract_args<std::decay_t<Args>...>(args));
				}
			});
			w_ref.resolve(req_id, status, json);
		}, &w_ref);
}

// Const method overload
template<is_webbridge_object TObj, typename Ret, typename... Args>
void bind_sync_method(
	webview::webview& w_ref,
	object_registry& registry,
	std::string_view type_name,
	std::string_view method_name,
	Ret (TObj::*method)(Args...) const)
{
	auto bind_name = std::format("__{}_{}", type_name, method_name);
	
	w_ref.bind(bind_name,
		[&registry, method](const std::string& req_id, const std::string& req, void* w_ptr) {
			auto& w_ref = *static_cast<webview::webview*>(w_ptr);
			auto [status, json] = invoke_and_serialize([&]() {
				auto args = nlohmann::json::parse(req);
				auto obj = get_object_or_throw<TObj>(registry, extract_object_id(args));
				
				if constexpr (sizeof...(Args) == 0) {
					return (obj.get()->*method)();
				} else {
					return std::apply([&](auto&&... a) {
						return (obj.get()->*method)(std::forward<decltype(a)>(a)...);
					}, extract_args<std::decay_t<Args>...>(args));
				}
			});
			w_ref.resolve(req_id, status, json);
		}, &w_ref);
}

// =============================================================================
// Async Method Binding
// =============================================================================

template<is_webbridge_object TObj, typename Ret, typename... Args>
void bind_async_method(
	webview::webview& w_ref,
	object_registry& registry,
	std::string_view type_name,
	std::string_view method_name,
	Ret (TObj::*method)(Args...))
{
	auto bind_name = std::format("__{}_{}", type_name, method_name);
	
	w_ref.bind(bind_name,
		[&registry, &w_ref, method](const std::string& req_id, const std::string& req, void*) {
			std::thread([&registry, &w_ref, method, req_id, req]() {
				auto [status, json] = invoke_and_serialize([&]() {
					auto args = nlohmann::json::parse(req);
					auto obj = get_object_or_throw<TObj>(registry, extract_object_id(args));
					
					if constexpr (sizeof...(Args) == 0) {
						return (obj.get()->*method)();
					} else {
						return std::apply([&](auto&&... a) {
							return (obj.get()->*method)(std::forward<decltype(a)>(a)...);
						}, extract_args<std::decay_t<Args>...>(args));
					}
				});
				w_ref.resolve(req_id, status, json);
			}).detach();
		}, nullptr);
}

// =============================================================================
// Instance Constant Getter Binding
// =============================================================================

template<is_webbridge_object TObj, typename T>
void bind_instance_constant_getter(
	webview::webview& w_ref,
	object_registry& registry,
	std::string_view type_name,
	std::string_view const_name,
	T TObj::* const_ptr)
{
	auto bind_name = std::format("__get_{}_{}", type_name, const_name);
	
	w_ref.bind(bind_name,
		[&registry, const_ptr](const std::string& req_id, const std::string& req, void* w_ptr) {
			auto& w_ref = *static_cast<webview::webview*>(w_ptr);
			auto [status, json] = invoke_and_serialize([&]() {
				auto args = nlohmann::json::parse(req);
				auto obj = get_object_or_throw<TObj>(registry, extract_object_id(args));
				return obj.get()->*const_ptr;
			});
			w_ref.resolve(req_id, status, json);
		}, &w_ref);
}

} // namespace webbridge::impl
