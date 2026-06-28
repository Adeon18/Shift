//
// Created by otrush on 6/28/2026.
//

#ifndef SHIFT_PIPELINEMANAGER_HPP
#define SHIFT_PIPELINEMANAGER_HPP

#include <span>
#include <vector>
#include <unordered_set>

#include "Graphics/RHI/RHI.hpp"
#include "ShaderManager.hpp"

namespace Shift::Graphics {

    //! Opaque, generation-checked reference to a pipeline owned by PipelineManager. Same shape as
    //! TextureHandle
    struct PipelineHandle {
        uint32_t slotIdx = 0;
        uint32_t generation = 0;
        bool operator==(const PipelineHandle& o) const noexcept = default;
    };

    //! The single authority for graphics pipelines, and the ONLY thing engine systems call to get
    //! one, subscribes it for hot-reload, and takes ownership
    //!
    //! Ownership: the manager single-owns every pipeline in a
    //! generational slot pool
    //!
    //! Future home of the engine-level PSO cache: dedup identical pipelines by a hash of
    //! (PipelineDescriptor + shader identities), turning CreatePipeline into get-or-create. The
    //! backend VkPipelineCache and pipeline-layout cache stay backend-internal.
    class PipelineManager {
    public:
        void Init(RenderBackend* rhi, ShaderManager* shaderManager);

        //! Compile/fetch the shaders, create the pipeline, subscribe it for hot-reload and take
        //! ownership. Returns an opaque handle; resolve to a Pipeline& for binding via Get().
        [[nodiscard]] PipelineHandle CreatePipeline(const PipelineDescriptor& desc, std::span<const ShaderDescriptor> shaderSources);

        //! Resolve a handle to the owned pipeline. Returns nullptr if the handle is stale.
        //! A const read: safe to call concurrently from worker threads
        //! during command recording, provided no pipeline is created/destroyed in that window.
        [[nodiscard]] Pipeline* Get(PipelineHandle handle) const;

        //! Destroy a pipeline mid-session: unsubscribe it from hot-reload, invalidate outstanding
        //! handles, and defer the GPU-side delete until the GPU is finished with in-flight work.
        void DestroyPipeline(PipelineHandle handle);

        //! Recompile any dirty shaders and rebuild every affected pipeline in place, deferring the
        //! retired GPU handle's release until the GPU is finished with it.
        void HotReload();

        //! Shutdown teardown. Precondition: GPU idle + RHI deferred queue flushed by the caller.
        void Destroy();

    private:
        //! One owned pipeline. Held by RAW pointer
        struct PipelineSlot {
            Pipeline* pipeline = nullptr;
            std::vector<ShaderStageDesc> stages;   //!< kept so we can unsubscribe from hot-reload on destroy
            uint32_t generation = 0;
            bool alive = false;
        };

        [[nodiscard]] PipelineSlot* ResolveSlot(PipelineHandle handle);

        RenderBackend* m_rhi = nullptr;
        RenderBackendInterface* m_backend = nullptr;
        ShaderManager* m_shaderManager = nullptr;

        //! Generational slot pool
        std::vector<PipelineSlot> m_slots;
        std::vector<uint32_t> m_freeSlots;
    };
}

#endif //SHIFT_PIPELINEMANAGER_HPP
