//
// Created by otrush on 10/28/2025.
//

#ifndef SHIFT_SRHI_HPP
#define SHIFT_SRHI_HPP

#include <atomic>

#include "RHIContext.hpp"

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

        RHIContext<API>& GetGraphicsContext() { return m_graphicsContexts[m_currentFrame]; }
        RHIContext<API>& GetComputeContext() { return m_computeContext; }
        RHIContext<API>& GetTransferContext() { return m_transferContext; }

        BinarySemaphore* GetSwapchainAcquireSemaphore(uint32_t imageIdx) { return &m_imageAvailable[imageIdx]; }
        BinarySemaphore* GetSwapchainRenderFinishedSemaphore(uint32_t imageIdx) { return &m_renderFinished[imageIdx]; }

        RHIContext<API>* AcquireSecondaryGraphicsContext();
        void ExecuteSecondaryGraphicsContexts();

        //! Waits until the swapchain binary semaprhores are released
        void WaitForGraphicsContext();

        RHIContext<API>::SubmitTimelinePayload GetTransferWaitPayload();
        RHIContext<API>::SubmitTimelinePayload ReserveTransferSignalPayload();

        RHIContext<API>::SubmitTimelinePayload GetGraphicsWaitPayload();
        RHIContext<API>::SubmitTimelinePayload ReserveGraphicsSignalPayload();


    private:
        RHILocal<API> m_local;

        std::array<RHIContext<API>, Conf::SHIFT_MAX_FRAMES_IN_FLIGHT> m_graphicsContexts;

        std::vector<RHIContext<API>> m_secondaryGraphicsContexts;
        std::mutex m_secondaryPoolMutex;

        // Single transfer/compute contexts (can be expanded to a pool)
        RHIContext<API> m_transferContext;
        RHIContext<API> m_computeContext;

        uint32_t m_currentFrame = 0;

        // Timeline semaphores per queue type
        TimelineSemaphore m_timelineGraphics;
        TimelineSemaphore m_timelineTransfer;
        TimelineSemaphore m_timelineCompute;

        std::atomic<uint64_t> m_timelineGraphicsValue{0};
        std::atomic<uint64_t> m_timelineTransferValue{0};
        std::atomic<uint64_t> m_timelineComputeValue{0};

        //! Swapchain-related semaphores
        std::vector<BinarySemaphore> m_imageAvailable;
        std::vector<BinarySemaphore> m_renderFinished;
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
#endif

        for (uint32_t i = 0; i < Conf::SHIFT_MAX_FRAMES_IN_FLIGHT; ++i) {
            CheckCritical(m_graphicsContexts[i].Init(&m_local, EContextType::Graphics, false), "Failed to create Graphics Context in flight!");
        }

        // TODO: [FEATURE] Pooled secondary contexts
        // m_secondaryGraphicsContexts.resize(Conf::MAX_SECONDARY_CONTEXTS);
        // for (auto& ctx : m_secondaryGraphicsContexts) {
        //     ctx.Init(&m_local, EContextType::Graphics, 0, true);
        // }

        // TODO: [FEATURE] Async COmpute COntext
        // Only 1 transfer queue for now
        // CheckCritical(m_computeContext.Init(&m_local, EContextType::Compute, false), "Failed to create Compute Context!");

        CheckCritical(m_transferContext.Init(&m_local, EContextType::Transfer, false), "Failed to create Transfer Context!");

        for (uint32_t i = 0; i < m_local.swapchain.GetImages().size(); ++i) {
            BinarySemaphore& sem = m_renderFinished.emplace_back();
            CheckCritical(sem.Init(&m_local.device), "Failed to create submit semaphore!");
            BinarySemaphore& sem2 = m_imageAvailable.emplace_back();
            CheckCritical(sem2.Init(&m_local.device), "Failed to create acqure image semaphore!");
        }

        // m_timelineCompute.Init(&m_local.device);
        m_timelineGraphics.Init(&m_local.device, 0);
        m_timelineTransfer.Init(&m_local.device, 0);

        return true;
    }

    template<ValidAPI API>
    void RenderHardwareInterface<API>::Destroy() {

        m_local.swapchain.Destroy();

        for (auto& sem: m_imageAvailable) {
            sem.Destroy();
        }
        for (auto& sem: m_renderFinished) {
            sem.Destroy();
        }

        m_timelineTransfer.Destroy();
        m_timelineGraphics.Destroy();

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
        return m_local.swapchain.AquireNextImage(m_imageAvailable[m_currentFrame], wasChanged);
    }

    template<ValidAPI API>
    uint32_t RenderHardwareInterface<API>::SwapchainPresent(uint32_t imageIdx, bool *isOld) {
        return m_local.swapchain.Present(m_renderFinished[imageIdx], imageIdx, isOld);
    }

    template<ValidAPI API>
    void RenderHardwareInterface<API>::WaitForGraphicsContext() {
        m_timelineGraphics.Wait(m_timelineGraphicsValue);
    }

    template<ValidAPI API>
    RHIContext<API>::SubmitTimelinePayload RenderHardwareInterface<API>::GetTransferWaitPayload() {
        return { &m_timelineTransfer, m_timelineTransferValue.load(std::memory_order_acquire) };
    }

    template<ValidAPI API>
    RHIContext<API>::SubmitTimelinePayload RenderHardwareInterface<API>::ReserveTransferSignalPayload() {
        uint64_t newVal = m_timelineTransferValue.fetch_add(1, std::memory_order_acq_rel) + 1;
        return { &m_timelineTransfer, newVal };
    }

    template<ValidAPI API>
    RHIContext<API>::SubmitTimelinePayload RenderHardwareInterface<API>::GetGraphicsWaitPayload() {
        return { &m_timelineGraphics, m_timelineGraphicsValue.load(std::memory_order_acquire) };
    }

    template<ValidAPI API>
    RHIContext<API>::SubmitTimelinePayload RenderHardwareInterface<API>::ReserveGraphicsSignalPayload() {
        uint64_t newVal = m_timelineGraphicsValue.fetch_add(1, std::memory_order_acq_rel) + 1;
        return { &m_timelineGraphics, newVal };
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

#ifdef SHIFT_VULKAN_BACKEND
    using SRHI = RenderHardwareInterface<RHI::Vulkan>;
    using SRHIContext = RHIContext<RHI::Vulkan>;
#endif
} // Shift

#endif //SHIFT_SRHI_HPP