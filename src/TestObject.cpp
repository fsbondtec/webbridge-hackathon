#include "TestObject.h"
#include <thread>
#include <chrono>

void TestObject::asyncProcess(const std::string& input)
{
    // Simuliere asynchrone Operation
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    testProp = "Processed: " + input;
    testEvent.emit("Processing completed for: " + input);
}

int TestObject::syncCalculate(int value)
{
    return value * 2 + 10;
}

int TestObject::benchmarkAsync(int x)
{
    return x + 1;
}
