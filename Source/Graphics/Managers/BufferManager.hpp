//
// Created by otrush on 6/29/2026.
//

#ifndef SHIFT_BUFFERMANAGER_HPP
#define SHIFT_BUFFERMANAGER_HPP

#include "Graphics/RHI/RHI.hpp"
#include "GenerationalPool.hpp"

namespace Shift::Graphics {

    using BufferHandle = GenerationalPool<Buffer>::Handle;

    //! The single authority for engine-created GPU buffers (vertex/index/uniform/etc). Mirrors
    //! PipelineManager/TextureManager
    //!
    //! Note: transient staging buffers are NOT managed here, as they would be different logic for different times I guess
    class BufferManager {
    public:
        void Init(RenderBackend* rhi);

        [[nodiscard]] BufferHandle CreateBuffer(const BufferDescriptor& desc);

        [[nodiscard]] Buffer* Get(BufferHandle handle) const;

        void DestroyBuffer(BufferHandle handle);

        void Destroy();

    private:
        RenderBackend* m_rhi = nullptr;
        RenderBackendInterface* m_backend = nullptr;
        GenerationalPool<Buffer> m_pool;
    };
}

#endif //SHIFT_BUFFERMANAGER_HPP
