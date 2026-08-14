//
// Created by otrush on 1/15/2026.
//

#ifndef SHIFT_TEXTUREMANAGER_HPP
#define SHIFT_TEXTUREMANAGER_HPP

#include <memory>

#include "Loaders/TextureLoader/ITextureLoader.hpp"

#include "Graphics/RHI/RHI.hpp"
#include "GenerationalPool.hpp"
#include "GlobalResourceSet.hpp"

namespace Shift::Graphics {

    //! Opaque, generation-checked reference to a bindless texture. slotIdx IS the index the shader
    //! samples in the bindless array
    using TextureHandle = GenerationalPool<Texture>::Handle;

    class TextureManager {
    public:
        TextureManager(ITextureLoader* loader, RenderBackend* rhi, GlobalResourceSet* globalSet, RenderContextEncoder* encode);

        [[nodiscard]] TextureHandle GetOrLoadTexture(const std::string& path, RenderContextEncoder* encode);

        void UnloadTexture(const TextureHandle& texHandle);

        [[nodiscard]] bool IsValid(const TextureHandle& handle) const;

        //! The permanently-resident 1x1 fallback at slot 0
        //! Never unloadable apart from destruction
        [[nodiscard]] TextureHandle GetPlaceholderHandle() const { return m_placeholder; }

        //! TODO [Feature] Implement a staging buffer pool so that this is not needed
        void FreeStagingBuffers();

        void UploadTexturesToGPU(RenderContextEncoder* encoder);

        ///! ================ Mid-run uploads ====================

        //! Book a texture slot and add the load to pending for the submit pending uploads that runs once per frame
        [[nodiscard]] TextureHandle LoadTextureDeferred(const std::string& path);

        //! Record and submit every deferred upload queued so far on the transfer queue.
        //! No-op when nothing is pending. Must run before the frame's graphics recording begins
        [[nodiscard]] bool SubmitPendingUploads();

        //! Point the bindless slots of the last submitted uploads at their real images.
        //! Must run after RHI::FlushPendingAcquires as we switch queues (potentially I guess)
        void RegisterSubmittedUploads();

        ~TextureManager();
    private:
        //! Just get the Raw data from an image
        [[nodiscard]] std::optional<RawTextureData> LoadRawData(const std::string& path);

        //! Create the GPU texture object sized/formatted for this raw data. Records nothing
        [[nodiscard]] Texture* CreateTextureForRaw(const std::string& path, const RawTextureData& raw);

        //! Record transfer-queue upload commands for one texture: staging fill, copy and the
        //! release half of the handoff to graphics. Returns the staging buffer
        [[nodiscard]] Buffer* RecordUploadCommands(Texture& texture, const RawTextureData& raw, RenderContextEncoder& encoder);

        Texture* LoadAndCreateTexture(const std::string & path, RenderContextEncoder* encode);

        void UploadToGPU(uint32_t slotIdx, Texture * texture);

        void ClearTexture(uint32_t slotIdx);

        GenerationalPool<Texture> m_pool;
        //! Free slot placeholder
        TextureHandle m_placeholder;

        ITextureLoader* m_loader;
        RenderBackend* m_rhi;
        RenderBackendInterface* m_backend;
        GlobalResourceSet* m_globalSet;

        std::vector<Core::UniquePtr<Buffer>> m_usedStagingBuffers;

        //! A textures with loaded raw data and a reserved slot but no GPU resources
        struct PendingUpload {
            TextureHandle handle;
            RawTextureData data;
        };

        std::vector<PendingUpload> m_pendingUploads;
        //! Uploads already submitted, waiting for their bindless slot to be pointed at them
        std::vector<TextureHandle> m_pendingRegister;
    };
}

#endif //SHIFT_TEXTUREMANAGER_HPP
