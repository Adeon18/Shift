//
// Created by otrush on 1/16/2026.
//
#include "TextureManager.hpp"
#include "Config/EngineConfig.hpp"

namespace Shift::Graphics {
    TextureManager::TextureManager(ITextureLoader* loader, RenderBackend* rhi, RenderContextEncoder* encoder)
        : m_loader(loader), m_rhi(rhi), m_backend(rhi->CreateInterface()) {

        PipelineLayoutDescriptor pipelineLayoutDescriptor;
        pipelineLayoutDescriptor.bindings.push_back(
            PipelineLayoutDescriptor::LayoutBindingDesc {
                .binding = 0,
                .type = EBindingType::SampledImage,
                .stageFlags = EBindingVisibility::Vertex |
                    // EBindingVisibility::TesselationEvaluation |
                    // EBindingVisibility::TesselationControl |
                    EBindingVisibility::Geometry |
                    EBindingVisibility::Fragment |
                    EBindingVisibility::Compute,
                .count = Conf::MAX_BINDLESS_IMAGES,
                .isBindless = true
            }
        );

        m_bindlessTextureSet = m_backend->CreateResourceSet(pipelineLayoutDescriptor);

        //! Slot 0 is the permanently-resident placeholder, it is never released
        m_placeholder = m_pool.Insert(LoadAndCreateTexture("PLACEHOLDER", encoder));
    }

    TextureHandle TextureManager::GetOrLoadTexture(const std::string &path, RenderContextEncoder* encoder) {
        //! GPU upload happens later via UploadTexturesToGPU
        return m_pool.Insert(LoadAndCreateTexture(path, encoder));
    }

    void TextureManager::UnloadTexture(const TextureHandle &handle) {
        if (!m_pool.IsValid(handle))
            return;

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

    TextureManager::~TextureManager() {
        //! Caller has to guarantee GPU is idle
        m_pool.ForEachLive([](uint32_t, Texture* texture) { delete texture; });
        m_pool.Clear();

        delete m_bindlessTextureSet;
    }

    Texture* TextureManager::LoadAndCreateTexture(const std::string &path, RenderContextEncoder* encoder) {
        std::optional<RawTextureData> rawData;
        if (path != "PLACEHOLDER") {
            rawData = m_loader->LoadFromFile(path);
        } else {
            rawData = m_loader->CreatePlaceholderTexture();
        }

        if (!rawData) {
            Log(Warning, "Failed to load texture at {}", path);
            return nullptr;
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
        } else {
            rawData->format = rawData->format;
        }

        Texture* texture = m_backend->CreateTexture(TextureDescriptor::CreateTexture2DDesc(
            rawData->width,
            rawData->height,
            path.c_str(),
            rawData->format,
            rawData->mipLevels,
            ETextureUsageFlags::TransferSrc | ETextureUsageFlags::TransferDst | ETextureUsageFlags::Sampled,
            ETextureAspect::Color,
            rawData->isCubemap
        ));

        uint32_t imageSize = rawData->width * rawData->height * rawData->channels;
        Core::UniquePtr<Buffer> stagingBuf = Core::UniquePtr<Buffer>(m_backend->CreateBuffer(BufferDescriptor{
            .size = imageSize,
            .name = "Staging",
            .type = EBufferType::Staging
        }));
        memcpy(stagingBuf->GetMapped(), rawData->data.data(), static_cast<size_t>(imageSize));

        TextureSubresourceRange subresourceRange{};
        subresourceRange.aspect = texture->GetAspect();
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = 1;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = texture->GetMipCount();

        encoder->TransitionTexture(*texture, EResourceLayout::TransferDstOptimal, EPipelineStageFlags::AllTransferBit);
        encoder->CopyBufferToTexture(
            BufferOpDescriptor{stagingBuf.get(), 0},
            TextureCopyDescriptor{texture, Extent3D{texture->GetWidth(), texture->GetHeight(), texture->GetDepth()}, Offset3D{}, subresourceRange});

        //! The upload runs on the transfer queue but every reader is a shader on the graphics
        //! one, so the image is handed over here rather than merely transitioned. This is the
        //! release half plus the layout change, the acquire half is recorded on the graphics
        //! context by RHI::FlushPendingAcquires before anything samples it
        encoder->ReleaseQueueOwnership(*texture, EContextType::Graphics,
                                       EResourceLayout::ShaderReadOnlyOptimal, EPipelineStageFlags::FragmentShaderBit);

        m_usedStagingBuffers.push_back(std::move(stagingBuf));


        //! TODO: [Feature]: Generate mips


        return texture;
        //! FYI: End and submit have to be called via TextureManager::SubmitAllLoads()
    }

    void TextureManager::UploadToGPU(uint32_t slotIdx, Texture *texture) {
        m_bindlessTextureSet->UpdateTexture(0, slotIdx, *texture);
        m_bindlessTextureSet->Apply();
    }

    void TextureManager::ClearTexture(uint32_t slotIdx) {
        m_bindlessTextureSet->UpdateTexture(0, slotIdx, *m_pool.Get(m_placeholder));
        m_bindlessTextureSet->Apply();
    }
}
