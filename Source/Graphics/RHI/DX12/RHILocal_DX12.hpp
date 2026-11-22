#ifndef SHIFT_RHILOCAL_DX12_HPP
#define SHIFT_RHILOCAL_DX12_HPP

#include <cstdint>

#include "DX12RHI_Impl.hpp"
#include "../Common/Capabilities.hpp"
#include "../Common/Memory.hpp"
#include "../Common/NativeWindow.hpp"
#include "../Common/NotImplemented.hpp"

namespace Shift {
    //! device and swapchain are named by the agnostic RHI, the rest is the backend's
    template<> struct RHILocal<RHI::DX12> {
        RHI::UniquePtr<DX12::Device> device;
        RHI::UniquePtr<DX12::Swapchain> swapchain;

        bool InitBackend(const NativeWindow& window, uint32_t width, uint32_t height, const RHIAppInfo& appInfo, const RHIBindlessCapacity& bindless, const RHIRequiredFeatures& requiredFeatures) { SRHI_NOT_IMPLEMENTED(); }
        bool RecreateSwapchain(uint32_t width, uint32_t height) { SRHI_NOT_IMPLEMENTED(); }
        void DestroySwapchain() { SRHI_NOT_IMPLEMENTED(); }
        void DestroyBackend() { SRHI_NOT_IMPLEMENTED(); }
        void CancelPendingAcquires(const DX12::Texture* texture) { SRHI_NOT_IMPLEMENTED(); }
    };
} // Shift

#endif //SHIFT_RHILOCAL_DX12_HPP
