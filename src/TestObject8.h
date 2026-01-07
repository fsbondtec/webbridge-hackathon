#pragma once

#include "webbridge/object.h"
#include <string>

class TestObject8 : public webbridge::object
{
public:
    // Properties
    property<int> prop1{100};
    property<std::string> prop2{"Default8"};

public:
    TestObject8() = default;

    // Constants
    static constexpr int CONST1 = 8 * 10;
    static constexpr int CONST2 = 8 * 20;

    // Methods
    int method1(int x) { return x + CONST1; }
    [[async]] std::string method2(const std::string& s);
};
