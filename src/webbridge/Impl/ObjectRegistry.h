#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <atomic>
#include <format>
#include <optional>

namespace webbridge::Impl {

class ObjectRegistry {
public:
	using ObjectPtr = std::shared_ptr<void>;

	static ObjectRegistry& instance() {
		static ObjectRegistry registry;
		return registry;
	}

	template<typename T>
	std::string registerObject(std::shared_ptr<T> obj, std::string_view typeName) {
		auto id = generateId(typeName);
		std::unique_lock lock(m_mutex);
		m_objects[id] = std::static_pointer_cast<void>(obj);
		return id;
	}

	template<typename T>
	std::shared_ptr<T> get(const std::string& id) {
		std::shared_lock lock(m_mutex);
		auto it = m_objects.find(id);
		if (it == m_objects.end()) {
			return nullptr;
		}
		return std::static_pointer_cast<T>(it->second);
	}

	bool remove(const std::string& id) {
		std::unique_lock lock(m_mutex);
		return m_objects.erase(id) > 0;
	}

	bool contains(const std::string& id) const {
		std::shared_lock lock(m_mutex);
		return m_objects.contains(id);
	}

private:
	ObjectRegistry() = default;

	std::string generateId(std::string_view typeName) {
		auto counter = m_counter.fetch_add(1, std::memory_order_relaxed);
		return std::format("{}_{}", typeName, counter);
	}

	mutable std::shared_mutex m_mutex;
	std::unordered_map<std::string, ObjectPtr> m_objects;
	std::atomic<uint64_t> m_counter{0};
};

} // namespace webbridge::Impl
