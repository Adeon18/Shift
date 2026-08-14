//
// Created by otrush on 8/14/2026.
//
#include "GlobalResourceSet.hpp"

#include "Config/EngineConfig.hpp"

namespace Shift::Graphics {

    const PipelineLayoutDescriptor& GlobalResourceSet::Layout() {
        static const PipelineLayoutDescriptor layout = [] {
            PipelineLayoutDescriptor desc;

            //! b0
            desc.bindings.push_back(PipelineLayoutDescriptor::LayoutBindingDesc{
                .binding = BINDING_SAMPLERS,
                .type = EBindingType::Sampler,
                .stageFlags = EBindingVisibility::Vertex |
                              EBindingVisibility::Geometry |
                              EBindingVisibility::Fragment |
                              EBindingVisibility::Compute,
                .count = Conf::MAX_BINDLESS_SAMPLERS,
                .isBindless = true,
                .updateAfterBind = true
            });

            //! b1
            desc.bindings.push_back(PipelineLayoutDescriptor::LayoutBindingDesc{
                .binding = BINDING_IMAGES_2D,
                .type = EBindingType::SampledImage,
                .stageFlags = EBindingVisibility::Vertex |
                              EBindingVisibility::Geometry |
                              EBindingVisibility::Fragment |
                              EBindingVisibility::Compute,
                .count = Conf::MAX_BINDLESS_IMAGES,
                .isBindless = true,
                .updateAfterBind = true
            });

            return desc;
        }();

        return layout;
    }

    bool GlobalResourceSet::Init(RenderBackendInterface* backend) {
        m_set = backend->CreateResourceSet(Layout());
        CheckCritical(m_set != nullptr, "Failed to create the global resource set!");
        CheckCritical(m_set->IsValid(), "The global resource set was allocated but is not valid!");

        return true;
    }

    void GlobalResourceSet::Destroy() {
        delete m_set;
        m_set = nullptr;
    }

    void GlobalResourceSet::WriteImage2D(uint32_t slot, const Texture& texture) {
        m_set->UpdateTexture(BINDING_IMAGES_2D, slot, texture);
    }

    void GlobalResourceSet::WriteSampler(uint32_t slot, const Sampler& sampler) {
        m_set->UpdateSampler(BINDING_SAMPLERS, slot, sampler);
    }

    void GlobalResourceSet::Apply() {
        m_set->Apply();
    }
}
