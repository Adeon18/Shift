//
// Created by otrush on 6/29/2026.
//

#ifndef SHIFT_GENERATIONALPOOL_HPP
#define SHIFT_GENERATIONALPOOL_HPP

#include <cstdint>
#include <utility>
#include <variant>
#include <vector>

namespace Shift::Graphics {

    //! Reusable generational slot pool used for texture/buffer/pso managers, etc.
    //!
    //! The engine never holds a resource pointer directly. It holds an opaque Handle{slotIdx,
    //! generation} and the pool single-owns the resource in a slot. The generation counter turns a
    //! use-after-free into a *detectable* stale handle: once a slot is released its generation is
    //! bumped, so every referencing handle to it stops gets nullptr on Get
    //! Ownership is RAW, every GPU-resource free is timeline-deferred.
    //! Meta carries per-resource slot data
    template <typename T, typename Meta = std::monostate>
    class GenerationalPool {
    public:
        //! Opaque, generation-checked reference into the pool. Distinct nested type per pool
        //! instantiation, so a TextureHandle can never be passed where a PipelineHandle is expected.
        struct Handle {
            static constexpr uint32_t INVALID_IDX = UINT32_MAX;

            uint32_t slotIdx = INVALID_IDX;
            uint32_t generation = INVALID_IDX;
            bool operator==(const Handle& o) const noexcept = default;
        };

        //! Take ownership of resource in a slot and return a stable handle.
        //! Reuses a recycled slot when available, otherwise grows the pool.
        Handle Insert(T* resource, Meta meta = {}) {
            uint32_t idx;
            if (!m_freeSlots.empty()) {
                idx = m_freeSlots.back();
                m_freeSlots.pop_back();
            } else {
                idx = static_cast<uint32_t>(m_slots.size());
                m_slots.emplace_back();
            }

            Slot& slot = m_slots[idx];
            slot.resource = resource;
            slot.meta = std::move(meta);
            slot.alive = true;
            return { idx, slot.generation };
        }

        //! Resolve a handle to the owned resource, or nullptr if the handle is stale.
        [[nodiscard]] T* Get(Handle handle) const {
            const Slot* slot = ResolveSlot(handle);
            return slot ? slot->resource : nullptr;
        }

        //! Resolve a handle to its mutable slot metadata, or nullptr if the handle is stale.
        [[nodiscard]] Meta* GetMeta(Handle handle) {
            Slot* slot = ResolveSlot(handle);
            return slot ? &slot->meta : nullptr;
        }

        [[nodiscard]] bool IsValid(Handle handle) const {
            return ResolveSlot(handle) != nullptr;
        }

        //! Mark the slot dead, bump its generation, invalidating every other refeerence to it
        [[nodiscard]] T* Release(Handle handle) {
            Slot* slot = ResolveSlot(handle);
            if (!slot) { return nullptr; }

            T* released = slot->resource;
            slot->resource = nullptr;
            slot->meta = Meta{};
            slot->alive = false;
            ++slot->generation;
            m_freeSlots.push_back(handle.slotIdx);
            return released;
        }

        //! Call a function on the entire pooled data
        template <typename Fn>
        void ForEachLive(Fn&& fn) {
            for (uint32_t i = 0; i < m_slots.size(); ++i) {
                Slot& slot = m_slots[i];
                if (slot.alive) { fn(i, slot.resource); }
            }
        }

        //! same as foreachlive but with slot metadata
        template <typename Fn>
        void ForEachLiveMeta(Fn&& fn) {
            for (uint32_t i = 0; i < m_slots.size(); ++i) {
                Slot& slot = m_slots[i];
                if (slot.alive) { fn(i, slot.resource, slot.meta); }
            }
        }

        //! Reset all bookkeeping. Does NOT delete the stored T*, the owner should have DONE IT BY NOW
        void Clear() {
            m_slots.clear();
            m_freeSlots.clear();
        }

    private:
        struct Slot {
            T* resource = nullptr;
            Meta meta{};
            uint32_t generation = 0;
            bool alive = false;
        };

        [[nodiscard]] Slot* ResolveSlot(Handle handle) {
            if (handle.slotIdx >= m_slots.size()) { return nullptr; }
            Slot& slot = m_slots[handle.slotIdx];
            if (!slot.alive || slot.generation != handle.generation) { return nullptr; }
            return &slot;
        }

        [[nodiscard]] const Slot* ResolveSlot(Handle handle) const {
            if (handle.slotIdx >= m_slots.size()) { return nullptr; }
            const Slot& slot = m_slots[handle.slotIdx];
            if (!slot.alive || slot.generation != handle.generation) { return nullptr; }
            return &slot;
        }

        std::vector<Slot> m_slots;
        std::vector<uint32_t> m_freeSlots;
    };
}

#endif //SHIFT_GENERATIONALPOOL_HPP
