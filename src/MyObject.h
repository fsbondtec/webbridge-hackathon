#pragma once

#include "webbridge/object.h"

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <magic_enum/magic_enum.hpp>



struct Pod { 
	unsigned a; unsigned long long b; 
	auto operator<=>(const Pod&) const = default;
};


inline void to_json(nlohmann::json& j, const Pod& p) {
	j = nlohmann::json{{"a", p.a}, {"b", p.b}};
}

inline void from_json(const nlohmann::json& j, Pod& p) {
	j.at("a").get_to(p.a);
	j.at("b").get_to(p.b);
}

class MyObject : public webbridge::object
{
public:
	enum class Status {
		Idle,
		Running,
		Completed,
		Error
	};

	property<bool> aBool{ false };
	property<std::string> strProp;
	property<int> counter{ 0 };
	property<std::vector<int>> numbers;
	property<Status> status{ Status::Idle };
	property<Pod> pod{ Pod{0,0} };
	event<int, bool> aEvent;

	// konstanten
	const std::string version;
	static inline const std::string appversion{"app version"};
	static inline constexpr unsigned cppversion{23};

public:
	explicit MyObject(const std::string& version_) : version(version_) {}
	explicit MyObject() : version("unknown") {;}

	[[async]] void foo(const std::string& val);

	bool bar();

	[[async]] void file();

	void testVectors();

	void throwError();

	std::string multiParamTest(
		int intValue,
		bool boolValue,
		const std::string& strValue,
		const std::vector<int>& vecValue,
		Status statusValue,
		const Pod& podValue
	);

private:

};

inline void to_json(nlohmann::json& j, const MyObject::Status& e) {
	j = std::string(magic_enum::enum_name(e));
}

inline void from_json(const nlohmann::json& j, MyObject::Status& e) {
	auto name = j.get<std::string>();
	auto value = magic_enum::enum_cast<MyObject::Status>(name);
	if (value.has_value()) {
		e = value.value();
	}
	else {
		throw nlohmann::json::type_error::create(
			302,
			"Invalid enum value: '" + name + "' for type MyObject::Status",
			nullptr
		);
	}
}
