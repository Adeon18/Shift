#include "VKDebugUtils.hpp"

namespace Shift::VK::Util {
    void SetDebugName(VkDevice device, VkObjectType type, uint64_t handle, const char* name) {
        if (!vkSetDebugUtilsObjectNameEXT || !name || name[0] == '\0' || handle == 0) { return; }

        VkDebugUtilsObjectNameInfoEXT info{};
        info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        info.objectType = type;
        info.objectHandle = handle;
        info.pObjectName = name;

        vkSetDebugUtilsObjectNameEXT(device, &info);
    }

    void CmdBeginDebugLabel(VkCommandBuffer cmd, const char* label) {
        if (!vkCmdBeginDebugUtilsLabelEXT || !label) { return; }

        VkDebugUtilsLabelEXT info{};
        info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        info.pLabelName = label;

        vkCmdBeginDebugUtilsLabelEXT(cmd, &info);
    }

    void CmdEndDebugLabel(VkCommandBuffer cmd) {
        if (!vkCmdEndDebugUtilsLabelEXT) { return; }
        vkCmdEndDebugUtilsLabelEXT(cmd);
    }

    void CmdInsertDebugLabel(VkCommandBuffer cmd, const char* label) {
        if (!vkCmdInsertDebugUtilsLabelEXT || !label) { return; }

        VkDebugUtilsLabelEXT info{};
        info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
        info.pLabelName = label;

        vkCmdInsertDebugUtilsLabelEXT(cmd, &info);
    }
} // Shift::VK::Util
