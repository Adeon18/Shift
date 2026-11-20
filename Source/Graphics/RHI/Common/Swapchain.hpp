//
// Created by otrush on 10/21/2024.
//

#ifndef SHIFT_SWAPCHAIN_HPP
#define SHIFT_SWAPCHAIN_HPP

#include <concepts>
#include <cstdint>
#include <type_traits>

#include "Base.hpp"
#include "Types.hpp"
#include "TextureFormat.hpp"

namespace Shift {
    //! The swapchain image a frame writes. Valid from acquire until presenta, should NOT BE CACHED!!!!
    //! Index is the imaghe index, not all backends use this
    struct FrameImage {
        Texture* texture = nullptr;
        uint32_t index = UINT32_MAX;
    };

    enum class EAcquireResult {
        Ok,
        NeedsResize,
        Failed
    };

    //! Acquire/present are per-backend so they are not here.
    //! Not trivially-destructible!
    //! \tparam Swapchain
    template<typename Swapchain>
    concept ISwapchain =
        std::constructible_from<Swapchain, const Device *, const WindowSurface*, uint32_t /*width*/, uint32_t /*height*/> &&
        !std::is_trivially_destructible_v<Swapchain> &&
    requires(Swapchain InputSwapchain, uint32_t width, uint32_t height, uint32_t imageIdx) {
        { InputSwapchain.Recreate(width, height) } -> std::same_as<bool>;
        { InputSwapchain.GetExtent() } -> std::same_as<Extent2D>;
        { InputSwapchain.GetViewport() } -> std::same_as<Viewport>;
        { InputSwapchain.GetFormat() } -> std::same_as<ETextureFormat>;
        { InputSwapchain.GetScissor() } -> std::same_as<Rect2D>;
        { InputSwapchain.GetSwapchainTexture(imageIdx) } -> std::same_as<Texture&>;
        { InputSwapchain.Get() };
    };
} // Shift

#endif //SHIFT_SWAPCHAIN_HPP
