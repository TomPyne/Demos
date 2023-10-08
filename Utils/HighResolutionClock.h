/**
 * High resolution clock used for performing timings.
 */

#pragma once

#include <chrono>

class HighResolutionClock
{
public:
    HighResolutionClock()
        : m_DeltaTime(0)
        , m_TotalTime(0)
    {
        m_T0 = std::chrono::high_resolution_clock::now();
    }

    // Tick the high resolution clock.
    // Tick the clock before reading the delta time for the first time.
    // Only tick the clock once per frame.
    // Use the Get* functions to return the elapsed time between ticks.
    void Tick()
    {
        auto t1 = std::chrono::high_resolution_clock::now();
        m_DeltaTime = t1 - m_T0;
        m_TotalTime += m_DeltaTime;
        m_T0 = t1;
    }

    // Reset the clock.
    void Reset()
    {
        m_T0 = std::chrono::high_resolution_clock::now();
        m_DeltaTime = std::chrono::high_resolution_clock::duration();
        m_TotalTime = std::chrono::high_resolution_clock::duration();
    }

    double GetDeltaNanoseconds() const { return m_DeltaTime.count() * 1.0; }
    double GetDeltaMicroseconds() const { return m_DeltaTime.count() * 1e-3; }
    double GetDeltaMilliseconds() const { return m_DeltaTime.count() * 1e-6; }
    double GetDeltaSeconds() const { return m_DeltaTime.count() * 1e-9; }

    double GetTotalNanoseconds() const { return m_TotalTime.count() * 1.0; }
    double GetTotalMicroseconds() const { return m_TotalTime.count() * 1e-3; }
    double GetTotalMilliSeconds() const { return m_TotalTime.count() * 1e-6; }
    double GetTotalSeconds() const { return m_TotalTime.count() * 1e-9; }

private:
    // Initial time point.
    std::chrono::high_resolution_clock::time_point m_T0;
    // Time since last tick.
    std::chrono::high_resolution_clock::duration m_DeltaTime;
    std::chrono::high_resolution_clock::duration m_TotalTime;
};