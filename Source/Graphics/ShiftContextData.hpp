//
// Created by otrush on 3/3/2024.
//

#ifndef SHIFT_SHIFTCONTEXTDATA_HPP
#define SHIFT_SHIFTCONTEXTDATA_HPP

#include <memory>
#include "Graphics/Abstraction/Device/Device.hpp"
#include "Graphics/Abstraction/Device/Swapchain.hpp"
#include "Graphics/Abstraction/Device/WindowSurface.hpp"
#include "Graphics/Abstraction/CommandPool.hpp"

namespace Shift::gfx {
    struct ShiftBackBuffer {
        std::unique_ptr<Swapchain> swapchain;
        std::unique_ptr<WindowSurface> windowSurface;
    };

    struct ShiftContext {
        std::unique_ptr<Device> device;
        std::unique_ptr<Instance> instance;

        std::unique_ptr<CommandPool> graphicsPool;
        std::unique_ptr<CommandPool> transferPool;
    };
} // shift::gfx

#endif //SHIFT_SHIFTCONTEXTDATA_HPP
