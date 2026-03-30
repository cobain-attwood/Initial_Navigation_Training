#include <iostream>
#include <chrono>
#include <ctime>
#include <cmath>

class Timer
{
public:
    void start();
    void stop();
    double elapsedMicroseconds();
    double elapsedMilliseconds();
    double elapsedSeconds();

private:
    std::chrono::time_point<std::chrono::steady_clock> StartTime;
    std::chrono::time_point<std::chrono::steady_clock> EndTime;
    bool Running = false;
};
