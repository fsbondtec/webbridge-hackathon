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

#include "ObjectRegistry.h"
#include "PropertyImpl.h"
#include "EventImpl.h"
#include "Concepts.h"
#include "../Object.h"
#include "../Error.h"

namespace webbridge::Impl {

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
std::tuple<int, std::string> invokeAndSerialize(Callable&& func, Args&&... args) {
	try {
		using ResultType = std::invoke_result_t<Callable, Args...>;
		
		if constexpr (std::is_void_v<ResultType>) {
			std::invoke(std::forward<Callable>(func), std::forward<Args>(args)...);
			return {0, "null"};
		} else {
			auto result = std::invoke(std::forward<Callable>(func), std::forward<Args>(args)...);
			return {0, nlohmann::json(result).dump()};
		}
	}
	catch (const nlohmann::json::exception& ex) {
		auto error = fromJsonException(ex);
		return {error.code, error.dump()};
	}
	catch (const std::exception& ex) {
		auto error = fromCppException(ex, ErrorCode::RuntimeError);
		return {error.code, error.dump()};
	}
	catch (...) {
		auto error = unknownError();
		return {error.code, error.dump()};
	}
}

// =============================================================================
// Object Lookup Helper
// =============================================================================

template<typename TObj>
std::shared_ptr<TObj> getObjectOrThrow(ObjectRegistry& registry, const std::string& objectId) {
	auto obj = registry.get<TObj>(objectId);
	if (!obj) {
		throw std::runtime_error("Object not found: " + objectId);
	}
	return obj;
}

// =============================================================================
// Argument Extraction
// =============================================================================

inline std::string extractObjectId(const nlohmann::json& args) {
	return args.at(0).get<std::string>();
}

template<typename... Args, std::size_t... Is>
std::tuple<Args...> extractArgsImpl(const nlohmann::json& args, std::index_sequence<Is...>) {
	return std::make_tuple(args.at(Is + 1).get<Args>()...);
}

template<typename... Args>
std::tuple<Args...> extractArgs(const nlohmann::json& args) {
	return extractArgsImpl<Args...>(args, std::index_sequence_for<Args...>{});
}

// =============================================================================
// Property Subscription
// =============================================================================

template<IsWebBridgeObject TObj, typename T>
void subscribeProperty(
	webview::webview& wRef,
	const std::string& objectId,
	const std::string& propName,
	Property<T>& prop)
{
	prop.setOnChanged([&wRef, objectId, propName](const T& val) {
		wRef.dispatch([&wRef, objectId, propName, val]() {
			wRef.eval(std::format(
				"window.__webbridge_notify('{}', '{}', {})",
				objectId, propName, nlohmann::json(val).dump()
			));
		});
	});
}

// =============================================================================
// Event Subscription
// =============================================================================

template<IsWebBridgeObject TObj, typename... Args>
void subscribeEvent(
	webview::webview& wRef,
	const std::string& objectId,
	const std::string& eventName,
	Event<Args...>& event)
{
	event.setForwarder([&wRef, objectId, eventName](Args... args) {
		wRef.dispatch([&wRef, objectId, eventName, args...]() {
			std::string argsStr;
			((argsStr += nlohmann::json(args).dump() + ", "), ...);
			if (!argsStr.empty()) {
				argsStr.resize(argsStr.size() - 2);
			}
			wRef.eval(std::format(
				"window.__webbridge_emit('{}', '{}', {})",
				objectId, eventName, argsStr
			));
		});
	});
}

// =============================================================================
// Property Getter Binding
// =============================================================================

template<IsWebBridgeObject TObj, typename T>
void bindPropertyGetter(
	webview::webview& wRef,
	ObjectRegistry& registry,
	std::string_view typeName,
	std::string_view propName,
	Property<T> TObj::* propPtr)
{
	auto bindName = std::format("__get_{}_{}", typeName, propName);
	
	wRef.bind(bindName,
		[&registry, propPtr](const std::string& reqId, const std::string& req, void* wPtr) {
			auto& wRef = *static_cast<webview::webview*>(wPtr);
			auto [status, json] = invokeAndSerialize([&]() {
				auto args = nlohmann::json::parse(req);
				auto obj = getObjectOrThrow<TObj>(registry, extractObjectId(args));
				return (obj.get()->*propPtr)();
			});
			wRef.resolve(reqId, status, json);
		}, &wRef);
}

// =============================================================================
// Sync Method Binding
// =============================================================================

template<IsWebBridgeObject TObj, typename Ret, typename... Args>
void bindSyncMethod(
	webview::webview& wRef,
	ObjectRegistry& registry,
	std::string_view typeName,
	std::string_view methodName,
	Ret (TObj::*method)(Args...))
{
	auto bindName = std::format("__{}_{}", typeName, methodName);
	
	wRef.bind(bindName,
		[&registry, method](const std::string& reqId, const std::string& req, void* wPtr) {
			auto& wRef = *static_cast<webview::webview*>(wPtr);
			auto [status, json] = invokeAndSerialize([&]() {
				auto args = nlohmann::json::parse(req);
				auto obj = getObjectOrThrow<TObj>(registry, extractObjectId(args));
				
				if constexpr (sizeof...(Args) == 0) {
					return (obj.get()->*method)();
				} else {
					return std::apply([&](auto&&... a) {
						return (obj.get()->*method)(std::forward<decltype(a)>(a)...);
					}, extractArgs<std::decay_t<Args>...>(args));
				}
			});
			wRef.resolve(reqId, status, json);
		}, &wRef);
}

// Const method overload
template<IsWebBridgeObject TObj, typename Ret, typename... Args>
void bindSyncMethod(
	webview::webview& wRef,
	ObjectRegistry& registry,
	std::string_view typeName,
	std::string_view methodName,
	Ret (TObj::*method)(Args...) const)
{
	auto bindName = std::format("__{}_{}", typeName, methodName);
	
	wRef.bind(bindName,
		[&registry, method](const std::string& reqId, const std::string& req, void* wPtr) {
			auto& wRef = *static_cast<webview::webview*>(wPtr);
			auto [status, json] = invokeAndSerialize([&]() {
				auto args = nlohmann::json::parse(req);
				auto obj = getObjectOrThrow<TObj>(registry, extractObjectId(args));
				
				if constexpr (sizeof...(Args) == 0) {
					return (obj.get()->*method)();
				} else {
					return std::apply([&](auto&&... a) {
						return (obj.get()->*method)(std::forward<decltype(a)>(a)...);
					}, extractArgs<std::decay_t<Args>...>(args));
				}
			});
			wRef.resolve(reqId, status, json);
		}, &wRef);
}

// =============================================================================
// Async Method Binding
// =============================================================================

template<IsWebBridgeObject TObj, typename Ret, typename... Args>
void bindAsyncMethod(
	webview::webview& wRef,
	ObjectRegistry& registry,
	std::string_view typeName,
	std::string_view methodName,
	Ret (TObj::*method)(Args...))
{
	auto bindName = std::format("__{}_{}", typeName, methodName);
	
	wRef.bind(bindName,
		[&registry, &wRef, method](const std::string& reqId, const std::string& req, void*) {
			std::thread([&registry, &wRef, method, reqId, req]() {
				auto [status, json] = invokeAndSerialize([&]() {
					auto args = nlohmann::json::parse(req);
					auto obj = getObjectOrThrow<TObj>(registry, extractObjectId(args));
					
					if constexpr (sizeof...(Args) == 0) {
						return (obj.get()->*method)();
					} else {
						return std::apply([&](auto&&... a) {
							return (obj.get()->*method)(std::forward<decltype(a)>(a)...);
						}, extractArgs<std::decay_t<Args>...>(args));
					}
				});
				wRef.resolve(reqId, status, json);
			}).detach();
		}, nullptr);
}

} // namespace webbridge::Impl
