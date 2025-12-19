#pragma once

#include <mutex>
#include <shared_mutex>
#include <functional>

namespace webbridge::Impl {

template<typename T>
class Property {
public:
	using Callback = std::function<void(const T&)>;

	Property() = default;

	explicit Property(T initial) : value_(std::move(initial)) {}

	[[nodiscard]] T value() const {
		std::shared_lock lock(mutex_);
		return value_;
	}

	[[nodiscard]] T operator()() const {
		return value();
	}

	Property& operator=(T newValue) {
		Callback cb;
		{
			std::unique_lock lock(mutex_);
			if (value_ == newValue) {
				return *this;
			}
			value_ = std::move(newValue);
			cb = onChanged_;
		}
		if (cb) {
			cb(value_);
		}
		return *this;
	}

	void setOnChanged(Callback callback) {
		std::unique_lock lock(mutex_);
		onChanged_ = std::move(callback);
	}

	Property(const Property&) = delete;
	Property& operator=(const Property&) = delete;

private:
	mutable std::shared_mutex mutex_;
	T value_{};
	Callback onChanged_;
};

} // namespace webbridge::Impl
