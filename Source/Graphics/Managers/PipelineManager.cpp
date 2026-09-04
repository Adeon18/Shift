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

    std::vector<ShaderStageDesc> PipelineManager::ResolveSources(std::span<const ShaderDescriptor> sources,
                                                                 bool& outAnyFallback) {
        std::vector<ShaderStageDesc> assets;
        assets.reserve(sources.size());
        outAnyFallback = false;

        for (const ShaderDescriptor& source : sources) {
            const ShaderResolution resolution = m_shaderManager->GetShader(source);
            if (!resolution.shader) {
                //! Nothing: die
                Log(Error, "Shader stage {} ({}) resolved to nothing", source.path, source.entry);
                return {};
            }
            outAnyFallback = outAnyFallback || resolution.isFallback;
            assets.push_back({source.type, resolution.shader});
        }

        return assets;
    }

    std::vector<ShaderStageDesc> PipelineManager::ResolveStages(std::span<const ShaderStageDesc> assets,
                                                                bool anyFallback) {
        if (!anyFallback) { return {assets.begin(), assets.end()}; }

        std::vector<ShaderStageDesc> stages;
        stages.reserve(assets.size());

        //! Essentially if any stage is fallbacked - fallback all stages!
        for (const ShaderStageDesc& asset : assets) {
            Shader* fallback = m_shaderManager->GetFallbackShader(asset.type);
            if (!fallback) {
                Log(Error, "No fallback shader for stage {}", ShaderTypeToString(asset.type));
                return {};
            }
            stages.push_back({asset.type, fallback});
        }

        return stages;
    }

    PipelineHandle PipelineManager::CreatePipeline(const PipelineDescriptor& desc, std::span<const ShaderDescriptor> shaderSources) {
        //! Compile/fetch each shader module
        bool anyFallback = false;
        std::vector<ShaderStageDesc> assets = ResolveSources(shaderSources, anyFallback);
        const std::vector<ShaderStageDesc> stages = ResolveStages(assets, anyFallback);
        if (assets.empty() || stages.empty()) {
            Log(Error, "Pipeline '{}' was not created: a shader stage failed with no fallback available", desc.name);
            return {};
        }

        PipelineMeta meta;
        meta.sources.assign(shaderSources.begin(), shaderSources.end());
        meta.isFallback = anyFallback;

        if (meta.isFallback) {
            Log(Error, "Pipeline '{}' is running on FALLBACK shaders: every stage was replaced "
                       "because at least one failed to compile", desc.name);
        }

        //! Create RHI pipeline
        Pipeline* pipeline = m_backend->CreatePipeline(desc, stages);

        //! Subscribe against the true assets, not fallbacks so at chnage we can rebuild
        for (const ShaderStageDesc& asset : assets) {
            m_shaderManager->RegisterPipeline(asset.handle, pipeline);
        }
        meta.assets = std::move(assets);

        return m_pool.Insert(pipeline, std::move(meta));
    }

    bool PipelineManager::IsRunningFallback(PipelineHandle handle) {
        const PipelineMeta* meta = m_pool.GetMeta(handle);
        return meta && meta->isFallback;
    }

    Pipeline* PipelineManager::Get(PipelineHandle handle) const {
        return m_pool.Get(handle);
    }

    void PipelineManager::DestroyPipeline(PipelineHandle handle) {
        PipelineMeta* meta = m_pool.GetMeta(handle);
        if (!meta) { return; }

        //! Stop hot-reload from rebuilding a pipeline we are about to free
        Pipeline* pipeline = m_pool.Get(handle);
        for (const ShaderStageDesc& asset : meta->assets) {
            m_shaderManager->UnregisterPipeline(asset.handle, pipeline);
        }


        //! Defer the GPU-side delete until the GPU is done with in-flight work using it
        Pipeline* retired = m_pool.Release(handle);
        auto payload = m_rhi->GetGraphicsWaitPayload();
        m_rhi->DeferExecute(payload.semaphore, payload.value, [retired]() { delete retired; });
    }

    uint32_t PipelineManager::HotReload() {
        //! Recompile dirty shaders and collect affected pipelines
        const std::unordered_set<Pipeline*> toRebuild = m_shaderManager->HotReload();
        if (toRebuild.empty()) { return 0; }

        //! Every rebuild retires its old GPU handle against the same in-flight graphics work, so
        //! one wait payload gates them all
        auto payload = m_rhi->GetGraphicsWaitPayload();
        uint32_t rebuilt = 0;

        //! Check recompile changed shaders, check if any have fallbacks and apply the fallbacks for rebuild
        m_pool.ForEachLiveMeta([&](uint32_t, Pipeline* pipeline, PipelineMeta& meta) {
            if (!toRebuild.contains(pipeline)) { return; }

            bool anyFallback = false;
            const std::vector<ShaderStageDesc> assets = ResolveSources(meta.sources, anyFallback);
            const std::vector<ShaderStageDesc> stages = ResolveStages(assets, anyFallback);
            //! Keep the pipeline it already has rather than replacing it with nothing
            if (assets.empty() || stages.empty()) { return; }
            const bool wasFallback = meta.isFallback;
            meta.isFallback = anyFallback;
            if (wasFallback && !meta.isFallback) {
                Log(Info, "Pipeline '{}' recovered from fallback shaders", pipeline->GetDescriptor().name);
            }

            pipeline->Rebuild(false, stages);
            m_rhi->DeferExecute(payload.semaphore, payload.value, [pipeline]() { pipeline->ReleaseRetired(); });
            ++rebuilt;
        });

        return rebuilt;
    }

    void PipelineManager::Destroy() {
        //! GPU is idle at shutdown, so owned pipelines are deleted directly.
        m_pool.ForEachLive([](uint32_t, Pipeline* pipeline) { delete pipeline; });
        m_pool.Clear();
    }
}
