#pragma once

#include "webbridge/object.h"
#include <string>

class TestObject35 : public webbridge::object
{
public:
    // Properties
    property<int> prop1{100};
    property<std::string> prop2{"Default35"};

public:
    TestObject35() = default;

    // Constants
    static constexpr int CONST1 = 35 * 10;
    static constexpr int CONST2 = 35 * 20;

    // Methods
    int method1(int x) { return x + CONST1; }
    [[async]] std::string method2(const std::string& s);
};
