//
// Created by otrush on 10/8/2026.
//

#ifndef SHIFT_SELECTIONBOXSYSTEM_HPP
#define SHIFT_SELECTIONBOXSYSTEM_HPP

#include <span>
#include <vector>

#include <glm/glm.hpp>

#include "Graphics/RHI/RHI.hpp"
#include "Graphics/Managers/PipelineManager.hpp"
#include "Graphics/Shared/GPUShared.h"
#include "Scene/NodeID.hpp"

namespace Shift {
    class ShiftScene;
}

namespace Shift::Graphics {
    class MeshManager;

    //! One world-space AABB. Axis aligned is what lets the shader skip a model matrix entirely
    struct SelectionBox {
        glm::vec3 min{0.0f};
        glm::vec3 max{0.0f};
    };

    struct SelectionBoxInputs {
        uint64_t frameConstantsRef = 0;
        glm::vec4 color{1.0f, 0.55f, 0.15f, 1.0f};
        float bracketFraction = 0.2f;
        Texture& output;
    };

    class SelectionBoxSystem {
    public:
        void Init(PipelineManager& pipelineManager, ETextureFormat outputFormat);

        [[nodiscard]] bool Record(RenderContextEncoder& encoder, const SelectionBoxInputs& inputs) const;

        [[nodiscard]] PipelineHandle GetPipeline() const { return m_pipeline; }

        //! CPU half: ONE box per selected root, each the world AABB of its own subtree. A root with no
        //! mesh under it gets no box, so the list can be shorter than the selection
        void Extract(const ShiftScene& scene, const MeshManager& meshes, std::span<const NodeID> selection);

        [[nodiscard]] const std::vector<SelectionBox>& GetBoxes() const { return m_boxes; }

    private:
        PipelineManager* m_pipelineManager = nullptr;
        PipelineHandle m_pipeline;
        //! Reused across frames, so a selection costs no allocation after the first
        std::vector<SelectionBox> m_boxes;
    };
}

#endif //SHIFT_SELECTIONBOXSYSTEM_HPP
