#include "DX12Swapchain.hpp"

#include "../Common/NotImplemented.hpp"

namespace Shift::DX12 {
    bool Swapchain::Recreate(uint32_t width, uint32_t height) { SRHI_NOT_IMPLEMENTED(); }
    void* Swapchain::Get() const { SRHI_NOT_IMPLEMENTED(); }
    Texture& Swapchain::GetSwapchainTexture(uint32_t imageIdx) const { SRHI_NOT_IMPLEMENTED(); }
    Extent2D Swapchain::GetExtent() const { SRHI_NOT_IMPLEMENTED(); }
    ETextureFormat Swapchain::GetFormat() const { SRHI_NOT_IMPLEMENTED(); }
    Viewport Swapchain::GetViewport() const { SRHI_NOT_IMPLEMENTED(); }
    Rect2D Swapchain::GetScissor() const { SRHI_NOT_IMPLEMENTED(); }

    Swapchain::~Swapchain() = default;
} // Shift::DX12
