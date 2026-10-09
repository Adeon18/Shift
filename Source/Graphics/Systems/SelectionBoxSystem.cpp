//
// Created by otrush on 10/8/2026.
//

#include "SelectionBoxSystem.hpp"

#include <array>
#include <limits>
#include <optional>

#include "Graphics/Managers/MeshManager.hpp"
#include "Scene/ShiftScene.hpp"
#include "Utility/Assertions.hpp"
#include "Utility/UtilStandard.hpp"

namespace Shift::Graphics {

    namespace {
        void ExpandWorldAABB(const glm::mat4& transform, const Bounds& bounds, glm::vec3& lo, glm::vec3& hi) {
            for (uint32_t i = 0; i < 8; ++i) {
                const glm::vec3 corner{
                    (i & 1u) ? bounds.max.x : bounds.min.x,
                    (i & 2u) ? bounds.max.y : bounds.min.y,
                    (i & 4u) ? bounds.max.z : bounds.min.z
                };
                const glm::vec3 world = glm::vec3(transform * glm::vec4(corner, 1.0f));
                lo = glm::min(lo, world);
                hi = glm::max(hi, world);
            }
        }

        //! Overdesigned just in cases lol
        void AccumulateSubtree(const ShiftScene& scene, const MeshManager& meshes, NodeID node, glm::vec3& lo, glm::vec3& hi, bool& anyMesh) {
            if (const MeshProperty* meshProperty = scene.GetProperty<MeshProperty>(node)) {
                if (const Mesh* mesh = meshes.Get(meshProperty->mesh)) {
                    ExpandWorldAABB(scene.GetWorld(node), mesh->bounds, lo, hi);
                    anyMesh = true;
                }
            }
            for (const NodeID child : scene.GetChildren(node)) {
                AccumulateSubtree(scene, meshes, child, lo, hi, anyMesh);
            }
        }
    }

    void SelectionBoxSystem::Extract(const ShiftScene& scene, const MeshManager& meshes, std::span<const NodeID> selection) {
        m_boxes.clear();
        for (const NodeID root : selection) {
            if (!scene.IsValid(root)) { continue; }

            glm::vec3 lo{std::numeric_limits<float>::max()};
            glm::vec3 hi{std::numeric_limits<float>::lowest()};
            bool anyMesh = false;
            AccumulateSubtree(scene, meshes, root, lo, hi, anyMesh);

            if (anyMesh) { m_boxes.push_back({lo, hi}); }
        }
    }

    void SelectionBoxSystem::Init(PipelineManager& pipelineManager, ETextureFormat outputFormat) {
        m_pipelineManager = &pipelineManager;

        PipelineDescriptor descriptor;
        descriptor.name = "SelectionBox";

        ShaderDescriptor vsDescriptor;
        vsDescriptor.type = EShaderType::Vertex;
        vsDescriptor.path = Shift::Util::GetShiftShaderSrcDir() + "Editor/SelectionBox.slang";
        vsDescriptor.entry = "mainVS";
        ShaderDescriptor fsDescriptor;
        fsDescriptor.type = EShaderType::Fragment;
        fsDescriptor.path = Shift::Util::GetShiftShaderSrcDir() + "Editor/SelectionBox.slang";
        fsDescriptor.entry = "mainPS";

        descriptor.colorBlendConfig.attachments.push_back({.format = outputFormat});
        descriptor.topology = EPrimitiveTopology::LineList;
        descriptor.rasterizerStateDesc.cullMode = ECullMode::None;
        //! NOTE: would be nice to have this but it is a vk only feature afaik
        descriptor.rasterizerStateDesc.lineWidth = 1.0f;
        descriptor.pushConstants = PushConstantRange{
            .offset = 0,
            .size = static_cast<uint32_t>(sizeof(GPU::SelectionBoxPushConstants)),
            .stageFlags = EBindingVisibility::Vertex | EBindingVisibility::Fragment
        };
        std::array<ShaderDescriptor, 2> shaderSources{vsDescriptor, fsDescriptor};
        m_pipeline = m_pipelineManager->CreatePipeline(descriptor, shaderSources);
    }

    bool SelectionBoxSystem::Record(RenderContextEncoder& encoder, const SelectionBoxInputs& inputs) const {
        if (m_boxes.empty()) { return true; }

        Pipeline* pipeline = m_pipelineManager->Get(m_pipeline);
        CheckCritical(pipeline != nullptr, "The selection box pipeline does not resolve!");

        encoder.PushDebugGroup("SelectionBoxPass", {0.95f, 0.60f, 0.20f, 1.0f}, true);
        encoder.TransitionTexture(inputs.output, EResourceLayout::ColorAttachmentOptimal, EPipelineStageFlags::ColorAttachmentOutputBit);

        const uint32_t width = inputs.output.GetWidth();
        const uint32_t height = inputs.output.GetHeight();

        //! Load: the tonemapped image is already there and this draws over it
        RenderPassDescriptor pass;
        pass.colorAttachments.push_back({.loadOperation = EAttachmentLoadOperation::Load});
        pass.extent = {width, height};
        std::array targets{&inputs.output};
        encoder.BeginRenderPass(pass, targets, std::nullopt);

        encoder.SetScissor({{0, 0}, {width, height}});
        //! TODO: [REFACTOR] I should probably put this somewhere
        encoder.SetViewport({0.0f, static_cast<float>(height), static_cast<float>(width), -static_cast<float>(height), 0.0f, 1.0f});

        encoder.BindGraphicsPipeline(*pipeline);

        for (const SelectionBox& box : m_boxes) {
            const GPU::SelectionBoxPushConstants push{
                .frameConstantsRef = inputs.frameConstantsRef,
                .boxMinFraction = glm::vec4(box.min, inputs.bracketFraction),
                .boxMaxPad = glm::vec4(box.max, 0.0f),
                .color = inputs.color
            };
            encoder.SetPushConstants(*pipeline, &push, static_cast<uint32_t>(sizeof(push)));
            encoder.Draw({.vertexCount = GPU::SELECTION_BOX_VERTEX_COUNT});
        }

        encoder.EndRenderPass();

        encoder.TransitionTexture(inputs.output, EResourceLayout::ShaderReadOnlyOptimal, EPipelineStageFlags::FragmentShaderBit);
        encoder.PopDebugGroup();

        return true;
    }
}
