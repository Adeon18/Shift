//
// Created by otrush on 1/15/2026.
//

#ifndef SHIFT_TEXTUREMANAGER_HPP
#define SHIFT_TEXTUREMANAGER_HPP

#include <memory>

#include "Loaders/TextureLoader/ITextureLoader.hpp"

#include "Graphics/RHI/RHI.hpp"

namespace Shift::Graphics {

    struct TextureHandle {
        //! This is the value that the shader has access to
        uint32_t slotIdx = 0;
        uint32_t generation = 0;

        bool operator==(const TextureHandle& other) const noexcept {
            return slotIdx == other.slotIdx && generation == other.generation;
        }
    };


    class TextureManager {
    struct TextureSlot {
        Texture* backendHandle = nullptr;
        bool isResident = false;
        uint32_t generation = 0;
    };
    public:
        TextureManager(ITextureLoader* loader, RenderBackendInterface* backend, RenderContextEncoder* encode);

        [[nodiscard]] TextureHandle GetOrLoadTexture(const std::string& path, RenderContextEncoder* encode);

        void UnloadTexture(const TextureHandle& texHandle);

        [[nodiscard]] bool IsValid(const TextureHandle& handle) const;

        //! TODO [Feature] Implement a staging buffer pool so that this is not needed
        void FreeStagingBuffers();

        void UploadTexturesToGPU(RenderContextEncoder* encoder);

        ~TextureManager();
    private:
        Texture* LoadAndCreateTexture(const std::string & path, RenderContextEncoder* encode);

        void UploadToGPU(uint32_t uint32, Texture * texture);

        void ClearTexture(uint32_t uint32);

        std::vector<TextureSlot> m_slots;
        std::vector<uint32_t> m_freeSlots;
        ITextureLoader* m_loader;
        RenderBackendInterface* m_backend;
        ResourceSet* m_bindlessTextureSet;

        std::vector<Buffer*> m_usedStagingBuffers;
    };
}

#endif //SHIFT_TEXTUREMANAGER_HPP