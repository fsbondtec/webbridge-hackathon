#pragma once

#include "webbridge/object.h"
#include <string>

class TestObject : public webbridge::object
{
public:
    // Property
    property<std::string> testProp{"Initial Value"};
    
    // Event
    event<std::string> testEvent;

public:
    TestObject() = default;

    // Async Methode
    [[async]] void asyncProcess(const std::string& input);

    // Sync Methode
    int syncCalculate(int value);
};
