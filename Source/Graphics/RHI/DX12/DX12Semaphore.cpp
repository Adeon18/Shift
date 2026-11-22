#include "DX12Semaphore.hpp"

#include "../Common/NotImplemented.hpp"

namespace Shift::DX12 {
    TimelineSemaphore::TimelineSemaphore(const Device* device, uint64_t initialValue) { SRHI_NOT_IMPLEMENTED(); }

    bool TimelineSemaphore::IsValid() const { SRHI_NOT_IMPLEMENTED(); }
    uint64_t TimelineSemaphore::GetCurrentValue() const { SRHI_NOT_IMPLEMENTED(); }
    void TimelineSemaphore::SetDebugName(const char* name) const { SRHI_NOT_IMPLEMENTED(); }
    void TimelineSemaphore::Wait(uint64_t value) { SRHI_NOT_IMPLEMENTED(); }

    TimelineSemaphore::~TimelineSemaphore() = default;
} // Shift::DX12
