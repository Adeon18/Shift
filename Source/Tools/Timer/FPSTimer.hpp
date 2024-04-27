#ifndef SHIFT_FPSTIMER_HPP
#define SHIFT_FPSTIMER_HPP

#include <chrono>
#include <iostream>


namespace shift::tool {
    using namespace std::chrono_literals;

    class FPSTimer {
        using clock = std::chrono::high_resolution_clock;
        using sys_clock = std::chrono::system_clock;
    public:
        //! Default constructor
        FPSTimer(float FPS) :
                m_base{clock::now()},
                m_dt{0ns}, m_lag{0ns},
                m_lastUpdated{clock::now()},
                m_progStart{sys_clock::now()},
                m_FPS{FPS}, m_ActualFPS{},
                m_fixed_dt{static_cast<uint64_t>(1.0f / FPS * std::nano::den)} {}

        //! Check if frame passed and return true if it did
        bool HasFrameElapsed();

        //! Should be called just after frameTimeElapsed to get the correct debug values
        std::pair<bool, float> IsDebugFPSShow();

        float GetFPSCurrent() const;

        //! Get the amount of seconds since the program start
        float GetSecondsSinceStart() const;

        //! Get the delta time
        float GetDt() const;

        float GetFrameTimeInMs() const;

    private:
        // Time point to calculate seconds since the program start from shader
        std::chrono::time_point<sys_clock> m_progStart;
        // Where we start FPS count
        std::chrono::time_point<clock> m_base;

        std::chrono::nanoseconds m_dt;
        std::chrono::nanoseconds m_lag;
        std::chrono::nanoseconds m_fixed_dt;
        std::chrono::time_point<clock> m_lastUpdated;
        // Fps we want the clock to run at
        float m_FPS;
        // FPS we print to debug
        float m_ActualFPS;
    };
} // shift::tool

#endif //SHIFT_FPSTIMER_HPP
