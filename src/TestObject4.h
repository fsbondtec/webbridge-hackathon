#pragma once

#include "webbridge/object.h"
#include <string>

class TestObject4 : public webbridge::object
{
public:
    // Properties
    property<int> prop1{100};
    property<std::string> prop2{"Default4"};

public:
    TestObject4() = default;

    // Constants
    static constexpr int CONST1 = 4 * 10;
    static constexpr int CONST2 = 4 * 20;

    // Methods
    int method1(int x) { return x + CONST1; }
    [[async]] std::string method2(const std::string& s);
};
