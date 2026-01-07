#pragma once

#include "webbridge/object.h"
#include <string>

class TestObject17 : public webbridge::object
{
public:
    // Properties
    property<int> prop1{100};
    property<std::string> prop2{"Default17"};

public:
    TestObject17() = default;

    // Constants
    static constexpr int CONST1 = 17 * 10;
    static constexpr int CONST2 = 17 * 20;

    // Methods
    int method1(int x) { return x + CONST1; }
    [[async]] std::string method2(const std::string& s);
};
