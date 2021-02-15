#ifndef CHRONO_HPP
#define CHRONO_HPP

#include <chrono>

/*
 * Chrono<double, std::ratio<1>> chrono => chrono in second 
 * Chrono<double, std::milli> chrono => chrono in millisecond 
 * 
*/

template<typename Rep = double, typename Period = std::milli>
class Chrono
{
public:

    Chrono()
        : running(false)
    {
    }

    ~Chrono()
    {
    }
    
    void start() {
        t1 = std::chrono::steady_clock::now();
        running = true;
    }
    
    void stop() {
        t2 = std::chrono::steady_clock::now();
        running = false;
    }
    
    double elapsedTime() {
        std::chrono::duration<Rep, Period> duration;
        duration = running ? std::chrono::steady_clock::now() - t1 : t2 - t1;
        return duration.count();
    }
    
    const bool isRunning() {
        return running;
    }
    
private :
    std::chrono::time_point<std::chrono::steady_clock> t1;
    std::chrono::time_point<std::chrono::steady_clock> t2;
    bool running;

};

#endif // CHRONO_HPP
