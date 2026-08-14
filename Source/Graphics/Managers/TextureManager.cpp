//
// Created by otrush on 1/16/2026.
//
#include "TextureManager.hpp"

#include <algorithm>
#include <array>
#include <utility>

namespace Shift::Graphics {
    TextureManager::TextureManager(ITextureLoader* loader, RenderBackend* rhi, GlobalResourceSet* globalSet, RenderContextEncoder* encoder)
        : m_loader(loader), m_rhi(rhi), m_backend(rhi->CreateInterface()), m_globalSet(globalSet) {

        //! Slot 0 is the permanently-resident placeholder, it is never released
        m_placeholder = m_pool.Insert(LoadAndCreateTexture("PLACEHOLDER", encoder));
    }

    TextureHandle TextureManager::GetOrLoadTexture(const std::string &path, RenderContextEncoder* encoder) {
        Texture* texture = LoadAndCreateTexture(path, encoder);

        //! Bugfix regarding storing nullprt in the pool
        if (!texture) { return m_placeholder; }

        //! GPU upload happens later via UploadTexturesToGPU
        return m_pool.Insert(texture);
    }

    void TextureManager::UnloadTexture(const TextureHandle &handle) {
        if (!m_pool.IsValid(handle))
            return;

        //! Do not delete placeholder texture on accident
        if (handle == m_placeholder) {
            Log(Warning, "Refusing to unload the placeholder texture (slot {})", handle.slotIdx);
            return;
        }

        //! Point this slot's bindless descriptor back at the placeholder before the texture goes away
        ClearTexture(handle.slotIdx);

        //! Recycle the slot and defer the delete, the texture may still be sampled by in-flight frames, so it must NOT be
        //! freed immediatly (tho ur welcome to try if you want to have fun:D)
        Texture* retired = m_pool.Release(handle);
        if (retired) {
            //! Drop the acquires for this texture as we have deleted it
            m_rhi->CancelPendingAcquires(retired);

            auto payload = m_rhi->GetGraphicsWaitPayload();
            m_rhi->DeferExecute(payload.semaphore, payload.value, [retired]() { delete retired; });
        }
    }

    bool TextureManager::IsValid(const TextureHandle& handle) const {
        return m_pool.IsValid(handle);
    }

    void TextureManager::FreeStagingBuffers() {
        m_usedStagingBuffers.clear();
    }

    void TextureManager::UploadTexturesToGPU(RenderContextEncoder *encoder) {
        //! Transition to read
        m_pool.ForEachLive([&](uint32_t, Texture* texture) {
            encoder->TransitionTexture(*texture, EResourceLayout::ShaderReadOnlyOptimal, EPipelineStageFlags::FragmentShaderBit);
        });
        //! Register to bindless array
        m_pool.ForEachLive([&](uint32_t slotIdx, Texture* texture) {
            UploadToGPU(slotIdx, texture);
        });
    }

    TextureHandle TextureManager::LoadTextureDeferred(const std::string &path) {
        std::optional<RawTextureData> rawData = LoadRawData(path);
        //! Revert to placeholder if data is bad
        if (!rawData) { return m_placeholder; }

        Texture* texture = CreateTextureForRaw(path, *rawData);
        const TextureHandle handle = m_pool.Insert(texture);

        //! The slot is live from this moment on, so it must not be left describing whichever
        //! image used to occupy it. The placeholder covers until RegisterAcquired runs
        ClearTexture(handle.slotIdx);

        m_pendingUploads.push_back(PendingUpload{handle, std::move(*rawData)});

        return handle;
    }

    bool TextureManager::SubmitPendingUploads() {
        if (m_pendingUploads.empty()) { return true; }

        //! TODO: [TEXTURES] This should be moved from wait to a transfer pool
        m_rhi->WaitForTransferIdle();

        RenderContext& tctx = m_rhi->GetTransferContext();
        RenderContextEncoder* encoder = tctx.CreateCommandEncoder();

        tctx.ResetCmds();
        CheckCritical(tctx.BeginCmds(), "Failed to begin the transfer context for a deferred texture upload!");

        encoder->PushDebugGroup("DeferredTextureUpload", {0.85f, 0.55f, 0.20f, 1.0f});

        std::vector<Buffer*> stagingBuffers;
        stagingBuffers.reserve(m_pendingUploads.size());

        for (PendingUpload& pending : m_pendingUploads) {
            //! The texture could have been unloaded even before submit (closed window, resources not needed, etc)
            Texture* texture = m_pool.Get(pending.handle);
            if (!texture) { continue; }

            stagingBuffers.push_back(RecordUploadCommands(*texture, pending.data, *encoder));
            m_pendingRegister.push_back(pending.handle);
        }
        m_pendingUploads.clear();

        encoder->PopDebugGroup();

        CheckCritical(tctx.EndCmds(), "Failed to end the transfer context for a deferred texture upload!");

        std::array sigPayloads{m_rhi->ReserveTransferSignalPayload()};
        const bool submitted = tctx.SubmitCmds({}, sigPayloads);

        m_rhi->DeferExecute(sigPayloads[0].semaphore, sigPayloads[0].value, [stagingBuffers]() {
            for (Buffer* staging : stagingBuffers) { delete staging; }
        });

        CheckCritical(submitted, "Failed to submit a deferred texture upload!");

        return true;
    }

    void TextureManager::RegisterSubmittedUploads() {
        if (m_pendingRegister.empty()) { return; }

        //! This is called after the prev frame wait
        for (const TextureHandle& handle : m_pendingRegister) {
            Texture* texture = m_pool.Get(handle);
            if (!texture) { continue; }

            //! Upload actual texture for usage with descriptors
            UploadToGPU(handle.slotIdx, texture);
        }

        m_pendingRegister.clear();
    }

    TextureManager::~TextureManager() {
        //! Caller has to guarantee GPU is idle
        m_pool.ForEachLive([](uint32_t, Texture* texture) { delete texture; });
        m_pool.Clear();
    }

    std::optional<RawTextureData> TextureManager::LoadRawData(const std::string &path) {
        std::optional<RawTextureData> rawData;
        if (path != "PLACEHOLDER") {
            rawData = m_loader->LoadFromFile(path);
        } else {
            rawData = m_loader->CreatePlaceholderTexture();
        }

        if (!rawData) {
            Log(Warning, "Failed to load texture at {}", path);
            return std::nullopt;
        }

        if (rawData->format == ETextureFormat::UNDEFINED) {
            //! 3 channel textures do not support optimal tiling
            // switch (rawData->channels) {
            //     case 1:
            //         rawData->format = ETextureFormat::R8_SRGB;
            //         break;
            //     case 2:
            //         rawData->format = ETextureFormat::R8G8_SRGB;
            //         break;
            //     case 3:
            //         rawData->format = ETextureFormat::R8G8B8_SRGB;
            //         break;
            //     case 4:
            //         rawData->format = ETextureFormat::R8G8B8A8_SRGB;
            //         break;
            //     default:
            //         Log(Warning, "Unsupported channel count {}", rawData->channels);
            //         return nullptr;
            // }
            rawData->format = ETextureFormat::R8G8B8A8_SRGB;
            rawData->channels = 4;
        }

        return rawData;
    }

    Texture* TextureManager::CreateTextureForRaw(const std::string &path, const RawTextureData &raw) {
        return m_backend->CreateTexture(TextureDescriptor::CreateTexture2DDesc(
            raw.width,
            raw.height,
            path.c_str(),
            raw.format,
            raw.mipLevels,
            ETextureUsageFlags::TransferSrc | ETextureUsageFlags::TransferDst | ETextureUsageFlags::Sampled,
            ETextureAspect::Color,
            raw.isCubemap
        ));
    }

    Buffer* TextureManager::RecordUploadCommands(Texture &texture, const RawTextureData &raw, RenderContextEncoder &encoder) {
        const uint32_t imageSize = raw.width * raw.height * raw.channels;

        //! Workaround for my stupid fix in stbi loader fuck
        const size_t availableBytes = std::min(static_cast<size_t>(imageSize), raw.data.size());
        if (availableBytes < static_cast<size_t>(imageSize)) {
            Log(Error, "Texture upload carries {} byte(s) of pixels but {}x{}x{} channels needs {}",
                raw.data.size(), raw.width, raw.height, raw.channels, imageSize);
        }

        Buffer* stagingBuf = m_backend->CreateBuffer(BufferDescriptor{
            .size = imageSize,
            .name = "Staging",
            .type = EBufferType::Staging
        });
        memcpy(stagingBuf->GetMapped(), raw.data.data(), availableBytes);

        //! We support no mips and overall this is fucked for now
        TextureSubresourceRange subresourceRange{};
        subresourceRange.aspect = texture.GetAspect();
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = raw.arrayLayers;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;

        encoder.TransitionTexture(texture, EResourceLayout::TransferDstOptimal, EPipelineStageFlags::AllTransferBit);
        encoder.CopyBufferToTexture(
            BufferOpDescriptor{stagingBuf, 0},
            TextureCopyDescriptor{&texture, Extent3D{texture.GetWidth(), texture.GetHeight(), texture.GetDepth()}, Offset3D{}, subresourceRange});

        //! The upload runs on the transfer queue but every reader is a shader on the graphics
        //! one, so the image is handed over here rather than merely transitioned. This is the
        //! release half plus the layout change, the acquire half is recorded on the graphics
        //! context by RHI::FlushPendingAcquires before anything samples it
        encoder.ReleaseQueueOwnership(texture, EContextType::Graphics,
                                      EResourceLayout::ShaderReadOnlyOptimal, EPipelineStageFlags::FragmentShaderBit);

        //! TODO: [Feature]: Generate mips

        return stagingBuf;
    }

    Texture* TextureManager::LoadAndCreateTexture(const std::string &path, RenderContextEncoder* encoder) {
        std::optional<RawTextureData> rawData = LoadRawData(path);
        if (!rawData) { return nullptr; }

        Texture* texture = CreateTextureForRaw(path, *rawData);

        m_usedStagingBuffers.push_back(Core::UniquePtr<Buffer>(RecordUploadCommands(*texture, *rawData, *encoder)));

        return texture;
        //! FYI: End and submit have to be called via TextureManager::SubmitAllLoads()
    }

    void TextureManager::UploadToGPU(uint32_t slotIdx, Texture *texture) {
        m_globalSet->WriteImage2D(slotIdx, *texture);
        m_globalSet->Apply();
    }

    void TextureManager::ClearTexture(uint32_t slotIdx) {
        m_globalSet->WriteImage2D(slotIdx, *m_pool.Get(m_placeholder));
        m_globalSet->Apply();
    }
}
