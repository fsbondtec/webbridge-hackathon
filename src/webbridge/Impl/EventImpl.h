#pragma once

#include <functional>

namespace webbridge::Impl {

template<typename... Args>
class Event {
public:
	using Callback = std::function<void(Args...)>;

	Event() = default;
	~Event() = default;

	Event(const Event&) = delete;
	Event& operator=(const Event&) = delete;
	Event(Event&&) = delete;
	Event& operator=(Event&&) = delete;

	void setForwarder(Callback callback) {
		m_forwarder = std::move(callback);
	}

	void emit(Args... args) {
		if (m_forwarder) {
			m_forwarder(args...);
		}
	}

private:
	Callback m_forwarder;
};

} // namespace webbridge::Impl
