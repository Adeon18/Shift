//
// Created by otrush on 1/25/2024.
//

#ifndef SHIFT_INSTANCE_HPP
#define SHIFT_INSTANCE_HPP

#include "GLFW/glfw3.h"

#include <string>

#include "Utility/Exceptions/VulkanExceptions.hpp"

namespace sft {
    namespace gfx {
        class Instance {
        public:
            Instance(std::string appName, uint32_t appVersion, std::string engName, uint32_t engVersion);
            Instance()=delete;
            Instance(const Instance&)=delete;
            Instance& operator=(const Instance&)=delete;

            [[nodiscard]] VkInstance Get() const { return m_instance; }

            ~Instance();
        private:
            void setupDebugMessenger();

            VkInstance m_instance;
            VkDebugUtilsMessengerEXT m_debugMessenger;
        };
    } // gfx
} // sft

#endif //SHIFT_INSTANCE_HPP
