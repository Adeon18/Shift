//
// Created by otrush on 11/14/2025.
//

#ifndef SHIFT_RHIDEFERREDEXECUTOR_HPP
#define SHIFT_RHIDEFERREDEXECUTOR_HPP
#include <mutex>

#include "RHIContext.hpp"

namespace Shift {
    //! Deferred executor for function tied to TimelineSemaphore. Useful for freeing RHI objects
    class RHIDeferredExecutor {
    public:
        RHIDeferredExecutor() = default;
        ~RHIDeferredExecutor() = default;
        using Callback = std::function<void()>;

        struct Pending {
            uint64_t value;
            Callback fn;
        };

        //! Defore execution tll semaphore signals a value, thread-safe
        //! @param timeline if nullptr - execute immediately
        //! @param value semaphore value to wit on
        //! @param fn
        void DeferExecute(TimelineSemaphore* timeline, uint64_t value, Callback fn);

        //! Defer execute function for end of the program, order is NOT guaranteed, but is thread-safe
        //! @param fn function to execute
        void DeferExecuteEndOfSession(Callback fn);

        //! Process all deferred callbacks and execute the ones that are ready
        void ProcessDeferredCallbacks();

        //! Flush all semaphore and end
        void FlushAllDeferredCallbacks();
    private:
        std::map<TimelineSemaphore*, std::vector<Pending>> m_callbacks;
        std::vector<Callback> m_endOfSessionCallbacks;
        std::mutex m_mutex;
    };
} // Shift

#endif //SHIFT_RHIDEFERREDEXECUTOR_HPP