#pragma once

#include "webbridge/object.h"
#include <string>

class TestObject1 : public webbridge::object
{
public:

    // Old-style enum
    enum OldEnum {
        ValueA = 0,
        ValueB = 1,
        ValueC = 2
    };

    // enum class
    enum class NewEnum {
        Alpha = 10,
        Beta = 20,
        Gamma = 30
    };

public:
    // Properties
    property<int> prop1{100};
    property<std::string> prop2{"Default1"};
    property<OldEnum> oldEnum{ValueA};
    property<NewEnum> newEnum{NewEnum::Alpha};


public:
    TestObject1() = default;

    // Constants
    static constexpr int CONST1 = 1 * 10;
    static constexpr int CONST2 = 1 * 20;

    // Methods
    int method1(int x) { return x + CONST1; }
    [[async]] std::string method2(const std::string& s);
};
