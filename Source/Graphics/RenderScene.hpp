//
// Created by otrush on 8/30/2026.
//

#ifndef SHIFT_RENDERSCENE_HPP
#define SHIFT_RENDERSCENE_HPP

#include <vector>

#include <glm/glm.hpp>

#include "DrawItem.hpp"
#include "Graphics/Managers/MeshManager.hpp"
#include "Graphics/Shared/GPUShared.h"

namespace Shift {
    class ShiftScene;
}

namespace Shift::Graphics {

    //! fro UI
    struct RenderSceneStats {
        uint32_t objects = 0;
        uint32_t drawCalls = 0;
        uint32_t instances = 0;
        //! Later I guess
        uint32_t culled = 0;
    };

    //! Manage the sorted object data and draw items for bindless as well. Later will manage the pass bits?:D
    class RenderScene {
    public:
        //! The only reader of the scene
        void Extract(const ShiftScene& scene, const MeshManager& meshes, PipelineHandle forwardPipeline);

        //! One entry per node with a MeshProperty, in extraction order
        [[nodiscard]] const std::vector<GPU::ObjectData>& GetObjects() const { return m_objects; }

        //! Sorted by DrawItem::sortKey
        [[nodiscard]] const std::vector<DrawItem>& GetDrawItems() const { return m_drawItems; }

        //! Get the isntance index list, only low 24 bits are index, high 8 are flags
        //! Wicked engine uses similar approach and I like it for now
        [[nodiscard]] const std::vector<uint32_t>& GetSubmeshInstances() const { return m_submeshInstances; }

        [[nodiscard]] const RenderSceneStats& GetStats() const { return m_stats; }

    private:
        std::vector<GPU::ObjectData> m_objects;
        std::vector<DrawItem> m_drawItems;
        std::vector<uint32_t> m_submeshInstances;
        RenderSceneStats m_stats;
    };

    //! An ObjectData index has to fit
    static_assert(Conf::MAX_SCENE_OBJECTS - 1u <= GPU::SUBMESH_INSTANCE_OBJECT_MASK, "MAX_SCENE_OBJECTS no longer fits the submesh instance entry's index bits");
}

#endif //SHIFT_RENDERSCENE_HPP
