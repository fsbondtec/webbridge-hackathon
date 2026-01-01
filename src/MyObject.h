#pragma once

#include "webbridge/Object.h"

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <magic_enum/magic_enum.hpp>


enum class Status {
	Idle,
	Running,
	Completed,
	Error
};

inline void to_json(nlohmann::json& j, const Status& e) {
	j = std::string(magic_enum::enum_name(e));
}

inline void from_json(const nlohmann::json& j, Status& e) {
	auto name = j.get<std::string>();
	auto value = magic_enum::enum_cast<Status>(name);
	if (value.has_value()) {
		e = value.value();
	}
	else {
		throw nlohmann::json::type_error::create(
			302,
			"Invalid enum value: '" + name + "' for type Status",
			nullptr
		);
	}
}

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

class MyObject : public webbridge::Object
{
public:
	Property<bool> aBool{ false };
	Property<std::string> strProp;
	Property<int> counter{ 0 };
	Property<std::vector<int>> numbers;
	Property<Status> status{ Status::Idle };
	Property<Pod> pod{ Pod{0,0} };
	Event<int, bool> aEvent;

	// konstanten
	const std::string version;
	static inline const std::string appversion{"app version"};

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
