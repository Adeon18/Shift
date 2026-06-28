//
// Created by otrush on 6/28/2026.
//
#include "PipelineManager.hpp"

namespace Shift::Graphics {
    void PipelineManager::Init(RenderBackend* rhi, ShaderManager* shaderManager) {
        m_rhi = rhi;
        m_backend = rhi->CreateInterface();
        m_shaderManager = shaderManager;
    }

    PipelineHandle PipelineManager::CreatePipeline(const PipelineDescriptor& desc, std::span<const ShaderDescriptor> shaderSources) {
        //! Compile/fetch each shader module
        std::vector<ShaderStageDesc> stages;
        stages.reserve(shaderSources.size());
        for (const ShaderDescriptor& source : shaderSources) {
            stages.push_back({source.type, m_shaderManager->GetShader(source)});
        }

        //! Create RHI pipeline
        Pipeline* pipeline = m_backend->CreatePipeline(desc, stages);

        //! Subscribe for hot-reload
        for (const ShaderStageDesc& stage : stages) {
            m_shaderManager->RegisterPipeline(stage.handle, pipeline);
        }

        //! Take ownership in a slot
        uint32_t idx;
        if (!m_freeSlots.empty()) {
            idx = m_freeSlots.back();
            m_freeSlots.pop_back();
        } else {
            idx = static_cast<uint32_t>(m_slots.size());
            m_slots.emplace_back();
        }

        PipelineSlot& slot = m_slots[idx];
        slot.pipeline = pipeline;
        slot.stages = std::move(stages);
        slot.alive = true;
        return { idx, slot.generation };
    }

    Pipeline* PipelineManager::Get(PipelineHandle handle) const {
        if (handle.slotIdx >= m_slots.size()) { return nullptr; }
        const PipelineSlot& slot = m_slots[handle.slotIdx];
        if (!slot.alive || slot.generation != handle.generation) { return nullptr; }
        //! No code resure from ResolveSlot as it is const
        return slot.pipeline;
    }

    PipelineManager::PipelineSlot* PipelineManager::ResolveSlot(PipelineHandle handle) {
        if (handle.slotIdx >= m_slots.size()) { return nullptr; }
        PipelineSlot& slot = m_slots[handle.slotIdx];
        if (!slot.alive || slot.generation != handle.generation) { return nullptr; }
        return &slot;
    }

    void PipelineManager::DestroyPipeline(PipelineHandle handle) {
        PipelineSlot* slot = ResolveSlot(handle);
        if (!slot) { return; }

        //! Stop hot-reload from rebuilding a pipeline we are about to free
        for (const ShaderStageDesc& stage : slot->stages) {
            m_shaderManager->UnregisterPipeline(stage.handle, slot->pipeline);
        }

        //! Defer the GPU-side delete until the GPU is done with in-flight work using it
        Pipeline* retired = slot->pipeline;
        auto payload = m_rhi->GetGraphicsWaitPayload();
        m_rhi->DeferExecute(payload.semaphore, payload.value, [retired]() { delete retired; });

        //! Invalidate every outstanding handle to this slot and recycle it.
        slot->pipeline = nullptr;
        slot->stages.clear();
        slot->alive = false;
        ++slot->generation;
        m_freeSlots.push_back(handle.slotIdx);
    }

    void PipelineManager::HotReload() {
        //! Recompile dirty shaders and collect affected pipelines
        std::unordered_set<Pipeline*> toRebuild = m_shaderManager->HotReload();
        if (toRebuild.empty()) { return; }

        //! Every rebuild retires its old GPU handle against the same in-flight graphics work, so
        //! one wait payload gates them all
        auto payload = m_rhi->GetGraphicsWaitPayload();
        for (Pipeline* pipeline : toRebuild) {
            //! TODO ? Maybe move the retired storage from the pipeline to this manager?
            pipeline->Rebuild(false);
            m_rhi->DeferExecute(payload.semaphore, payload.value, [pipeline]() { pipeline->ReleaseRetired(); });
        }
    }

    void PipelineManager::Destroy() {
        //! GPU is idle at shutdown, so owned pipelines are deleted directly.
        for (PipelineSlot& slot : m_slots) {
            if (slot.alive) {
                delete slot.pipeline;
                slot.pipeline = nullptr;
                slot.alive = false;
            }
        }
        m_slots.clear();
        m_freeSlots.clear();
    }
}
