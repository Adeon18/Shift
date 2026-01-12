//
// Created by otrush on 10/28/2025.
//

#ifndef SHIFT_SRHI_HPP
#define SHIFT_SRHI_HPP

#include <atomic>

#include "RHIContext.hpp"
#include "RHIDeferredExecutor.hpp"
#include "../Managers/ShaderManager.hpp"

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
        [[nodiscard]] Pipeline* CreatePipeline(const PipelineDescriptor& desc, const std::vector<ShaderStageDesc>& shaders);
        [[nodiscard]] ResourceSet CreateResourceSet(const PipelineLayoutDescriptor& desc);
        [[nodiscard]] Sampler CreateSampler(const SamplerDescriptor& desc);
        [[nodiscard]] Shader* CreateShader(const ShaderDescriptor& desc);

        [[nodiscard]] Swapchain& GetSwapchain() { return m_local.swapchain; }
        [[nodiscard]] uint32_t SwapchainAquireImage(bool* wasChanged);
        [[nodiscard]] bool SwapchainPresent(uint32_t imageIdx, bool* isOld);
        [[nodiscard]] bool ResizeSwapchain(uint32_t width, uint32_t height);

        void EndFrame();

        uint32_t GetCurrentFrame() { return m_currentFrame; }
        uint32_t GetCurrentGlobalIndex() { return m_currentFrameGlobalIndex; }

        RHIContext<API>& GetGraphicsContext() { return m_graphicsContexts[m_currentFrame]; }
        RHIContext<API>& GetComputeContext() { return m_computeContext; }
        RHIContext<API>& GetTransferContext() { return m_transferContext; }

        BinarySemaphore* GetSwapchainAcquireSemaphore(uint32_t imageIdx) { return &m_imageAvailable[imageIdx]; }
        BinarySemaphore* GetSwapchainRenderFinishedSemaphore(uint32_t imageIdx) { return &m_renderFinished[imageIdx]; }

        RHIContext<API>* AcquireSecondaryGraphicsContext();
        void ExecuteSecondaryGraphicsContexts(std::span<RHIContext<API>*> secondaries, RHIContext<API>::SubmitTimelinePayload releasePayload);

        //! Waits until the swapchain binary semaprhores are released
        void WaitForGraphicsContext();

        void WaitForImagePresent(uint32_t imageIndex);

        RHIContext<API>::SubmitTimelinePayload GetTransferWaitPayload();
        RHIContext<API>::SubmitTimelinePayload ReserveTransferSignalPayload();

        RHIContext<API>::SubmitTimelinePayload GetGraphicsWaitPayload();
        RHIContext<API>::SubmitTimelinePayload ReserveGraphicsSignalPayload();

        void DeferExecuteEndOfSession(RHIDeferredExecutor::Callback fn);
        void DeferExecute(TimelineSemaphore* sem, uint64_t waitVal, RHIDeferredExecutor::Callback fn);

        void DeferExecuteToFrame(uint64_t frameIndexGlobal, RHIDeferredExecutor::Callback fn);

        void ProcessDeferredCallbacks();

        void ShaderHotReload();

        [[nodiscard]] const RHILocal<API>& GetLocal() const { return m_local; }

    private:
        RHILocal<API> m_local;

        std::array<RHIContext<API>, Conf::SHIFT_MAX_FRAMES_IN_FLIGHT> m_graphicsContexts;
        struct SecondaryContextData {
            bool inUse = false;
            bool submittedToPrimary = false;
            uint32_t id = 0;
        };

        std::array<std::vector<RHIContext<API>>, Conf::SHIFT_MAX_FRAMES_IN_FLIGHT> m_secondaryGraphicsContexts;
        std::unordered_map<RHIContext<API>*, SecondaryContextData> m_secondaryGraphicsContextData;
        std::mutex m_secondaryPoolMutex;

        // Single transfer/compute contexts (can be expanded to a pool)
        RHIContext<API> m_transferContext;
        RHIContext<API> m_computeContext;

        RHIDeferredExecutor m_deferredExecutor;
        Graphics::ShaderManager m_shaderManager;
        //! TODO: [Architecture] This will be replaced with pipeline manager someday
        std::unordered_set<Pipeline*> m_pipelines;

        uint32_t m_currentFrame = 0;
        uint64_t m_currentFrameGlobalIndex = 0;

        //! Timeline semaphores per queue type
        TimelineSemaphore m_timelineGraphics;
        TimelineSemaphore m_timelineTransfer;
        TimelineSemaphore m_timelineCompute;

        std::atomic<uint64_t> m_timelineGraphicsValue{0};
        std::atomic<uint64_t> m_timelineTransferValue{0};
        std::atomic<uint64_t> m_timelineComputeValue{0};

        //! To be able to wait on presentation
        std::vector<Fence> m_presentFences;

        //! This is wrong implementation, with this we can wait until pixels appear in screen but NOT for presentation
        // std::vector<uint64_t> m_imagePresentIds;
        // uint64_t m_globalPresentId = 0;

        //! Swapchain-related semaphores
        std::array<BinarySemaphore, Conf::SHIFT_MAX_FRAMES_IN_FLIGHT> m_imageAvailable;
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
        CheckCritical(m_local.instance.Init(appName, uAppVersion, engineName, uEngVersion, API::requiredFeatures), "Failed to create VK instance!");
        CheckCritical(m_local.surface.Init(m_local.instance.Get(), window), "Failed to create VK surface!");
        //! TODO: Features (features could be pulled from API template arg, for now they are just default
        CheckCritical(m_local.device.Init(m_local.instance, m_local.surface.Get(), API::requiredFeatures), "Failed to create VK device!");
        m_local.descLayoutCache.Init(&m_local.device);
        CheckCritical(m_local.descAllocator.Init(&m_local.device), "Failed to create VK descriptor allocator!");
        CheckCritical(m_local.swapchain.Init(&m_local.device, &m_local.surface, width, height), "Failed to create VK swapchain!");
#endif

        for (uint32_t i = 0; i < Conf::SHIFT_MAX_FRAMES_IN_FLIGHT; ++i) {
            CheckCritical(m_graphicsContexts[i].Init(&m_local, EContextType::Graphics, false), "Failed to create Graphics Context in flight!");
        }

        for (uint32_t i = 0; i < Conf::SHIFT_MAX_FRAMES_IN_FLIGHT; ++i) {
            m_secondaryGraphicsContexts[i].resize(Conf::MAX_SECONDARY_CONTEXTS);
            for (uint32_t j = 0; j < m_secondaryGraphicsContexts[i].size(); ++j) {
                m_secondaryGraphicsContexts[i][j].Init(&m_local, EContextType::Graphics, true);
                m_secondaryGraphicsContextData[&m_secondaryGraphicsContexts[i][j]].inUse = false;
                m_secondaryGraphicsContextData[&m_secondaryGraphicsContexts[i][j]].submittedToPrimary = false;
                m_secondaryGraphicsContextData[&m_secondaryGraphicsContexts[i][j]].id = Conf::MAX_SECONDARY_CONTEXTS * i + j;
            }
        }

        CheckCritical(m_computeContext.Init(&m_local, EContextType::Compute, false), "Failed to create Compute Context!");

        CheckCritical(m_transferContext.Init(&m_local, EContextType::Transfer, false), "Failed to create Transfer Context!");

        uint32_t imageCount = m_local.swapchain.GetImages().size();
        m_renderFinished.clear();
        for(uint32_t i = 0; i < imageCount; i++) {
            auto& sem = m_renderFinished.emplace_back();
            CheckCritical(sem.Init(&m_local.device), "Failed to create render semaphore");
            auto& fence = m_presentFences.emplace_back();
            CheckCritical(fence.Init(&m_local.device, true), "Failed to create present wait fence");
        }

        for (uint32_t i = 0; i < Conf::SHIFT_MAX_FRAMES_IN_FLIGHT; ++i) {
            CheckCritical(m_imageAvailable[i].Init(&m_local.device), "Failed Init Acquire Sem");
        }

        // m_imagePresentIds.clear();
        // m_imagePresentIds.resize(m_local.swapchain.GetImages().size(), 0);
        // m_globalPresentId = 0;

        m_timelineCompute.Init(&m_local.device, 0);
        m_timelineGraphics.Init(&m_local.device, 0);
        m_timelineTransfer.Init(&m_local.device, 0);

        m_shaderManager.Init(&m_local.device, Util::GetShiftShaderRootDir());

        return true;
    }

    template<ValidAPI API>
    void RenderHardwareInterface<API>::Destroy() {

        m_deferredExecutor.FlushAllDeferredCallbacks();

        m_shaderManager.Destroy();

        m_local.swapchain.Destroy();

        for (auto& sem: m_imageAvailable) {
            sem.Destroy();
        }
        for (auto& sem: m_renderFinished) {
            sem.Destroy();
        }

        for (auto& fence: m_presentFences) {
            fence.Destroy();
        }

        m_timelineTransfer.Destroy();
        m_timelineGraphics.Destroy();
        m_timelineCompute.Destroy();

        for (uint32_t i = 0; i < Conf::SHIFT_MAX_FRAMES_IN_FLIGHT; ++i) {
            m_graphicsContexts[i].Destroy();
        }

        for (uint32_t i = 0; i < Conf::SHIFT_MAX_FRAMES_IN_FLIGHT; ++i) {
            for (auto& ctx : m_secondaryGraphicsContexts[i]) {
                ctx.Destroy();
            }
        }

        m_transferContext.Destroy();
        m_computeContext.Destroy();

        m_local.descLayoutCache.Destroy();
        m_local.descAllocator.Destroy();

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
    Shader* RenderHardwareInterface<API>::CreateShader(const ShaderDescriptor &desc) {
        return m_shaderManager.GetShader(desc);
    }

    template<ValidAPI API>
    uint32_t RenderHardwareInterface<API>::SwapchainAquireImage(bool *wasChanged) {
        return m_local.swapchain.AquireNextImage(m_imageAvailable[m_currentFrame], wasChanged);
    }

    template<ValidAPI API>
    bool RenderHardwareInterface<API>::ResizeSwapchain(uint32_t width, uint32_t height) {
        if (!m_local.swapchain.Recreate(width, height)) {
            return false;
        }

        uint32_t newImageCount = m_local.swapchain.GetImages().size();


        if (m_renderFinished.size() != newImageCount) {
            for (auto& sem : m_renderFinished) {
                sem.Destroy();
            }
            m_renderFinished.clear();

            for(uint32_t i = 0; i < newImageCount; i++) {
                auto& sem = m_renderFinished.emplace_back();
                CheckCritical(sem.Init(&m_local.device), "Failed to recreate render semaphore");
            }
        }
        return true;
    }


    template<ValidAPI API>
    bool RenderHardwareInterface<API>::SwapchainPresent(uint32_t imageIdx, bool *isOld) {
        // uint64_t nextId = ++m_globalPresentId;
        // m_imagePresentIds[imageIdx] = nextId;
        return m_local.swapchain.Present(m_renderFinished[imageIdx], imageIdx, isOld, m_presentFences[imageIdx]);
    }

    template<ValidAPI API>
    void RenderHardwareInterface<API>::EndFrame() {
        ProcessDeferredCallbacks();

        m_currentFrame = (++m_currentFrame) % Conf::SHIFT_MAX_FRAMES_IN_FLIGHT;
        ++m_currentFrameGlobalIndex;
    }

    template<ValidAPI API>
    RHIContext<API>* RenderHardwareInterface<API>::AcquireSecondaryGraphicsContext() {
        std::lock_guard<std::mutex> lock(m_secondaryPoolMutex);

        for (auto& ctx : m_secondaryGraphicsContexts[m_currentFrame]) {
            if (!m_secondaryGraphicsContextData[&ctx].inUse) {
                m_secondaryGraphicsContextData[&ctx].inUse = true;
                ctx.ResetCmds();
                return &ctx;
            }
        }

        Log(Warning, "Failed to acquire secondary graphics context!");
        return nullptr;
    }

    template<ValidAPI API>
    void RenderHardwareInterface<API>::ExecuteSecondaryGraphicsContexts(std::span<RHIContext<API> *> secondaries,
        typename RHIContext<API>::SubmitTimelinePayload releasePayload)
    {
        std::lock_guard<std::mutex> lock(m_secondaryPoolMutex);

        std::vector<CommandBuffer*> secondaryBuffers;
        for (RHIContext<API>* sec: secondaries) {
            secondaryBuffers.push_back(&sec->GetCommandBuffer());
            m_secondaryGraphicsContextData[sec].submittedToPrimary = true;
        }

        m_graphicsContexts[m_currentFrame].ExecuteSecondaryGraphicsContexts(secondaryBuffers);

        std::vector<RHIContext<API>*> secondariesCopy(secondaries.begin(), secondaries.end());

        m_deferredExecutor.DeferExecute(releasePayload.semaphore, releasePayload.value, [this, secondariesCopy = std::move(secondariesCopy)]() mutable {
            std::lock_guard<std::mutex> lock(m_secondaryPoolMutex);
            for (RHIContext<API>* sec: secondariesCopy) {
                m_secondaryGraphicsContextData[sec].inUse = false;
                m_secondaryGraphicsContextData[sec].submittedToPrimary = false;
            }
        });
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

    template<ValidAPI API>
    void RenderHardwareInterface<API>::DeferExecuteEndOfSession(RHIDeferredExecutor::Callback fn) {
        m_deferredExecutor.DeferExecuteEndOfSession(fn);
    }

    template<ValidAPI API>
    void RenderHardwareInterface<API>::DeferExecute(TimelineSemaphore *sem, uint64_t waitVal,
        RHIDeferredExecutor::Callback fn)
    {
        m_deferredExecutor.DeferExecute(sem, waitVal, fn);
    }

    template<ValidAPI API>
    void RenderHardwareInterface<API>::DeferExecuteToFrame(uint64_t frameIndexGlobal,
        RHIDeferredExecutor::Callback fn)
    {
        m_deferredExecutor.DeferExecuteToFrame(frameIndexGlobal, fn);
    }

    template<ValidAPI API>
    void RenderHardwareInterface<API>::ProcessDeferredCallbacks() {
        m_deferredExecutor.ProcessDeferredCallbacks(m_currentFrameGlobalIndex);
    }

    template<ValidAPI API>
    void RenderHardwareInterface<API>::ShaderHotReload() {
        std::unordered_set<Pipeline*> toRebuild = m_shaderManager.HotReload();

        auto payload = GetGraphicsWaitPayload();

        for (Pipeline* pipeline: toRebuild) {
#ifdef SHIFT_VULKAN_BACKEND
            auto pipelineHandle = pipeline->VK_Get();
#endif
            DeferExecute(payload.semaphore, payload.value, [this, pipelineHandle]() {
                    m_local.device.DestroyPipeline(pipelineHandle);
                }
            );

            pipeline->Rebuild(false);
        }
    }


    //! ------------------------------- Vulkan Specific -------------------------------

    template<>
    inline void RenderHardwareInterface<RHI::Vulkan>::WaitForGPU() {
        vkDeviceWaitIdle(m_local.device.Get());
    }

    template<>
    inline Pipeline* RenderHardwareInterface<RHI::Vulkan>::CreatePipeline(const PipelineDescriptor &desc,
        const std::vector<ShaderStageDesc> &shaders)
    {

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

        Pipeline* p = new Pipeline{};
        p->Init(&m_local.device, desc, shaders, setLayouts);

        for (auto& stage: shaders) {
            m_shaderManager.RegisterPipeline(stage.handle, p);
        }

        m_pipelines.insert(p);

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
    inline void RenderHardwareInterface<RHI::Vulkan>::WaitForImagePresent(uint32_t imageIndex) {
        m_presentFences[imageIndex].Wait();
        m_presentFences[imageIndex].Reset();
    }

#ifdef SHIFT_VULKAN_BACKEND
    using SRHI = RenderHardwareInterface<RHI::Vulkan>;
    using SRHIContext = RHIContext<RHI::Vulkan>;
    using ShiftSelectedAPI = RHI::Vulkan;
#endif
} // Shift

#endif //SHIFT_SRHI_HPP