//
// Created by otrush on 1/16/2026.
//
#include "TextureManager.hpp"
#include "Config/EngineConfig.hpp"

namespace Shift::Graphics {
    TextureManager::TextureManager(ITextureLoader* loader, RenderBackendInterface* backend, RenderContextEncoder* encoder): m_loader(loader), m_backend(backend) {

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

        m_slots.resize(1);
        m_slots[0].isResident = true;
        m_slots[0].backendHandle = Core::UniquePtr<Texture>(LoadAndCreateTexture("PLACEHOLDER", encoder));
    }

    TextureHandle TextureManager::GetOrLoadTexture(const std::string &path, RenderContextEncoder* encoder) {
        uint32_t slotIndex;

        if (!m_freeSlots.empty()) {
            slotIndex = m_freeSlots.back();
            m_freeSlots.pop_back();
        }
        else
        {
            slotIndex = static_cast<uint32_t>(m_slots.size());
            m_slots.emplace_back();
        }

        TextureSlot& slot = m_slots[slotIndex];
        slot.isResident = true;
        slot.backendHandle = Core::UniquePtr<Texture>(LoadAndCreateTexture(path, encoder));

        //! GPU Uploads come via separate function

        return TextureHandle{slotIndex, slot.generation};
    }

    void TextureManager::UnloadTexture(const TextureHandle &handle) {
        if (!IsValid(handle))
            return;

        TextureSlot& slot = m_slots[handle.slotIdx];

        ClearTexture(handle.slotIdx);

        slot.isResident = false;
        slot.backendHandle = nullptr;
        //! This woild invalidate all existing handles
        slot.generation++;

        m_freeSlots.push_back(handle.slotIdx);
    }

    bool TextureManager::IsValid(const TextureHandle& handle) const {
        if (handle.slotIdx >= m_slots.size())
            return false;

        const TextureSlot& slot = m_slots[handle.slotIdx];
        return slot.isResident && slot.generation == handle.generation;
    }

    void TextureManager::FreeStagingBuffers() {
        m_usedStagingBuffers.clear();
    }

    void TextureManager::UploadTexturesToGPU(RenderContextEncoder *encoder) {
        //! First transition all for reading
        for (uint32_t i = 0; i < m_slots.size(); i++) {
            if (m_slots[i].isResident) {
                encoder->TransitionTexture(*m_slots[i].backendHandle, EResourceLayout::ShaderReadOnlyOptimal, EPipelineStageFlags::FragmentShaderBit);
            }
        }
        //! Then upload all on GPU
        for (uint32_t i = 0; i < m_slots.size(); i++) {
            if (m_slots[i].isResident) {
                UploadToGPU(i, m_slots[i].backendHandle.get());
            }
        }
    }

    TextureManager::~TextureManager() {
        m_slots.clear();

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
            EResourceLayout::Undefined,
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

        // encoder->TransitionTexture(*texture, EResourceLayout::ShaderReadOnlyOptimal, EPipelineStageFlags::FragmentShaderBit);

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
        m_bindlessTextureSet->UpdateTexture(0, slotIdx, *m_slots[0].backendHandle);
        m_bindlessTextureSet->Apply();
    }
}
