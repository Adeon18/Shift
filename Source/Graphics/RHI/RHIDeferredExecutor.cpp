//
// Created by otrush on 11/14/2025.
//

#include "RHIDeferredExecutor.hpp"

namespace Shift {
    void RHIDeferredExecutor::DeferExecute(TimelineSemaphore *timeline, uint64_t value, Callback fn) {
        if (!fn) {
            Log(Warning, "No function passed to RHIDeferredExecutor!");
            return;
        }

        if (!timeline) {
            fn();
            return;
        }

        std::lock_guard<std::mutex> guard(m_mutex);
        m_callbacks[timeline].emplace_back(value, fn);
    }

    void RHIDeferredExecutor::DeferExecuteToFrame(uint64_t frameIdx, Callback fn) {
        if (!fn) {
            Log(Warning, "No function passed to RHIDeferredExecutor!");
            return;
        }

        std::lock_guard<std::mutex> guard(m_mutex);
        m_frameCallbacks[frameIdx].emplace_back(fn);
    }

    void RHIDeferredExecutor::DeferExecuteEndOfSession(Callback fn) {
        std::lock_guard<std::mutex> guard(m_mutex);
        m_endOfSessionCallbacks.emplace_back(fn);
    }

    void RHIDeferredExecutor::ProcessDeferredCallbacks(uint64_t currentFrameGlobalIdx) {
        std::lock_guard<std::mutex> guard(m_mutex);

        for (auto &[sem, callbacks] : m_callbacks) {
            uint64_t semVal = sem->GetCurrentValue();

            for (auto it = callbacks.begin(); it != callbacks.end();) {
                if (it->value <= semVal) {
                    it->fn();
                    it = callbacks.erase(it);
                } else {
                    ++it;
                }
            }
        }

        for (auto& [frameIdx, callbacks]: m_frameCallbacks) {
            if (frameIdx == currentFrameGlobalIdx) {
                for (auto it = callbacks.begin(); it != callbacks.end(); ++it) {
                    (*it)();
                }
                //! Each frame only 1 frame is processed
                m_frameCallbacks.erase(frameIdx);
                break;
            }
        }
    }

    void RHIDeferredExecutor::FlushAllDeferredCallbacks() {
        std::lock_guard<std::mutex> guard(m_mutex);

        for (auto &[_, callbacks] : m_callbacks) {
            for (auto it = callbacks.begin(); it != callbacks.end(); ++it) {
                it->fn();
            }
        }
        m_callbacks.clear();

        for (auto& callback: m_endOfSessionCallbacks) {
            callback();
        }
        m_endOfSessionCallbacks.clear();

        for (auto& [frameIdx, callbacks]: m_frameCallbacks) {
            for (auto it = callbacks.begin(); it != callbacks.end(); ++it) {
                (*it)();
            }
        }
        m_frameCallbacks.clear();
    }
} // Shift
