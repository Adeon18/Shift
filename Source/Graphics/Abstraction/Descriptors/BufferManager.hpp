//
// Created by otrush on 3/2/2024.
//

#ifndef SHIFT_BUFFERMANAGER_HPP
#define SHIFT_BUFFERMANAGER_HPP

#include <unordered_map>
#include <array>

#include "Graphics/Abstraction/Device/Device.hpp"
#include "Graphics/Abstraction/Buffers/BasicBuffers.hpp"

#include "Utility/GUIDGenerator/GUIDGenerator.hpp"

namespace shift::gfx {
    //! A manager designed to create and store buffers, handling per frame in flight data and storing them by UUID
    class BufferManager {
    public:
        BufferManager(const Device& device): m_device{device} {}

        //! Allocate UBO of certain size for all FIF and store by ID
        void AllocateUBO(SGUID id, VkDeviceSize size);

        UniformBuffer& GetUBO(SGUID id, uint32_t frameIdx) { return *m_uboStorage[id][frameIdx]; }

        ~BufferManager() = default;

        BufferManager() = delete;
        BufferManager(const BufferManager&) = delete;
        BufferManager& operator=(const BufferManager&) = delete;

    private:
        const Device& m_device;

        std::unordered_map<SGUID, std::array<std::unique_ptr<UniformBuffer>, gutil::SHIFT_MAX_FRAMES_IN_FLIGHT>> m_uboStorage;
    };
} // shift::gfx


#endif //SHIFT_BUFFERMANAGER_HPP
