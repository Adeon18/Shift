//
// Created by otrush on 8/30/2026.
//

#ifndef SHIFT_RENDERSCENE_HPP
#define SHIFT_RENDERSCENE_HPP

#include <span>
#include <vector>

#include <glm/glm.hpp>

#include "DrawItem.hpp"
#include "Graphics/Managers/MeshManager.hpp"
#include "Graphics/Shared/GPUShared.h"

namespace Shift::Graphics {

    //! Kinda temp until scene container.
    struct MeshPlacement {
        MeshHandle mesh;
        glm::mat4 transform{1.0f};
    };

    //! fro UI
    struct RenderSceneStats {
        uint32_t placements = 0;
        uint32_t objects = 0;
        uint32_t drawCalls = 0;
        uint32_t instances = 0;
        //! Later I guess
        uint32_t culled = 0;
    };

    //! Manage the sorted object data and draw items for bindless as well. Later will manage the pass bits?:D
    class RenderScene {
    public:
        void Extract(std::span<const MeshPlacement> placements,
                     const MeshManager& meshes,
                     PipelineHandle forwardPipeline);

        //! One entry per placement, in placement order
        [[nodiscard]] const std::vector<GPU::ObjectData>& GetObjects() const { return m_objects; }

        //! Sorted by DrawItem::sortKey
        [[nodiscard]] const std::vector<DrawItem>& GetDrawItems() const { return m_drawItems; }

        [[nodiscard]] const RenderSceneStats& GetStats() const { return m_stats; }

    private:
        std::vector<GPU::ObjectData> m_objects;
        std::vector<DrawItem> m_drawItems;
        RenderSceneStats m_stats;
    };
}

#endif //SHIFT_RENDERSCENE_HPP
