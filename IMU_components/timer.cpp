#include "timer.h"

void Timer::start()
{
    StartTime = std::chrono::steady_clock::now();
    Running = true;
}

void Timer::stop()
{
    EndTime = std::chrono::steady_clock::now();
    Running = false;
}

double Timer::elapsedMicroseconds()
{
    std::chrono::time_point<std::chrono::steady_clock> endTime;
    
    if(Running)
    {
        endTime = std::chrono::steady_clock::now();
    }
    else
    {
        endTime = EndTime;
    }
    
    return std::chrono::duration_cast<std::chrono::microseconds>(endTime - StartTime).count();
}

double Timer::elapsedMilliseconds()
{
    std::chrono::time_point<std::chrono::steady_clock> endTime;
    
    if(Running)
    {
        endTime = std::chrono::steady_clock::now();
    }
    else
    {
        endTime = EndTime;
    }
    
    return std::chrono::duration_cast<std::chrono::milliseconds>(endTime - StartTime).count();
}

double Timer::elapsedSeconds()
{
    return elapsedMilliseconds() / 1000.0;
}

//for reference 
/*long fibonacci(unsigned n)
{
    if (n < 2) return n;
    return fibonacci(n-1) + fibonacci(n-2);
}

int main()
{
//    std::chrono::time_point<std::chrono::system_clock> start, end;
//    start = std::chrono::system_clock::now();
//    Timer timer;
//    timer.start();
//    std::cout << "f(42) = " << fibonacci(42) << '\n';
//    timer.stop();
//    
//    std::cout << "Time: " << timer.elapsed() << std::endl;
//    end = std::chrono::system_clock::now();
    
//    std::chrono::duration<double> elapsed_seconds = end-start;
//    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
    
//    std::cout << "finished computation at " << std::ctime(&end_time)
//    << "elapsed time: " << elapsed_seconds.count() << "s\n";
    
    Timer timer;
    timer.start();
    int counter = 0;
    double test, test2;
    while(timer.elapsedSeconds() < 10.0)
    {
        counter++;
        test = std::cos(counter / M_PI);
        test2 = std::sin(counter / M_PI);
    }
    timer.stop();
    
    std::cout << counter << std::endl;
    std::cout << "Seconds: " << timer.elapsedSeconds() << std::endl;
    std::cout << "Milliseconds: " << timer.elapsedMilliseconds() << std::endl
    ;
}*/
