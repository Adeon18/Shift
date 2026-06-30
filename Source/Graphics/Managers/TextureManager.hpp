//
// Created by otrush on 1/15/2026.
//

#ifndef SHIFT_TEXTUREMANAGER_HPP
#define SHIFT_TEXTUREMANAGER_HPP

#include <memory>

#include "Loaders/TextureLoader/ITextureLoader.hpp"

#include "Graphics/RHI/RHI.hpp"
#include "GenerationalPool.hpp"

namespace Shift::Graphics {

    //! Opaque, generation-checked reference to a bindless texture. slotIdx IS the index the shader
    //! samples in the bindless array
    using TextureHandle = GenerationalPool<Texture>::Handle;

    class TextureManager {
    public:
        TextureManager(ITextureLoader* loader, RenderBackend* rhi, RenderContextEncoder* encode);

        [[nodiscard]] TextureHandle GetOrLoadTexture(const std::string& path, RenderContextEncoder* encode);

        void UnloadTexture(const TextureHandle& texHandle);

        [[nodiscard]] bool IsValid(const TextureHandle& handle) const;

        //! TODO [Feature] Implement a staging buffer pool so that this is not needed
        void FreeStagingBuffers();

        void UploadTexturesToGPU(RenderContextEncoder* encoder);

        ~TextureManager();
    private:
        Texture* LoadAndCreateTexture(const std::string & path, RenderContextEncoder* encode);

        void UploadToGPU(uint32_t slotIdx, Texture * texture);

        void ClearTexture(uint32_t slotIdx);

        GenerationalPool<Texture> m_pool;
        //! Free slot placeholder
        TextureHandle m_placeholder;

        ITextureLoader* m_loader;
        RenderBackend* m_rhi;
        RenderBackendInterface* m_backend;
        ResourceSet* m_bindlessTextureSet;

        std::vector<Core::UniquePtr<Buffer>> m_usedStagingBuffers;
    };
}

#endif //SHIFT_TEXTUREMANAGER_HPP
