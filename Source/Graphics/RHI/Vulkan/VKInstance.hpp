//
// Created by otrush on 1/25/2024.
//

#ifndef SHIFT_VKINSTANCE_HPP
#define SHIFT_VKINSTANCE_HPP

#include <string>

#include "GLFW/glfw3.h"

#include "VKMacros.hpp"

namespace Shift::VK {
    class Instance {
    public:
        Instance() = default;
        Instance(const Instance&)=delete;
        Instance& operator=(const Instance&)=delete;

        //! Initialize a VkInstance from the App Data
        //! \param appName The name of the application
        //! \param appVersion The application version
        //! \param engName The name of the Engine (Always Shift)
        //! \param engVersion The engine version
        //! \return True if initialization successful, false otherwise
        bool Init(const std::string& appName, uint32_t appVersion, const std::string& engName, uint32_t engVersion);

        //! Returns a Vk Instance handle
        //! \return VkInstance
        [[nodiscard]] VkInstance Get() const { return m_instance; }

        //! Call the ext function BeginRendering
        //! \param buff The command buffer
        //! \param info BeginRendering info struct
        void CallBeginRenderingExternal(VkCommandBuffer buff, VkRenderingInfoKHR info) const { vkCmdBeginRenderingKHR(buff, &info); }
        //! Call the ext function EndRendering
        //! \param buff the command buffer
        void CallEndRenderingExternal(VkCommandBuffer buff) const { vkCmdEndRenderingKHR(buff); }

        //! Free the instance, should be done last
        void Destroy();
        ~Instance() = default;
    private:
        bool SetupDebugMessenger();
        bool PollDynamicRenderingFunctions();

        VkInstance m_instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

        PFN_vkCmdBeginRenderingKHR vkCmdBeginRenderingKHR = VK_NULL_HANDLE;
        PFN_vkCmdEndRenderingKHR   vkCmdEndRenderingKHR = VK_NULL_HANDLE;
    };
} // Shift::VK

#endif //SHIFT_VKINSTANCE_HPP
