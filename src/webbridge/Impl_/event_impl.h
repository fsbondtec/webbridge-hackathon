#pragma once

#include <functional>

namespace webbridge::impl {

template<typename... Args>
class event {
public:
	using callback = std::function<void(Args...)>;

	event() = default;
	~event() = default;

	event(const event&) = delete;
	event& operator=(const event&) = delete;
	event(event&&) = delete;
	event& operator=(event&&) = delete;

	void set_forwarder(callback callback) {
		m_forwarder = std::move(callback);
	}

	void emit(Args... args) {
		if (m_forwarder) {
			m_forwarder(args...);
		}
	}

private:
	callback m_forwarder;
};

} // namespace webbridge::impl
