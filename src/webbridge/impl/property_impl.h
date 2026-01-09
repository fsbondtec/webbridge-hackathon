#pragma once

#include <mutex>
#include <shared_mutex>
#include <functional>

namespace webbridge::impl {

template<typename T>
class property {
public:
	using callback = std::function<void(const T&)>;

	property() = default;

	explicit property(T initial) : value_(std::move(initial)) {}

	[[nodiscard]] T get() const {
		std::shared_lock lock(mutex_);
		return value_;
	}

	[[nodiscard]] T operator()() const {
		return get();
	}

	// Kurzform für set()
	property& operator=(T newValue) {
		return set(std::move(newValue));
	}

	property& set(T newValue) {
		callback cb;
		{
			std::unique_lock lock(mutex_);
			if (value_ == newValue) {
				return *this;
			}
			value_ = std::move(newValue);
			cb = on_changed_;
		}
		if (cb) {
			cb(value_);
		}
		return *this;
	}

	void set_on_changed(callback callback) {
		std::unique_lock lock(mutex_);
		on_changed_ = std::move(callback);
	}

	property(const property&) = delete;
	property& operator=(const property&) = delete;

private:
	mutable std::shared_mutex mutex_;
	T value_{};
	callback on_changed_;
};

} // namespace webbridge::impl
