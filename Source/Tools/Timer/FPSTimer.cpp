#include "FPSTimer.hpp"

namespace Shift::tool {
    bool FPSTimer::HasFrameElapsed() {
        const auto current_time = clock::now();
        m_dt = current_time - m_base;
        m_base = current_time;

        m_lag += m_dt;

        if (m_lag > m_fixed_dt) {
            ++m_ActualFPS;
            m_lag = 0ns;
            return true;
        }

        return false;
    }

    std::pair<bool, float> FPSTimer::IsDebugFPSShow() {
        std::pair<bool, float> res;
        if (m_base - m_lastUpdated > 1s)
        {
            res.first = true;
            res.second = m_ActualFPS;

            m_lastUpdated = m_base;
            m_ActualFPS = 0;
            return res;
        }
        res.first = false;
        res.second = 0.0f;
        return res;
    }

    float FPSTimer::GetFPSCurrent() const { return m_ActualFPS; }

    float FPSTimer::GetSecondsSinceStart() const {
        const std::chrono::duration<float, std::ratio<1>> t = std::chrono::duration_cast<std::chrono::milliseconds>(sys_clock::now() - m_progStart);
        return t.count();
    }
    float FPSTimer::GetDt() const
    {
        return static_cast<float>(m_dt.count()) / 1'000'000'000.0f;
    }

    float FPSTimer::GetFrameTimeInMs() const
    {
        return static_cast<float>(m_dt.count()) / 1'000'000.0f;
    }

} // shift::tool