//
// Created by otrush on 3/2/2024.
//

#include "BufferManager.hpp"

namespace shift::gfx {
    void BufferManager::AllocateUBO(shift::SGUID id, VkDeviceSize size) {
        for (auto& buffPtr: m_uboStorage[id]) {
            buffPtr = std::make_unique<UniformBuffer>(m_device, size);
            if (!buffPtr->IsValid()) {
                spdlog::warn("BufferManager: Failed to allocate UBO!");
            }
        }
    }

} // shift::gfx