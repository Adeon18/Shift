//
// Created by otrush on 10/28/2025.
//

#ifndef SHIFT_SRHI_HPP
#define SHIFT_SRHI_HPP

#include <concepts>

#include "Types.hpp"
#include "Texture.hpp"
#include "Buffer.hpp"
#include "Pipeline.hpp"
#include "Sampler.hpp"
#include "Swapchain.hpp"
#include "RenderPass.hpp"

namespace Shift {
    namespace RHI {
        struct Vulkan {
            static constexpr const char* Name = "Vulkan";
            // static constexpr bool SupportsBindless = true;
        };

        struct DX12 {
            static constexpr const char* Name = "DX12";
        };
    } // RHI

    // For now:D
    template<typename T>
    concept ValidAPI = std::same_as<T, RHI::Vulkan>;

    //! Forward dec for further per API local constructs (device, instance, some descriptor specific stuff)
    template<ValidAPI API> struct RHILocal {};

} // Shift

#ifdef SHIFT_VULKAN_BACKEND
#include "Vulkan/VKRHI_Impl.hpp"
#include "RHILocal_VK.hpp"
#endif

namespace Shift {
    template<ValidAPI API>
    class RenderHardwareInterface {
    public:
        bool Init(GLFWwindow* window, uint32_t width, uint32_t height, const std::string& appName, const std::string& appVersion, const std::string& engineName, const std::string& engineVersion);

        [[nodiscard]] Buffer CreateBuffer(const BufferDescriptor& desc);
        [[nodiscard]] Texture CreateTexture(const TextureDescriptor& desc);
        [[nodiscard]] Pipeline CreatePipeline(const PipelineDescriptor& desc, const std::vector<ShaderStageDesc>& shaders);
        [[nodiscard]] ResourceSet CreateResourceSet(const PipelineLayoutDescriptor& desc);
        [[nodiscard]] Sampler CreateSampler(const SamplerDescriptor& desc);
        [[nodiscard]] Shader CreateShader(const ShaderDescriptor& desc);

        //! TODO: [FEATURE] Here we would have additional parameters for multicommand-buffer concurrency handling (probably)
        [[nodiscard]] bool BeginCmds() const;

        [[nodiscard]] bool EndCmds() const;

        void ResetCmds() const;

        bool SubmitCmds() const;
        bool SubmitCmdsAndWait() const;

        void BeginRenderPass(const RenderPassDescriptor& desc);

        void EndRenderPass();

        ///! ------------------- Copy Buffer Commands ------------------- !///

        //! Copy buffer data to another buffer
        //! \param srcBuf buffer + offset into the buffer
        //! \param dstBuf buffer + offset into the buffer
        //! \param size size to copy
        void CopyBufferToBuffer(const BufferOpDescriptor& srcBuf, const BufferOpDescriptor& dstBuf, uint32_t size) const;

        //! Copy buffer data to a texture
        //! \param srcBuf buffer + offset into the buffer
        //! \param srcTex texture + size to copy + offset + subresource range
        void CopyBufferToTexture(const BufferOpDescriptor& srcBuf, const TextureCopyDescriptor& dstTex) const;

        ///! ------------------- Rendering Buffer Commands ------------------- !///

        //! Bind a single vertex buffer
        //! \param buffers buffer + offset into the buffer
        //! \param bindIdx bind idx
        void BindVertexBuffer(const BufferOpDescriptor& buffer, uint32_t bindIdx) const;

        //! Bind a range of vertex buffers
        //! \param buffers span of buffers + offsets into the buffers
        //! \param firstBind bind idx for the first buffer
        void BindVertexBuffers(std::span<BufferOpDescriptor> buffers, uint32_t firstBind) const;

        //! Bind an index buffer
        //! TODO: Add support for buffer indice sizes, current default and only type is uint16
        //! \param buffer buffer + offset into the buffer
        void BindIndexBuffer(const BufferOpDescriptor& buffer, EIndexSize indexSize) const;

        //! Bind the graphics pipeline
        //! \param pipeline The Pipeline wrapper
        void BindGraphicsPipeline(const Pipeline& pipeline) const;

        void DrawIndexed(const DrawIndexedConfig& drawConf) const;

        //! Draw/Draw instanced
        //! \param drawConf draw configuration
        void Draw(const DrawConfig& drawConf) const;

        ///! ------------------- Mics Buffer Commands ------------------- !///

        //! Blit the texture into the other texture
        //! \param srcTexture source texture with sizes and extents
        //! \param dstTexture destination texture with sizes and extents
        //! \param blitRegion blit operation description
        //! \param filter blit filter
        void BlitTexture(const TextureBlitData& srcTexture, const TextureBlitData& dstTexture, const TextureBlitRegion& blitRegion, EFilterMode filter) const;

        //! Set viewport, we don't support multiple
        //! \param viewport Viewport struct
        void SetViewport(Viewport viewport) const;

        //! Set scissor
        //! \param scissor scissor structure
        void SetScissor(Rect2D scissor) const;


    private:
        RHILocal<API> m_local;

        Swapchain m_swapchain{};

        uint32_t m_currentFrame = 0;
    };
} // Shift

#endif //SHIFT_SRHI_HPP