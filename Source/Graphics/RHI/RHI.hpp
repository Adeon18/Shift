//
// Created by otrush on 10/28/2025.
//

#ifndef SHIFT_SRHI_HPP
#define SHIFT_SRHI_HPP

#include <concepts>
#include <array>

#include "Types.hpp"
#include "Texture.hpp"
#include "Buffer.hpp"
#include "Pipeline.hpp"
#include "Sampler.hpp"
#include "Swapchain.hpp"
#include "RenderPass.hpp"
#include "Config/EngineConfig.hpp"

#include "Utility/UtilStandard.hpp"
#include "Utility/Assertions.hpp"

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
#include "Utility/Vulkan/VKUtilRHI.hpp"
#include "Utility/Vulkan/VKUtilInfo.hpp"
#endif

namespace Shift {
    template<ValidAPI API>
    class RenderHardwareInterface {
    public:
        bool Init(GLFWwindow* window, uint32_t width, uint32_t height, const std::string& appName, const std::string& appVersion, const std::string& engineName, const std::string& engineVersion);

        //! Wait for GPU to complete work before deleting stuff
        void WaitForGPU();
        void Destroy();

        [[nodiscard]] Buffer CreateBuffer(const BufferDescriptor& desc);
        [[nodiscard]] Texture CreateTexture(const TextureDescriptor& desc);
        [[nodiscard]] Pipeline CreatePipeline(const PipelineDescriptor& desc, const std::vector<ShaderStageDesc>& shaders);
        [[nodiscard]] ResourceSet CreateResourceSet(const PipelineLayoutDescriptor& desc);
        [[nodiscard]] Sampler CreateSampler(const SamplerDescriptor& desc);
        [[nodiscard]] Shader CreateShader(const ShaderDescriptor& desc);

        [[nodiscard]] Swapchain& GetSwapchain() { return m_local.swapchain; }
        [[nodiscard]] uint32_t SwapchainAquireImage(bool* wasChanged);
        [[nodiscard]] uint32_t SwapchainPresent(uint32_t imageIdx, bool* isOld);
        void NextFrame() { m_currentFrame = (++m_currentFrame) % Conf::SHIFT_MAX_FRAMES_IN_FLIGHT; }
        uint32_t GetCurrentFrame() { return m_currentFrame; }

        //! TODO: [FEATURE] Here we would have additional parameters for multicommand-buffer concurrency handling (probably)
        [[nodiscard]] bool BeginCmds() const;

        [[nodiscard]] bool EndCmds() const;

        void ResetCmds() const;

        bool SubmitCmds(uint32_t imageIdx) const;
        bool SubmitCmdsAndWait(uint32_t imageIdx) const;

        void BeginRenderPass(const RenderPassDescriptor& desc, std::span<Texture*> colorTextures, std::optional<Texture*> depthTexture);
        void BeginRenderPassToSwapchain(const RenderPassDescriptor& desc, uint32_t imageIdx, std::optional<Texture*> depthTexture);

        void EndRenderPass();

        ///! ------------------- Copy Buffer Commands ------------------- !///

        //! Copy buffer data to another buffer
        //! \param srcBuf buffer + offset into the buffer
        //! \param dstBuf buffer + offset into the buffer
        //! \param size size to copy
        void CopyBufferToBuffer(const BufferOpDescriptor& srcBuf, const BufferOpDescriptor& dstBuf, uint32_t size) const;

        //! Copy buffer data to a texture
        //! \param srcBuf buffer + offset into the buffer
        //! \param dstTex texture + size to copy + offset + subresource range
        void CopyBufferToTexture(const BufferOpDescriptor& srcBuf, const TextureCopyDescriptor& dstTex) const;

        ///! ------------------- Rendering Buffer Commands ------------------- !///

        //! Bind a single vertex buffer
        //! \param buffer buffer + offset into the buffer
        //! \param bindIdx bind idx
        void BindVertexBuffer(const BufferOpDescriptor& buffer, uint32_t bindIdx) const;

        //! Bind a range of vertex buffers
        //! \param buffers span of buffers + offsets into the buffers
        //! \param firstBind bind idx for the first buffer
        void BindVertexBuffers(std::span<BufferOpDescriptor> buffers, uint32_t firstBind) const;

        //! Bind an index buffer
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
        void SetViewport(const Viewport& viewport) const;

        //! Set scissor
        //! \param scissor scissor structure
        void SetScissor(const Rect2D& scissor) const;

        void TransitionTexture(const Texture& texture, EResourceLayout newLayout, EPipelineStageFlags newStageFlags);

        void TransitionSwapchainTexture(uint32_t imageIdx, EResourceLayout newLayout, EPipelineStageFlags newStageFlags);

    private:
        RHILocal<API> m_local;

        std::array<CommandBuffer, Conf::SHIFT_MAX_FRAMES_IN_FLIGHT> m_cmdBuffersFlight;
        std::vector<CommandBuffer> m_cmdBuffersTransfer;
        //! TODO [DX12] My ass has a feeling that DX12 does not do this
        std::array<Semaphore, Conf::SHIFT_MAX_FRAMES_IN_FLIGHT> m_imgAvailableSemaphores;
        //! Since the new VK validation layer spec you now have to ensure that the submit semaphores are per swapchain image
        std::vector<Semaphore> m_renderFinishedSemaphores;

        uint32_t m_currentFrame = 0;
    };

    template<ValidAPI API>
    bool RenderHardwareInterface<API>::Init(GLFWwindow *window, uint32_t width, uint32_t height,
        const std::string &appName, const std::string &appVersion, const std::string &engineName,
        const std::string &engineVersion)
    {
        auto getVersionUintFromString = [](const std::string& str) {
            std::vector<std::string_view> tokens;
            Util::StrSplitView(str, '.', &tokens);
            if (tokens.size() != 3) {
                tokens.resize(3);
                Log(Warning, "Version has to have 3 version codes split by '.'. Defaulting to 1.0.0");
                tokens[0] = "1"; tokens[1] = "0"; tokens[2] = "0";
            }
            uint32_t uMajor; Util::StrToUint32Fast(tokens[0], uMajor);
            uint32_t uMinor; Util::StrToUint32Fast(tokens[1], uMinor);
            uint32_t uPatch; Util::StrToUint32Fast(tokens[2], uPatch);

            return std::tuple{uMajor, uMinor, uPatch};
        };

        auto [uAppMajor, uAppMinor, uAppPatch] = getVersionUintFromString(appVersion);

        uint32_t uAppVersion = 0;
#ifdef SHIFT_VULKAN_BACKEND
        uAppVersion = VK_MAKE_VERSION(uAppMajor, uAppMinor, uAppPatch);
#endif

        auto [uEngMajor, uEngMinor, uEngPatch] = getVersionUintFromString(engineVersion);

        uint32_t uEngVersion = 0;
#ifdef SHIFT_VULKAN_BACKEND
        uEngVersion = VK_MAKE_VERSION(uEngMajor, uEngMinor, uEngPatch);
#endif

#ifdef SHIFT_VULKAN_BACKEND
        CheckCritical(m_local.instance.Init(appName, uAppVersion, engineName, uEngVersion), "Failed to create VK instance!");
        CheckCritical(m_local.surface.Init(m_local.instance.Get(), window), "Failed to create VK surface!");
        //! TODO: Features (features could be pulled from API template arg, for now they are just default
        CheckCritical(m_local.device.Init(m_local.instance, m_local.surface.Get()), "Failed to create VK device!");
        m_local.cmdPoolStorage.Init(&m_local.device, &m_local.instance);
        m_local.descLayoutCache.Init(&m_local.device);
        CheckCritical(m_local.descAllocator.Init(&m_local.device), "Failed to create VK descriptor allocator!");
        CheckCritical(m_local.swapchain.Init(&m_local.device, &m_local.surface, width, height), "Failed to create VK swapchain!");
        for (uint32_t i = 0; i < Conf::SHIFT_MAX_FRAMES_IN_FLIGHT; ++i) {
            CheckCritical(m_cmdBuffersFlight[i].Init(&m_local.device, &m_local.instance, m_local.cmdPoolStorage.GetGraphics(), EPoolQueueType::Graphics), "Failed to create VK command buffer in flight!");
        }

        // Only 1 transfer queue for now
        CommandBuffer b;
        CheckCritical(b.Init(&m_local.device, &m_local.instance, m_local.cmdPoolStorage.GetTransfer(), EPoolQueueType::Transfer), "Failed to create VK command buffer for tranfer!");
        m_cmdBuffersTransfer.push_back(b);

        CommandBuffer b2;
        CheckCritical(b2.Init(&m_local.device, &m_local.instance, m_local.cmdPoolStorage.GetTransfer(), EPoolQueueType::Transfer), "Failed to create VK command buffer for tranfer 2!");
        m_cmdBuffersTransfer.push_back(b2);
#endif

        for (uint32_t i = 0; i < Conf::SHIFT_MAX_FRAMES_IN_FLIGHT; ++i) {
            CheckCritical(m_imgAvailableSemaphores[i].Init(&m_local.device), "Failed to create image available semaphore!");
        }
        for (uint32_t i = 0; i < m_local.swapchain.GetImages().size(); ++i) {
            Semaphore& sem = m_renderFinishedSemaphores.emplace_back();
            CheckCritical(sem.Init(&m_local.device), "Failed to create submit semaphore!");
        }


        return true;
    }

    template<ValidAPI API>
    void RenderHardwareInterface<API>::Destroy() {

        m_local.swapchain.Destroy();

        for (auto& sem: m_imgAvailableSemaphores) {
            sem.Destroy();
        }
        for (auto& sem: m_renderFinishedSemaphores) {
            sem.Destroy();
        }

        //! cmd Destroy just destroys the fence
        for (auto& cmd: m_cmdBuffersFlight) {
            cmd.Destroy();
        }
        for (auto& cmd: m_cmdBuffersTransfer) {
            cmd.Destroy();
        }

        m_local.descLayoutCache.Destroy();
        m_local.descAllocator.Destroy();

        m_local.cmdPoolStorage.Destroy();
        m_local.surface.Destroy();

        m_local.device.Destroy();
        m_local.instance.Destroy();
    }

    template<ValidAPI API>
    Buffer RenderHardwareInterface<API>::CreateBuffer(const BufferDescriptor &desc) {
        Buffer b;
        b.Init(&m_local.device, desc);
        return b;
    }

    template<ValidAPI API>
    Texture RenderHardwareInterface<API>::CreateTexture(const TextureDescriptor &desc) {
        Texture t;
        t.Init(&m_local.device, desc);
        return t;
    }

    template<ValidAPI API>
    Sampler RenderHardwareInterface<API>::CreateSampler(const SamplerDescriptor &desc) {
        Sampler s;
        s.Init(&m_local.device, desc);

        return s;
    }

    template<ValidAPI API>
    Shader RenderHardwareInterface<API>::CreateShader(const ShaderDescriptor &desc) {
        Shader s;
        s.Init(&m_local.device, desc);

        return s;
    }

    template<ValidAPI API>
    uint32_t RenderHardwareInterface<API>::SwapchainAquireImage(bool *wasChanged) {
        return m_local.swapchain.AquireNextImage(m_imgAvailableSemaphores[m_currentFrame], wasChanged);
    }

    template<ValidAPI API>
    uint32_t RenderHardwareInterface<API>::SwapchainPresent(uint32_t imageIdx, bool *isOld) {
        return m_local.swapchain.Present(m_renderFinishedSemaphores[imageIdx], imageIdx, isOld);
    }

    template<ValidAPI API>
    bool RenderHardwareInterface<API>::BeginCmds() const {
        if (!m_cmdBuffersFlight[m_currentFrame].IsAvailable()) {
            m_cmdBuffersFlight[m_currentFrame].Wait();
        }
        m_cmdBuffersFlight[m_currentFrame].Reset();
        return m_cmdBuffersFlight[m_currentFrame].Begin();
    }

    template<ValidAPI API>
    bool RenderHardwareInterface<API>::EndCmds() const {
        return m_cmdBuffersFlight[m_currentFrame].End();
    }

    template<ValidAPI API>
    void RenderHardwareInterface<API>::ResetCmds() const {
        return m_cmdBuffersFlight[m_currentFrame].Reset();
    }

    template<ValidAPI API>
    bool RenderHardwareInterface<API>::SubmitCmds(uint32_t imageIdx) const {
        return m_cmdBuffersFlight[m_currentFrame].Submit(m_imgAvailableSemaphores[m_currentFrame], m_renderFinishedSemaphores[imageIdx]);
    }

    template<ValidAPI API>
    bool RenderHardwareInterface<API>::SubmitCmdsAndWait(uint32_t imageIdx) const {
        return m_cmdBuffersFlight[m_currentFrame].SubmitAndWait(m_imgAvailableSemaphores[m_currentFrame], m_renderFinishedSemaphores[imageIdx]);
    }


    template<ValidAPI API>
    void RenderHardwareInterface<API>::EndRenderPass() {
#ifdef SHIFT_VULKAN_BACKEND
        m_cmdBuffersFlight[m_currentFrame].VK_EndRenderPass();
#endif
    }

    template<ValidAPI API>
    void RenderHardwareInterface<API>::CopyBufferToBuffer(const BufferOpDescriptor &srcBuf,
        const BufferOpDescriptor &dstBuf, uint32_t size) const {

        m_cmdBuffersTransfer[m_currentFrame].Reset();
        m_cmdBuffersTransfer[m_currentFrame].Begin();
        m_cmdBuffersTransfer[m_currentFrame].CopyBufferToBuffer(srcBuf, dstBuf, size);
        m_cmdBuffersTransfer[m_currentFrame].End();
        m_cmdBuffersTransfer[m_currentFrame].SubmitAndWait();
    }

    template<ValidAPI API>
    void RenderHardwareInterface<API>::CopyBufferToTexture(const BufferOpDescriptor &srcBuf,
        const TextureCopyDescriptor &dstTex) const {
        m_cmdBuffersTransfer[m_currentFrame].Reset();
        m_cmdBuffersTransfer[m_currentFrame].Begin();
        m_cmdBuffersTransfer[m_currentFrame].CopyBufferToTexture(srcBuf, dstTex);
        m_cmdBuffersTransfer[m_currentFrame].End();
        m_cmdBuffersTransfer[m_currentFrame].SubmitAndWait();
    }

    template<ValidAPI API>
    void RenderHardwareInterface<API>::BindVertexBuffer(const BufferOpDescriptor &buffer, uint32_t bindIdx) const {
        m_cmdBuffersFlight[m_currentFrame].BindVertexBuffer(buffer, bindIdx);
    }

    template<ValidAPI API>
    void RenderHardwareInterface<API>::BindVertexBuffers(std::span<BufferOpDescriptor> buffers,
        uint32_t firstBind) const {
        m_cmdBuffersFlight[m_currentFrame].BindVertexBuffers(buffers, firstBind);
    }

    template<ValidAPI API>
    void RenderHardwareInterface<API>::BindIndexBuffer(const BufferOpDescriptor &buffer, EIndexSize indexSize) const {
        m_cmdBuffersFlight[m_currentFrame].BindIndexBuffer(buffer, indexSize);
    }

    template<ValidAPI API>
    void RenderHardwareInterface<API>::BindGraphicsPipeline(const Pipeline &pipeline) const {
        m_cmdBuffersFlight[m_currentFrame].BindGraphicsPipeline(pipeline);
    }

    template<ValidAPI API>
    void RenderHardwareInterface<API>::DrawIndexed(const DrawIndexedConfig &drawConf) const {
        m_cmdBuffersFlight[m_currentFrame].DrawIndexed(drawConf);
    }

    template<ValidAPI API>
    void RenderHardwareInterface<API>::Draw(const DrawConfig &drawConf) const {
        m_cmdBuffersFlight[m_currentFrame].Draw(drawConf);
    }

    template<ValidAPI API>
    void RenderHardwareInterface<API>::BlitTexture(const TextureBlitData &srcTexture, const TextureBlitData &dstTexture,
        const TextureBlitRegion &blitRegion, EFilterMode filter) const
    {
        m_cmdBuffersFlight[m_currentFrame].BlitTexture(srcTexture, dstTexture, blitRegion, filter);
    }

    template<ValidAPI API>
    void RenderHardwareInterface<API>::SetViewport(const Viewport& viewport) const {
        m_cmdBuffersFlight[m_currentFrame].SetViewport(viewport);
    }

    template<ValidAPI API>
    void RenderHardwareInterface<API>::SetScissor(const Rect2D& scissor) const {
        m_cmdBuffersFlight[m_currentFrame].SetScissor(scissor);
    }



    //! ------------------------------- Vulkan Specific -------------------------------

    template<>
    inline void RenderHardwareInterface<RHI::Vulkan>::WaitForGPU() {
        vkDeviceWaitIdle(m_local.device.Get());
    }

    template<>
    inline Pipeline RenderHardwareInterface<RHI::Vulkan>::CreatePipeline(const PipelineDescriptor &desc,
        const std::vector<ShaderStageDesc> &shaders)
    {
        Pipeline p;

        std::vector<VkDescriptorSetLayout> setLayouts;
        setLayouts.reserve(desc.descriptorLayouts.size());

        // For each layout, create the descriptor set layout
        for (const auto& layoutDesc : desc.descriptorLayouts) {
            std::vector<VkDescriptorSetLayoutBinding> vkBindings;
            vkBindings.reserve(layoutDesc.bindings.size());

            for (const auto& b : layoutDesc.bindings) {
                VkDescriptorSetLayoutBinding binding{};
                binding.binding = b.binding;
                binding.descriptorCount = b.count;
                binding.stageFlags = VK::Util::ShiftToVKBindingVisibility(b.stageFlags);
                binding.descriptorType = VK::Util::ShiftToVKBindingType(b.type);
                binding.pImmutableSamplers = nullptr; // handle immutable samplers if needed
                vkBindings.push_back(binding);
            }

            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = static_cast<uint32_t>(vkBindings.size());
            layoutInfo.pBindings = vkBindings.data();

            setLayouts.push_back(m_local.descLayoutCache.CreateDescriptorLayout(layoutInfo));
        }

        p.Init(&m_local.device, desc, shaders, setLayouts);

        return p;
    }

    template<>
    inline ResourceSet RenderHardwareInterface<RHI::Vulkan>::CreateResourceSet(const PipelineLayoutDescriptor &desc) {
        ResourceSet rs;

        //! TODO [CLEANUP] create a shared function for set pulling of set layout between this and pipeline creation
        std::vector<VkDescriptorSetLayoutBinding> vkBindings;
        vkBindings.reserve(desc.bindings.size());

        for (const auto& b : desc.bindings) {
            VkDescriptorSetLayoutBinding binding{};
            binding.binding = b.binding;
            binding.descriptorCount = b.count;
            binding.stageFlags = VK::Util::ShiftToVKBindingVisibility(b.stageFlags);
            binding.descriptorType = VK::Util::ShiftToVKBindingType(b.type);
            binding.pImmutableSamplers = nullptr; // handle immutable samplers if needed
            vkBindings.push_back(binding);
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(vkBindings.size());
        layoutInfo.pBindings = vkBindings.data();

        rs.Init(&m_local.device, m_local.descAllocator.Allocate(m_local.descLayoutCache.CreateDescriptorLayout(layoutInfo)));

        return rs;
    }


    template<>
    inline void RenderHardwareInterface<RHI::Vulkan>::BeginRenderPass(const RenderPassDescriptor& desc, std::span<Texture*> colorTextures, std::optional<Texture*> depthTexture) {

        assert(desc.colorAttachments.size() == colorTextures.size());
        assert(desc.depthAttachment.has_value() == depthTexture.has_value());

        std::vector<VkRenderingAttachmentInfo> colorInfo;
        std::optional<VkRenderingAttachmentInfo> depthInfo;
        for (uint32_t i = 0; i < desc.colorAttachments.size(); i++) {
            const Texture* colTex = colorTextures[i];
            const RenderPassDescriptor::RenderPassAttachmentInfo& att = desc.colorAttachments[i];
            colorInfo.push_back(VK::Util::CreateRenderingAttachmentInfo(
                    colTex->GetView(),
                    VK::Util::ShiftToVKResourceLayout(colTex->GetResourceLayout()),
                    VK::Util::ShiftToVKClearColor(att.clearValue),
                    VK::Util::ShiftToVKAttachmentLoadOperation(att.loadOperation),
                    VK::Util::ShiftToVKAttachmentStoreOperation(att.storeOperation)
                )
            );
        }

        if (desc.depthAttachment.has_value()) {
            const RenderPassDescriptor::RenderPassAttachmentInfo& att = desc.depthAttachment.value();
            *depthInfo = VK::Util::CreateRenderingAttachmentInfo(
                    (*depthTexture)->GetView(),
                    VK::Util::ShiftToVKResourceLayout((*depthTexture)->GetResourceLayout()),
                    VK::Util::ShiftToVKClearDepthStencil(att.clearValue),
                    VK::Util::ShiftToVKAttachmentLoadOperation(att.loadOperation),
                    VK::Util::ShiftToVKAttachmentStoreOperation(att.storeOperation)
            );
        }

        VkRenderingInfoKHR renderInfo{};
        renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
        renderInfo.renderArea = {.offset = VK::Util::ShiftToVKOffset2D(desc.offset), .extent = VK::Util::ShiftToVKExtent2D(desc.extent)};
        renderInfo.layerCount = 1;
        renderInfo.colorAttachmentCount = 1;
        renderInfo.pColorAttachments = colorInfo.data();
        if (depthInfo.has_value()) {
            renderInfo.pDepthAttachment = &depthInfo.value();
        }

        m_cmdBuffersFlight[m_currentFrame].VK_BeginRenderPass(renderInfo);
    }

    template<>
    inline void RenderHardwareInterface<RHI::Vulkan>::BeginRenderPassToSwapchain(const RenderPassDescriptor& desc, uint32_t imageIdx, std::optional<Texture*> depthTexture) {

        assert(desc.depthAttachment.has_value() == depthTexture.has_value());

        std::vector<VkRenderingAttachmentInfo> colorInfo;
        std::optional<VkRenderingAttachmentInfo> depthInfo;
        const RenderPassDescriptor::RenderPassAttachmentInfo& att = desc.colorAttachments[0];
        colorInfo.push_back(VK::Util::CreateRenderingAttachmentInfo(
                m_local.swapchain.GetImageViews()[imageIdx],
                m_local.swapchain.GetImageLayouts()[imageIdx],
                VK::Util::ShiftToVKClearColor(att.clearValue),
                VK::Util::ShiftToVKAttachmentLoadOperation(att.loadOperation),
                VK::Util::ShiftToVKAttachmentStoreOperation(att.storeOperation)
            )
        );

        if (desc.depthAttachment.has_value()) {
            const RenderPassDescriptor::RenderPassAttachmentInfo& att = desc.depthAttachment.value();
            *depthInfo = VK::Util::CreateRenderingAttachmentInfo(
                    (*depthTexture)->GetView(),
                    VK::Util::ShiftToVKResourceLayout((*depthTexture)->GetResourceLayout()),
                    VK::Util::ShiftToVKClearDepthStencil(att.clearValue),
                    VK::Util::ShiftToVKAttachmentLoadOperation(att.loadOperation),
                    VK::Util::ShiftToVKAttachmentStoreOperation(att.storeOperation)
            );
        }

        VkRenderingInfoKHR renderInfo{};
        renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
        renderInfo.renderArea = {.offset = VK::Util::ShiftToVKOffset2D(desc.offset), .extent = VK::Util::ShiftToVKExtent2D(desc.extent)};
        renderInfo.layerCount = 1;
        renderInfo.colorAttachmentCount = 1;
        renderInfo.pColorAttachments = colorInfo.data();
        if (depthInfo.has_value()) {
            renderInfo.pDepthAttachment = &depthInfo.value();
        }

        m_cmdBuffersFlight[m_currentFrame].VK_BeginRenderPass(renderInfo);

    }


    //! Big TODO for now: This only works for the graphics queue
    template<>
    inline void RenderHardwareInterface<RHI::Vulkan>::TransitionTexture(const Texture &texture, EResourceLayout newLayout,
        EPipelineStageFlags newStageFlags)
    {

        VkImageSubresourceRange subresourceRange{};
        subresourceRange.aspectMask = VK::Util::ShiftToVKTextureAspect(texture.GetAspect());
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = 1;

        m_cmdBuffersFlight[m_currentFrame].VK_TransferImageLayout(
            texture.GetImage(),
            VK::Util::ShiftToVKResourceLayout(texture.GetResourceLayout()),
            VK::Util::ShiftToVKResourceLayout(newLayout),
            texture.VK_GetStageFlags(),
            VK::Util::ShiftToVKPipelineStageFlags(newStageFlags)
        );

        texture.SetResourceLayout(newLayout);
        texture.VK_SetStageFlags(VK::Util::ShiftToVKPipelineStageFlags(newStageFlags));
    }

    template<>
    inline void RenderHardwareInterface<RHI::Vulkan>::TransitionSwapchainTexture(uint32_t imageIdx, EResourceLayout newLayout,
        EPipelineStageFlags newStageFlags)
    {
        VkImageSubresourceRange subresourceRange{};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = 1;

        m_cmdBuffersFlight[m_currentFrame].VK_TransferImageLayout(
            m_local.swapchain.GetImages()[imageIdx],
            m_local.swapchain.GetImageLayouts()[imageIdx],
            VK::Util::ShiftToVKResourceLayout(newLayout),
            m_local.swapchain.GetImageStageFlags()[imageIdx],
            VK::Util::ShiftToVKPipelineStageFlags(newStageFlags)
        );

        m_local.swapchain.GetImageLayouts()[imageIdx] = VK::Util::ShiftToVKResourceLayout(newLayout);
        m_local.swapchain.GetImageStageFlags()[imageIdx] = VK::Util::ShiftToVKPipelineStageFlags(newStageFlags);
    }
} // Shift

#endif //SHIFT_SRHI_HPP