//
// Created by otrush on 8/11/2026.
//

#ifndef SHIFT_FRAMERINGBUFFER_HPP
#define SHIFT_FRAMERINGBUFFER_HPP

#include <cstring>
#include <string>
#include <utility>

#include "Config/EngineConfig.hpp"
#include "Graphics/Managers/BufferManager.hpp"
#include "Utility/Assertions.hpp"

namespace Shift::Graphics {

    //! Frame slots are padded up to this. Will work if we bind it directly too
    inline constexpr uint64_t FRAME_RING_SLOT_ALIGNMENT = 256;

    [[nodiscard]] constexpr uint64_t FrameRingSlotStride(uint64_t bytesPerSlot) {
        return ((bytesPerSlot + FRAME_RING_SLOT_ALIGNMENT - 1) / FRAME_RING_SLOT_ALIGNMENT) *
               FRAME_RING_SLOT_ALIGNMENT;
    }

    //! One host-visible, persistently-mapped, device-addressable buffer carved into one slot per
    //! frame in flight. The renderer rewrites the slot belonging to the frame it is recording and
    //! hands that slot's address to the shaders
    //!
    //! A slot may only be written after RHI::BeginFrame() has
    //! returned for that frame.
    //!
    //! Lifetime: the buffer belongs to the BufferManager that created it, like every other engine
    //! buffer, and dies with it. The ring holds no ownership and needs no teardown.
    template<typename T>
    class FrameRingBuffer {
    public:
        //! \param buffers the manager that will own the underlying buffer
        //! \param name debug name, shows up in validation messages and VMA leak reports
        //! \param elementsPerSlot array length per frame (1 for a single struct such as FrameConstants)
        [[nodiscard]] bool Init(BufferManager& buffers, std::string name, uint32_t elementsPerSlot = 1) {
            CheckCritical(elementsPerSlot > 0, "A frame ring needs at least one element per slot");

            m_elementsPerSlot = elementsPerSlot;
            m_slotStride = FrameRingSlotStride(sizeof(T) * elementsPerSlot);

            BufferDescriptor desc;
            desc.size = m_slotStride * Conf::SHIFT_MAX_FRAMES_IN_FLIGHT;
            desc.name = std::move(name);
            //! Uniform for now, and I think forewer??
            desc.type = EBufferType::Uniform;
            desc.isDeviceAddressable = true;

            m_handle = buffers.CreateBuffer(desc);
            Buffer* buffer = buffers.Get(m_handle);
            CheckCritical(buffer != nullptr, "Frame ring buffer handle did not resolve");
            CheckCritical(buffer->IsValid(), "Frame ring buffer allocation failed");

            m_mapped = static_cast<uint8_t*>(buffer->GetMapped());
            CheckCritical(m_mapped != nullptr,
                          "Frame ring buffer is not persistently mapped, so it cannot be written per frame");

            m_baseAddress = buffer->GetDeviceAddress();
            CheckCritical(m_baseAddress != 0,
                          "Frame ring buffer reported address 0, so no shader could reach it");

            std::memset(m_mapped, 0, desc.size);

            return true;
        }

        [[nodiscard]] bool IsValid() const { return m_mapped != nullptr; }

        [[nodiscard]] uint32_t GetElementsPerSlot() const { return m_elementsPerSlot; }

        //! The CPU write target for the given frame in flight
        [[nodiscard]] T* Slot(uint32_t frameSlot) {
            if (!InRange(frameSlot)) { return nullptr; }
            return reinterpret_cast<T*>(m_mapped + frameSlot * m_slotStride);
        }

        //! Const version of write Slot
        [[nodiscard]] const T* Slot(uint32_t frameSlot) const {
            if (!InRange(frameSlot)) { return nullptr; }
            return reinterpret_cast<const T*>(m_mapped + frameSlot * m_slotStride);
        }

        //! The GPUBufferRef this slot is reached by. Esentially BDA to pass to the shader
        [[nodiscard]] uint64_t SlotAddress(uint32_t frameSlot) const {
            if (!InRange(frameSlot)) { return 0; }
            return m_baseAddress + frameSlot * m_slotStride;
        }

    private:
        [[nodiscard]] bool InRange(uint32_t frameSlot) const {
            if (frameSlot < Conf::SHIFT_MAX_FRAMES_IN_FLIGHT) { return true; }
            Log(Error, "Frame ring slot {} is out of range (max frames in flight is {})",
                frameSlot, Conf::SHIFT_MAX_FRAMES_IN_FLIGHT);
            return false;
        }

        BufferHandle m_handle{};
        //! Persistemntly mapped
        uint8_t* m_mapped = nullptr;
        uint64_t m_baseAddress = 0;

        uint64_t m_slotStride = 0;
        uint32_t m_elementsPerSlot = 0;
    };
}

#endif //SHIFT_FRAMERINGBUFFER_HPP
