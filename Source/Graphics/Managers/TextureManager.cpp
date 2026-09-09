//
// Created by otrush on 1/16/2026.
//
#include "TextureManager.hpp"

#include "Config/EngineConfig.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <utility>

namespace Shift::Graphics {
    namespace {
        size_t HashCombine(size_t seed, size_t value) {
            constexpr size_t GOLDEN = static_cast<size_t>(0x9e3779b97f4a7c15ULL);
            return seed ^ (std::hash<size_t>()(value) + GOLDEN + (seed << 6) + (seed >> 2));
        }

        constexpr std::array PLACEHOLDERS{
            TextureManager::PlaceholderDesc{{255, 255, 255, 255}, ETextureColorSpace::SRGB,   "PlaceholderWhiteSRGB"},
            TextureManager::PlaceholderDesc{{255, 255, 255, 255}, ETextureColorSpace::Linear, "PlaceholderWhiteLinear"},
            TextureManager::PlaceholderDesc{{128, 128, 255, 255}, ETextureColorSpace::Linear, "PlaceholderFlatNormal"},
        };
        static_assert(PLACEHOLDERS.size() == static_cast<size_t>(ETexturePlaceholder::Count), "every ETexturePlaceholder needs a row here, in enum order");

        uint32_t GetFullMipChainLength(uint32_t width, uint32_t height) {
            uint32_t levels = 1;
            for (uint32_t extent = std::max(width, height); extent > 1; extent >>= 1) { ++levels; }
            return levels;
        }
    }
    TextureManager::TextureManager(ITextureLoader* loader, RenderBackend* rhi, GlobalResourceSet* globalSet, RenderContextEncoder* encoder)
        : m_loader(loader), m_rhi(rhi), m_backend(rhi->CreateInterface()), m_globalSet(globalSet) {

        //! Slot 0 is the error texture
        m_errorTexture = CreatePlaceholderTexture({ITextureLoader::ERROR_COLOR, ETextureColorSpace::SRGB, "ErrorTexture"}, encoder);

        for (uint32_t i = 0; i < PLACEHOLDERS.size(); ++i) {
            TextureHandle h = CreatePlaceholderTexture({ PLACEHOLDERS[i].rgba, PLACEHOLDERS[i].colorSpace, PLACEHOLDERS[i].debugName }, encoder);
            if (!m_pool.IsValid(h)) {
                Log(Warning, "Placeholder texture at index {} was not created", i);
            } else {
                m_placeholders[i] = h;
            }
        }

        CheckCriticalEmptyReturn(m_pool.IsValid(m_errorTexture), "The error texture could not be created!");
    }

    TextureHandle TextureManager::CreatePlaceholderTexture(const PlaceholderDesc& desc, RenderContextEncoder* encoder) {
        std::optional<RawTextureData> raw = m_loader->Create1x1Texture(desc.rgba);
        if (!raw) {
            Log(Error, "Could not build the 1x1 placeholder '{}'", desc.debugName);
            return TextureHandle{};
        }
        raw->format = (desc.colorSpace == ETextureColorSpace::SRGB) ? ETextureFormat::R8G8B8A8_SRGB: ETextureFormat::R8G8B8A8_UNORM;
        raw->channels = 4;

        Texture* texture = CreateTextureForRaw(desc.debugName, *raw, FULL_MIP_CHAIN);
        m_usedStagingBuffers.push_back(Core::UniquePtr<Buffer>(RecordUploadCommands(*texture, *raw, *encoder)));

        //! These are not in cache
        return m_pool.Insert(texture);
    }

    uint32_t TextureManager::GetPlaceholderSlot(ETexturePlaceholder kind) const {
        if (kind >= ETexturePlaceholder::Count) { return m_errorTexture.slotIdx; }

        const TextureHandle handle = m_placeholders[static_cast<size_t>(kind)];
        if (!m_pool.IsValid(handle)) { return m_errorTexture.slotIdx; }
        return handle.slotIdx;
    }

    bool TextureManager::IsPlaceholderSlot(uint32_t slotIdx) const {
        if (slotIdx == m_errorTexture.slotIdx) { return true; }
        for (const TextureHandle& handle : m_placeholders) {
            if (handle.slotIdx == slotIdx && m_pool.IsValid(handle)) {
                return true;
            }
        }
        return false;
    }

    bool TextureManager::HasFreeSlots() const {
        //! reuse a recycled slot if we can
        if (m_pool.HasRecycledSlot()) { return true; }
        return m_pool.SlotCount() < Conf::MAX_BINDLESS_IMAGES;
    }

    size_t TextureManager::CacheKeyHash::operator()(const CacheKey& key) const {
        size_t hash = HashCombine(std::hash<std::string>{}(key.path), static_cast<size_t>(key.colorSpace));
        return HashCombine(hash, static_cast<size_t>(key.mipLevels));
    }

    TextureHandle TextureManager::FindInCache(const CacheKey& key) const {
        const auto it = m_cache.find(key);
        return (it != m_cache.end()) ? it->second : TextureHandle{};
    }

    void TextureManager::StoreInCache(const CacheKey& key, TextureHandle handle) {
        m_cache.insert_or_assign(key, handle);
    }

    TextureHandle TextureManager::GetOrLoadTexture(const std::string &path, RenderContextEncoder* encoder, ETextureColorSpace colorSpace, uint32_t mipLevels) {
        const CacheKey key{path, colorSpace, mipLevels};

        const TextureHandle cached = FindInCache(key);
        if (m_pool.IsValid(cached)) { return cached; }

        //! Overflow
        if (!HasFreeSlots()) {
            Log(Error, "The bindless image array is full at {} slots; '{}' falls back to the error texture", Conf::MAX_BINDLESS_IMAGES, path);
            return m_errorTexture;
        }

        Texture* texture = LoadAndCreateTexture(path, encoder, colorSpace, mipLevels);

        //! Do not store the failed load in cache
        if (!texture) { return m_errorTexture; }

        //! GPU upload happens later via UploadTexturesToGPU
        const TextureHandle handle = m_pool.Insert(texture);
        StoreInCache(key, handle);
        return handle;
    }

    void TextureManager::UnloadTexture(const TextureHandle &handle) {
        if (!m_pool.IsValid(handle))
            return;

        //! Cannot unload a placeholder/error slot
        if (IsPlaceholderSlot(handle.slotIdx)) {
            Log(Warning, "Refusing to unload the placeholder texture (slot {})", handle.slotIdx);
            return;
        }

        //! Point this slot's bindless descriptor back at the error texture before the texture goes away
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

    ETextureFormat TextureManager::GetFormat(const TextureHandle& handle) const {
        const Texture* texture = m_pool.Get(handle);
        return texture ? texture->GetFormat() : ETextureFormat::UNDEFINED;
    }

    ETextureFormat TextureManager::GetFormatOfSlot(uint32_t slotIdx) {
        ETextureFormat found = ETextureFormat::UNDEFINED;
        m_pool.ForEachLive([&](uint32_t liveSlot, Texture* texture) {
            if (liveSlot == slotIdx) { found = texture->GetFormat(); }
        });
        return found;
    }

    uint32_t TextureManager::GetMipCount(const TextureHandle& handle) const {
        const Texture* texture = m_pool.Get(handle);
        return texture ? texture->GetMipCount() : 0u;
    }

    uint32_t TextureManager::GetMipCountOfSlot(uint32_t slotIdx) {
        uint32_t found = 0u;
        m_pool.ForEachLive([&](uint32_t liveSlot, Texture* texture) {
            if (liveSlot == slotIdx) { found = texture->GetMipCount(); }
        });
        return found;
    }

    void TextureManager::FreeStagingBuffers() {
        m_usedStagingBuffers.clear();
    }

    void TextureManager::UploadTexturesToGPU(RenderContextEncoder *encoder) {
        m_pool.ForEachLive([&](uint32_t slotIdx, Texture* texture) {
            FinalizeUpload(encoder, slotIdx, texture);
        });
    }

    TextureHandle TextureManager::LoadTextureDeferred(const std::string &path, ETextureColorSpace colorSpace, uint32_t mipLevels) {
        const CacheKey key{path, colorSpace, mipLevels};

        const TextureHandle cached = FindInCache(key);
        if (m_pool.IsValid(cached)) { return cached; }

        if (!HasFreeSlots()) {
            Log(Error, "The bindless image array is full at {} slots; '{}' falls back to the error texture", Conf::MAX_BINDLESS_IMAGES, path);
            return m_errorTexture;
        }

        std::optional<RawTextureData> rawData = LoadRawData(path, colorSpace);
        //! Revert to the error texture if data is bad
        if (!rawData) { return m_errorTexture; }

        Texture* texture = CreateTextureForRaw(path, *rawData, mipLevels);
        const TextureHandle handle = m_pool.Insert(texture);
        StoreInCache(key, handle);

        //! The slot is live from this moment on, so it must not be left describing whichever
        //! image used to occupy it. The error texture covers until RegisterAcquired runs
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

    void TextureManager::RegisterSubmittedUploads(RenderContextEncoder* encoder) {
        if (m_pendingRegister.empty()) { return; }

        //! This is called after the prev frame wait
        for (const TextureHandle& handle : m_pendingRegister) {
            Texture* texture = m_pool.Get(handle);
            if (!texture) { continue; }

            FinalizeUpload(encoder, handle.slotIdx, texture);
        }

        m_pendingRegister.clear();
    }

    TextureManager::~TextureManager() {
        //! Caller has to guarantee GPU is idle
        m_pool.ForEachLive([](uint32_t, Texture* texture) { delete texture; });
        m_pool.Clear();
    }

    std::optional<RawTextureData> TextureManager::LoadRawData(const std::string &path, ETextureColorSpace colorSpace) {
        std::optional<RawTextureData> rawData = m_loader->LoadFromFile(path);

        if (!rawData) {
            Log(Warning, "Failed to load texture at {}", path);
            return std::nullopt;
        }

        if (rawData->format == ETextureFormat::UNDEFINED) {
            rawData->format = (colorSpace == ETextureColorSpace::SRGB)
                                  ? ETextureFormat::R8G8B8A8_SRGB
                                  : ETextureFormat::R8G8B8A8_UNORM;
            rawData->channels = 4;
        }

        return rawData;
    }

    Texture* TextureManager::CreateTextureForRaw(const std::string &path, const RawTextureData &raw, uint32_t requestedMipLevels) {
        //! TODO: [TEXTURES] a loader may deliver it pown chain and it is not handled yet
        //! and must skip generation - the upload path copies level 0 only, so it cannot yet
        const uint32_t maxLevels = GetFullMipChainLength(raw.width, raw.height);
        const bool outOfContract = requestedMipLevels != FULL_MIP_CHAIN &&
                                   (requestedMipLevels == 0 || requestedMipLevels > maxLevels);
        if (outOfContract) {
            Log(Warning, "{} asked for {} mip level(s) but {}x{} allows 1..{}; clamping",
                path, requestedMipLevels, raw.width, raw.height, maxLevels);
        }
        const uint32_t mipLevels = (requestedMipLevels == FULL_MIP_CHAIN)
                                       ? maxLevels
                                       : std::clamp(requestedMipLevels, 1u, maxLevels);

        return m_backend->CreateTexture(TextureDescriptor::CreateTexture2DDesc(
            raw.width,
            raw.height,
            path.c_str(),
            raw.format,
            mipLevels,
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

        //! Here we handle level 0 only as we need graphics queue to gen mips
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
        //! context by RHI::FlushPendingAcquires before anything samples it.
        //! TODO: [PERFORMANCE] One barrier too much becuase we co not check whether the image has only one mip to leave it as read only optimal straing up as no mips will be generated
        encoder.ReleaseQueueOwnership(texture, EContextType::Graphics, EResourceLayout::TransferDstOptimal, EPipelineStageFlags::AllTransferBit);

        return stagingBuf;
    }

    Texture* TextureManager::LoadAndCreateTexture(const std::string &path, RenderContextEncoder* encoder, ETextureColorSpace colorSpace, uint32_t mipLevels) {
        std::optional<RawTextureData> rawData = LoadRawData(path, colorSpace);
        if (!rawData) { return nullptr; }

        Texture* texture = CreateTextureForRaw(path, *rawData, mipLevels);

        m_usedStagingBuffers.push_back(Core::UniquePtr<Buffer>(RecordUploadCommands(*texture, *rawData, *encoder)));

        return texture;
        //! FYI: End and submit have to be called via TextureManager::SubmitAllLoads()
    }

    void TextureManager::FinalizeUpload(RenderContextEncoder* encoder, uint32_t slotIdx, Texture* texture) {
        //! Graphics queue only :D
        encoder->GenerateMips(*texture, EResourceLayout::ShaderReadOnlyOptimal, EPipelineStageFlags::FragmentShaderBit);
        UploadToGPU(slotIdx, texture);
    }

    void TextureManager::UploadToGPU(uint32_t slotIdx, Texture *texture) {
        m_globalSet->WriteImage2D(slotIdx, *texture);
        m_globalSet->Apply();
    }

    void TextureManager::ClearTexture(uint32_t slotIdx) {
        m_globalSet->WriteImage2D(slotIdx, *m_pool.Get(m_errorTexture));
        m_globalSet->Apply();
    }
}
