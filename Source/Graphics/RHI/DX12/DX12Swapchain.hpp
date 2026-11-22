#ifndef SHIFT_DX12SWAPCHAIN_HPP
#define SHIFT_DX12SWAPCHAIN_HPP

#include <cstdint>

#include "DX12Device.hpp"
#include "DX12Texture.hpp"
#include "../Common/Swapchain.hpp"

namespace Shift::DX12 {
    class Swapchain {
    public:
        [[nodiscard]] bool Recreate(uint32_t width, uint32_t height);
        [[nodiscard]] void* Get() const;
        [[nodiscard]] Texture& GetSwapchainTexture(uint32_t imageIdx) const;
        [[nodiscard]] Extent2D GetExtent() const;
        [[nodiscard]] ETextureFormat GetFormat() const;
        [[nodiscard]] Viewport GetViewport() const;
        [[nodiscard]] Rect2D GetScissor() const;

        ~Swapchain();
    };

    ASSERT_INTERFACE(ISwapchain, Swapchain);
} // Shift::DX12

#endif //SHIFT_DX12SWAPCHAIN_HPP
