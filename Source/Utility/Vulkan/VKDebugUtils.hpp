#ifndef SHIFT_VKDEBUGUTILS_HPP
#define SHIFT_VKDEBUGUTILS_HPP

#include "Utility/Vulkan/VKInclude.hpp"

namespace Shift::VK::Util {
    //! F-DEBUGMARKERS: thin wrappers over VK_EXT_debug_utils naming/labeling. Every function
    //! no-ops when the extension is absent (volk leaves the function pointers null )

    //! Attach a debug name to any Vulkan object
    void SetDebugName(VkDevice device, VkObjectType type, uint64_t handle, const char* name);

    //! Open/close a nested label region on a command buffer (must be paired, per command buffer)
    void CmdBeginDebugLabel(VkCommandBuffer cmd, const char* label);
    void CmdEndDebugLabel(VkCommandBuffer cmd);
    //! Drop a single point label into the command stream
    void CmdInsertDebugLabel(VkCommandBuffer cmd, const char* label);
} // Shift::VK::Util

#endif //SHIFT_VKDEBUGUTILS_HPP
