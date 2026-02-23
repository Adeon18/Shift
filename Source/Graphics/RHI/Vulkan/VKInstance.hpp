//
// Created by otrush on 1/25/2024.
//

#ifndef SHIFT_VKINSTANCE_HPP
#define SHIFT_VKINSTANCE_HPP

#include <string>

#include "GLFW/glfw3.h"

#include "VKMacros.hpp"

#include "../Common/Capabilities.hpp"

namespace Shift::VK {
    class Instance {
    public:
        //! Initialize a VkInstance from the App Data
        //! \param appName The name of the application
        //! \param appVersion The application version
        //! \param engName The name of the Engine (Always Shift)
        //! \param engVersion The engine version
        Instance(const std::string& appName, uint32_t appVersion, const std::string& engName, uint32_t engVersion, const RHIRequiredFeatures& required);
        Instance(const Instance&)=delete;
        Instance& operator=(const Instance&)=delete;

        //! Returns a Vk Instance handle
        //! \return VkInstance
        [[nodiscard]] VkInstance Get() const { return m_instance; }

        [[nodiscard]] bool IsValid() const { return m_valid; }

        VkResult WaitForPresent(VkDevice dev, VkSwapchainKHR swapchain, uint64_t waitId, uint64_t timeout) const;

        //! Free the instance, should be done last
        ~Instance();
    private:
        bool SetupDebugMessenger();

        //! TODO: volk
        PFN_vkWaitForPresentKHR m_fpWaitForPresentKHR = nullptr;

        VkInstance m_instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

        bool m_valid = false;
    };
} // Shift::VK

#endif //SHIFT_VKINSTANCE_HPP
