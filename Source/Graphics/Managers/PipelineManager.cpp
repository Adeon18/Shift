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

        return m_pool.Insert(pipeline, std::move(stages));
    }

    Pipeline* PipelineManager::Get(PipelineHandle handle) const {
        return m_pool.Get(handle);
    }

    void PipelineManager::DestroyPipeline(PipelineHandle handle) {
        std::vector<ShaderStageDesc>* stages = m_pool.GetMeta(handle);
        if (!stages) { return; }

        //! Stop hot-reload from rebuilding a pipeline we are about to free
        Pipeline* pipeline = m_pool.Get(handle);
        for (const ShaderStageDesc& stage : *stages) {
            m_shaderManager->UnregisterPipeline(stage.handle, pipeline);
        }


        //! Defer the GPU-side delete until the GPU is done with in-flight work using it
        Pipeline* retired = m_pool.Release(handle);
        auto payload = m_rhi->GetGraphicsWaitPayload();
        m_rhi->DeferExecute(payload.semaphore, payload.value, [retired]() { delete retired; });
    }

    uint32_t PipelineManager::HotReload() {
        //! Recompile dirty shaders and collect affected pipelines
        std::unordered_set<Pipeline*> toRebuild = m_shaderManager->HotReload();
        if (toRebuild.empty()) { return 0; }

        //! Every rebuild retires its old GPU handle against the same in-flight graphics work, so
        //! one wait payload gates them all
        auto payload = m_rhi->GetGraphicsWaitPayload();
        for (Pipeline* pipeline : toRebuild) {
            pipeline->Rebuild(false);
            m_rhi->DeferExecute(payload.semaphore, payload.value, [pipeline]() { pipeline->ReleaseRetired(); });
        }
        return static_cast<uint32_t>(toRebuild.size());
    }

    void PipelineManager::Destroy() {
        //! GPU is idle at shutdown, so owned pipelines are deleted directly.
        m_pool.ForEachLive([](uint32_t, Pipeline* pipeline) { delete pipeline; });
        m_pool.Clear();
    }
}
