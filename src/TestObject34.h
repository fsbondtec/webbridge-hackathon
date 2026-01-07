#pragma once

#include "webbridge/object.h"
#include <string>

class TestObject34 : public webbridge::object
{
public:
    // Properties
    property<int> prop1{100};
    property<std::string> prop2{"Default34"};

public:
    TestObject34() = default;

    // Constants
    static constexpr int CONST1 = 34 * 10;
    static constexpr int CONST2 = 34 * 20;

    // Methods
    int method1(int x) { return x + CONST1; }
    [[async]] std::string method2(const std::string& s);
};
