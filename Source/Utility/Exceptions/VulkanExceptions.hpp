#ifndef SHIFT_VULKANEXCEPTIONS_HPP
#define SHIFT_VULKANEXCEPTIONS_HPP

#include <exception>
#include <string>

namespace shift {
    class VulkanCreateResourceException: public std::exception {
    public:
        VulkanCreateResourceException(std::string message): m_message{std::move(message)} {}

        [[nodiscard]] const char *what() const noexcept override {
            return m_message.c_str();
        }
    private:
        std::string m_message;
    };
} // shift

#endif //SHIFT_VULKANEXCEPTIONS_HPP
