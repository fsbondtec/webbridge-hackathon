#pragma once

#include "webbridge/object.h"
#include <string>

class TestObject13 : public webbridge::object
{
public:
    // Properties
    property<int> prop1{100};
    property<std::string> prop2{"Default13"};

public:
    TestObject13() = default;

    // Constants
    static constexpr int CONST1 = 13 * 10;
    static constexpr int CONST2 = 13 * 20;

    // Methods
    int method1(int x) { return x + CONST1; }
    [[async]] std::string method2(const std::string& s);
};
