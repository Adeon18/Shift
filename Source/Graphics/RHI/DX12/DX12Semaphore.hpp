#ifndef SHIFT_DX12SEMAPHORE_HPP
#define SHIFT_DX12SEMAPHORE_HPP

#include <cstdint>

#include "DX12Device.hpp"
#include "../Common/Semaphore.hpp"

namespace Shift::DX12 {
    class TimelineSemaphore {
    public:
        TimelineSemaphore(const Device* device, uint64_t initialValue);

        [[nodiscard]] bool IsValid() const;
        [[nodiscard]] uint64_t GetCurrentValue() const;
        void SetDebugName(const char* name) const;
        void Wait(uint64_t value);

        ~TimelineSemaphore();
    };

    ASSERT_INTERFACE(ITimelineSemaphore, TimelineSemaphore);
} // Shift::DX12

#endif //SHIFT_DX12SEMAPHORE_HPP
