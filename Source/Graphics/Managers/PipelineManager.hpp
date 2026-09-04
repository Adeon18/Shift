//
// Created by otrush on 6/28/2026.
//

#ifndef SHIFT_PIPELINEMANAGER_HPP
#define SHIFT_PIPELINEMANAGER_HPP

#include <span>
#include <vector>
#include <unordered_set>

#include "Graphics/RHI/RHI.hpp"
#include "GenerationalPool.hpp"
#include "ShaderManager.hpp"

namespace Shift::Graphics {
    struct PipelineMeta {
        std::vector<ShaderDescriptor> sources;
        std::vector<ShaderStageDesc> assets;
        bool isFallback = false;
    };

    using PipelineHandle = GenerationalPool<Pipeline, PipelineMeta>::Handle;

    //! The main class for getting pipelines and subscribing them to how reload. Owns every pipeline
    class PipelineManager {
    public:
        void Init(RenderBackend* rhi, ShaderManager* shaderManager);

        //! Compile/fetch the shaders, create the pipeline, subscribe it for hot-reload and take
        //! ownership. Returns an opaque handle; resolve to a Pipeline* for binding via Get().
        [[nodiscard]] PipelineHandle CreatePipeline(const PipelineDescriptor& desc, std::span<const ShaderDescriptor> shaderSources);

        //! Resolve a handle to the owned pipeline. Returns nullptr if the handle is stale.
        //! A const read: safe to call concurrently from worker threads during command recording,
        //! provided no pipeline is created/destroyed in that window.
        [[nodiscard]] Pipeline* Get(PipelineHandle handle) const;

        //! Destroy a pipeline mid-session: unsubscribe it from hot-reload, invalidate outstanding
        //! handles, and defer the GPU-side delete until the GPU is finished with in-flight work.
        void DestroyPipeline(PipelineHandle handle);

        //! Recompile any dirty shaders and rebuild every affected pipeline in place, deferring the
        //! retired GPU handle's release until the GPU is finished with it.
        //! Returns the number of pipelines rebuilt
        uint32_t HotReload();

        //! Shutdown teardown. Precondition: GPU idle + RHI deferred queue flushed by the caller.
        void Destroy();

        //! True when pipeline is using fallback shaders
        [[nodiscard]] bool IsRunningFallback(PipelineHandle handle);

    private:
        //! Ask the ShaderManager for each source's own asset. Empty if any stage has neither a
        //! compiled shader nor a fallback to stand in for it
        [[nodiscard]] std::vector<ShaderStageDesc> ResolveSources(std::span<const ShaderDescriptor> sources,
                                                                  bool& outAnyFallback);

        //! Manage stages
        [[nodiscard]] std::vector<ShaderStageDesc> ResolveStages(std::span<const ShaderStageDesc> assets,
                                                                 bool anyFallback);

        RenderBackend* m_rhi = nullptr;
        RenderBackendInterface* m_backend = nullptr;
        ShaderManager* m_shaderManager = nullptr;

        GenerationalPool<Pipeline, PipelineMeta> m_pool;
    };
}

#endif //SHIFT_PIPELINEMANAGER_HPP
