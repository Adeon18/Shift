//
// Created by otrush on 8/30/2026.
//

#ifndef SHIFT_DRAWITEM_HPP
#define SHIFT_DRAWITEM_HPP

#include <cstdint>

#include "Graphics/Managers/PipelineManager.hpp"

namespace Shift::Graphics {
    enum class EPassBit : uint32_t {
        None        = 0u,
        Forward     = 1u << 0,
        DepthOnly   = 1u << 1,
        //! We will sort transparency separately
        Transparent = 1u << 2,
    };

    [[nodiscard]] constexpr uint32_t operator|(EPassBit lhs, EPassBit rhs) {
        return static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs);
    }

    [[nodiscard]] constexpr uint32_t operator|(uint32_t lhs, EPassBit rhs) {
        return lhs | static_cast<uint32_t>(rhs);
    }

    [[nodiscard]] constexpr bool HasPass(uint32_t passMask, EPassBit bit) {
        return (passMask & static_cast<uint32_t>(bit)) != 0u;
    }

    struct DrawItem {
        uint32_t passMask = 0u;

        PipelineHandle pipeline{};
        //! mesh.indexRange.first + submesh.firstIndex
        uint32_t firstIndex = 0u;
        uint32_t indexCount = 0u;
        uint32_t objectIndex = 0u;
        //! TODO: 1 for now
        uint32_t instanceCount = 1u;
        uint32_t materialIndex = 0u;
        uint32_t meshVertexBase = 0u;
        //! Sort from least to most signficant changes, so pipeline -> material
        uint64_t sortKey = 0u;
    };

    //! More expensive chnages to more significant bits
    [[nodiscard]] constexpr uint64_t MakeDrawSortKey(uint32_t pipelineSlot, uint32_t meshSlot,
                                                     uint32_t submeshIndex, uint32_t materialIndex) {
        return (static_cast<uint64_t>(pipelineSlot & 0xFFFFu)   << 48)
             | (static_cast<uint64_t>(meshSlot     & 0xFFFFFFu) << 24)
             | (static_cast<uint64_t>(submeshIndex & 0xFFFu)    << 12)
             | (static_cast<uint64_t>(materialIndex & 0xFFFu));
    }
}

#endif //SHIFT_DRAWITEM_HPP
