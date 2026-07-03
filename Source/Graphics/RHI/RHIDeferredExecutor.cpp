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
        m_callbacks[timeline].emplace_back(value, std::move(fn));
    }

    void RHIDeferredExecutor::DeferExecuteToFrame(uint64_t frameIdx, Callback fn) {
        if (!fn) {
            Log(Warning, "No function passed to RHIDeferredExecutor!");
            return;
        }

        std::lock_guard<std::mutex> guard(m_mutex);
        m_frameCallbacks[frameIdx].emplace_back(std::move(fn));
    }

    void RHIDeferredExecutor::DeferExecuteEndOfSession(Callback fn) {
        std::lock_guard<std::mutex> guard(m_mutex);
        m_endOfSessionCallbacks.emplace_back(std::move(fn));
    }

    void RHIDeferredExecutor::ProcessDeferredCallbacks(uint64_t currentFrameGlobalIdx) {
        //! Collect under the lock, execute outside, to avoid dedlock when callback defers the call again
        std::vector<Callback> due;
        {
            std::lock_guard<std::mutex> guard(m_mutex);

            for (auto &[sem, callbacks] : m_callbacks) {
                uint64_t semVal = sem->GetCurrentValue();

                for (auto it = callbacks.begin(); it != callbacks.end();) {
                    if (it->value <= semVal) {
                        due.push_back(std::move(it->fn));
                        it = callbacks.erase(it);
                    } else {
                        ++it;
                    }
                }
            }

            //! Each frame only 1 frame bucket is processed
            if (auto it = m_frameCallbacks.find(currentFrameGlobalIdx); it != m_frameCallbacks.end()) {
                for (auto& fn: it->second) {
                    due.push_back(std::move(fn));
                }
                m_frameCallbacks.erase(it);
            }
        }

        for (auto& fn: due) {
            fn();
        }
    }

    void RHIDeferredExecutor::FlushAllDeferredCallbacks() {
        //! Execute all defer's of defer's of defer's till nothing is left
        while (true) {
            std::vector<Callback> due;
            {
                std::lock_guard<std::mutex> guard(m_mutex);

                for (auto &[_, callbacks] : m_callbacks) {
                    for (auto& pending: callbacks) {
                        due.push_back(std::move(pending.fn));
                    }
                }
                m_callbacks.clear();

                for (auto& callback: m_endOfSessionCallbacks) {
                    due.push_back(std::move(callback));
                }
                m_endOfSessionCallbacks.clear();

                for (auto& [frameIdx, callbacks]: m_frameCallbacks) {
                    for (auto& fn: callbacks) {
                        due.push_back(std::move(fn));
                    }
                }
                m_frameCallbacks.clear();
            }

            if (due.empty()) {
                return;
            }

            for (auto& fn: due) {
                fn();
            }
        }
    }
} // Shift
