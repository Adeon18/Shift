//
// Created by otrush on 10/21/2024.
//

#ifndef SHIFT_SWAPCHAIN_HPP
#define SHIFT_SWAPCHAIN_HPP

#include <concepts>
#include <type_traits>

#include "Base.hpp"
#include "Types.hpp"
#include "TextureFormat.hpp"

namespace Shift {
    //! The interface for a swapchain, has a lot of input variables, might be simpler for older APIs
    //! Not trivially-destructible!
    //! \tparam Swapchain
    template<typename Swapchain>
    concept ISwapchain =
        std::is_default_constructible_v<Swapchain> &&
    requires(Swapchain InputSwapchain, const Semaphore& InputSemaphore, uint64_t timeout, uint32_t width, uint32_t height, uint32_t imageIdx, bool* isOld, bool* wasChanged) {
        { InputSwapchain.Destroy() } -> std::same_as<void>;
        { InputSwapchain.AquireNextImage(InputSemaphore, wasChanged, timeout) } -> std::same_as<uint32_t>;
        { InputSwapchain.Recreate(width, height) } -> std::same_as<bool>;
        { InputSwapchain.Present(InputSemaphore, imageIdx, isOld) } -> std::same_as<bool>;
        { InputSwapchain.GetExtent() } -> std::same_as<Extent2D>;
        { InputSwapchain.GetViewport() } -> std::same_as<Viewport>;
        { InputSwapchain.GetFormat() } -> std::same_as<ETextureFormat>;
        { InputSwapchain.GetScissor() } -> std::same_as<Rect2D>;
        { InputSwapchain.IsValid() } -> std::same_as<bool>;
        { InputSwapchain.Get() };
    };
} // Shift

#endif //SHIFT_SWAPCHAIN_HPP
