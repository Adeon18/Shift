//
// Created by otrush on 10/28/2025.
//

#ifndef SHIFT_SRHI_HPP
#define SHIFT_SRHI_HPP

#include <atomic>
#include <numeric>

#include "Common/NativeWindow.hpp"
#include "RHIContext.hpp"
#include "RHIDeferredExecutor.hpp"

namespace Shift {

    template<ValidAPI API>
    class RenderHardwareInterface {
    public:
        //! The API for Backend useage by different engine systems (stores handle creation functions, rest are hidden)
        class HandleCreator {
        public:
            HandleCreator() = default;
            explicit HandleCreator(RenderHardwareInterface* backend): m_backend{backend} {}
            HandleCreator(const HandleCreator&) = default;
            HandleCreator& operator=(const HandleCreator&) = default;
            [[nodiscard]] Buffer* CreateBuffer(const BufferDescriptor& desc);
            [[nodiscard]] Texture* CreateTexture(const TextureDescriptor& desc);
            [[nodiscard]] Pipeline* CreatePipeline(const PipelineDescriptor& desc, const std::vector<ShaderStageDesc>& shaders);
            [[nodiscard]] ResourceSet* CreateResourceSet(const PipelineLayoutDescriptor& desc);
            [[nodiscard]] Sampler* CreateSampler(const SamplerDescriptor& desc);
            //! Creates a backend shader module from already-compiled bytecode. Makes it harder to miscall and cause abstraction leak this way
            //! Ideally a ShaderManager of sorts should call this function, handle shader hot reload/etc.
            [[nodiscard]] Shader* CreateShader(std::span<uint8_t> bytecode, const ShaderDescriptor& desc);
        private:
            RenderHardwareInterface* m_backend = nullptr;
        };
        //! requiredFeatures defaults to the backend's standard set; tests override it
        bool Init(const NativeWindow& window, uint32_t width, uint32_t height, const RHIAppInfo& appInfo, const RHIBindlessCapacity& bindless, const RHIRequiredFeatures& requiredFeatures = API::requiredFeatures);

        //! Wait for GPU to complete work before deleting stuff
        void WaitForGPU();
        void Destroy();

        //! Handle creator is an API for the RHI for mosty managers
        HandleCreator* CreateInterface() { return &m_handleCreator; }

        [[nodiscard]] const RHICapabilities& GetCapabilities() const;

        //! The buffer layout a 2D buffer<->texture copy needs on this device, fill or read the buffer with it
        //! It is this complex as it has to support the stupid d3d12 256-byte row rule
        [[nodiscard]] TextureCopyFootprint GetTextureCopyFootprint(ETextureFormat format, uint32_t width, uint32_t height) const {
            const uint32_t texelSize = BytesPerPixel(format);
            assert(texelSize != 0 && "the format has no known texel size");
            //! Whole texels on every backend, on top of the device's alignment
            const uint32_t alignment = std::lcm(std::max(GetCapabilities().limits.textureCopyRowPitchAlignment, 1u), texelSize);
            const uint32_t rowSize = width * texelSize;
            const uint32_t rowPitch = (rowSize + alignment - 1) / alignment * alignment;
            return {rowSize, rowPitch, height, uint64_t(rowPitch) * height};
        }

        [[nodiscard]] Swapchain& GetSwapchain() { return *m_local.swapchain; }
        [[nodiscard]] EAcquireResult AcquireFrameImage(FrameImage& out);
        //! Pass the image to the SubmitCmds that last writes it before presenting
        [[nodiscard]] bool Present(const FrameImage& image, bool* needsResize);
        [[nodiscard]] bool ResizeSwapchain(uint32_t width, uint32_t height);

        void EndFrame();

        uint32_t GetCurrentFrame() { return m_currentFrame; }
        uint64_t GetCurrentGlobalIndex() { return m_currentFrameGlobalIndex; }

        RHIContext<API>& GetGraphicsContext() { return m_graphicsContexts[m_currentFrame]; }
        RHIContext<API>& GetComputeContext() { return m_computeContext; }
        RHIContext<API>& GetTransferContext() { return m_transferContext; }
        //! Separate graphics context not bound by frame context requirements
        RHIContext<API>& GetCaptureContext() { return m_captureContext; }

        //! Start-of-frame work for the slot about to be recorded
        void BeginFrame();

        RHIContext<API>* AcquireSecondaryGraphicsContext();
        void ExecuteSecondaryGraphicsContexts(std::span<RHIContext<API>*> secondaries, RHIContext<API>::SubmitTimelinePayload releasePayload);

        //! Waits until this frame slot's previous submit is done
        void WaitForGraphicsContext();

        ///! ------------------- Queue ownership handoffs (F-QFOT) ------------------- !///

        //! Drop the handoff for a certain texture if we are freeeing it
        void CancelPendingAcquires(Texture* texture) { m_local.CancelPendingAcquires(texture); }

        //! Block until every transfer submitted so far has completed on the GPU
        void WaitForTransferIdle() { m_timelineTransfer->Wait(m_timelineTransferValue.load(std::memory_order_acquire)); }

        RHIContext<API>::SubmitTimelinePayload GetTransferWaitPayload();
        RHIContext<API>::SubmitTimelinePayload ReserveTransferSignalPayload();

        RHIContext<API>::SubmitTimelinePayload GetGraphicsWaitPayload();
        RHIContext<API>::SubmitTimelinePayload ReserveGraphicsSignalPayload();

        void DeferExecuteEndOfSession(RHIDeferredExecutor::Callback fn);
        void DeferExecute(TimelineSemaphore* sem, uint64_t waitVal, RHIDeferredExecutor::Callback fn);

        void DeferExecuteToFrame(uint64_t frameIndexGlobal, RHIDeferredExecutor::Callback fn);

        void ProcessDeferredCallbacks();

        //! Resolved GPU timing ranges of the most recently completed frame
        [[nodiscard]] const std::vector<GPUTimeRange>& GetLastFrameGPUTimeRanges() const { return m_lastFrameGPUTimeRanges; }

        //! Force-run every pending deferred callback regardless of its gate. Intended for
        //! shutdown: call once the GPU is idle and BEFORE tearing down owned resources, so a
        //! queued callback never fires past its target.
        void FlushAllDeferredCallbacks();

        [[nodiscard]] const RHILocal<API>& GetLocal() const { return m_local; }

    private:
        RHILocal<API> m_local;
        HandleCreator m_handleCreator;

        std::array<RHIContext<API>, RHI::MAX_FRAMES_IN_FLIGHT> m_graphicsContexts;
        struct SecondaryContextData {
            bool inUse = false;
            bool submittedToPrimary = false;
            uint32_t id = 0;
        };

        std::array<std::vector<RHIContext<API>>, RHI::MAX_FRAMES_IN_FLIGHT> m_secondaryGraphicsContexts;
        std::unordered_map<RHIContext<API>*, SecondaryContextData> m_secondaryGraphicsContextData;
        std::mutex m_secondaryPoolMutex;

        // Single transfer/compute contexts (can be expanded to a pool)
        RHIContext<API> m_transferContext;
        RHIContext<API> m_computeContext;
        //! Out-of-frame graphics work (capture). Never one of the m_graphicsContexts slots
        RHIContext<API> m_captureContext;

        RHIDeferredExecutor m_deferredExecutor;

        uint32_t m_currentFrame = 0;
        uint64_t m_currentFrameGlobalIndex = 0;

        //! Timeline semaphores per queue type
        RHI::UniquePtr<TimelineSemaphore> m_timelineGraphics;
        RHI::UniquePtr<TimelineSemaphore> m_timelineTransfer;
        RHI::UniquePtr<TimelineSemaphore> m_timelineCompute;

        std::atomic<uint64_t> m_timelineGraphicsValue{0};
        std::atomic<uint64_t> m_timelineTransferValue{0};
        std::atomic<uint64_t> m_timelineComputeValue{0};

        //! Graphics-timeline value each frame slot's last submit will reach, snapshotted in
        //! EndFrame. Each next use of the FIF slot waits exactly on its own value
        std::array<uint64_t, RHI::MAX_FRAMES_IN_FLIGHT> m_frameSlotGraphicsValue{};

        //! Last completed frame's GPU timing ranges, refreshed in BeginFrame
        std::vector<GPUTimeRange> m_lastFrameGPUTimeRanges;
    };

    template<ValidAPI API>
    bool RenderHardwareInterface<API>::Init(const NativeWindow& window, uint32_t width, uint32_t height, const RHIAppInfo& appInfo, const RHIBindlessCapacity& bindless, const RHIRequiredFeatures& requiredFeatures)
    {

        m_handleCreator = HandleCreator{this};

        //! Native backend bring-up lives behind a per-API hook
        SRHI_CHECK_CRITICAL(m_local.InitBackend(window, width, height, appInfo, bindless, requiredFeatures), "Failed to initialize the main backend handles!");

        for (uint32_t i = 0; i < RHI::MAX_FRAMES_IN_FLIGHT; ++i) {
            SRHI_CHECK_CRITICAL(m_graphicsContexts[i].Init(&m_local, EContextType::Graphics, false), "Failed to create Graphics Context in flight!");
            m_graphicsContexts[i].GetCommandBuffer().SetDebugName(("GraphicsPrimaryCB[" + std::to_string(i) + "]").c_str());
        }

        for (uint32_t i = 0; i < RHI::MAX_FRAMES_IN_FLIGHT; ++i) {
            m_secondaryGraphicsContexts[i].resize(RHI::MAX_SECONDARY_CONTEXTS);
            for (uint32_t j = 0; j < m_secondaryGraphicsContexts[i].size(); ++j) {
                m_secondaryGraphicsContexts[i][j].Init(&m_local, EContextType::Graphics, true);
                m_secondaryGraphicsContexts[i][j].GetCommandBuffer().SetDebugName(("SecondaryGraphicsCB[" + std::to_string(i) + "][" + std::to_string(j) + "]").c_str());
                m_secondaryGraphicsContextData[&m_secondaryGraphicsContexts[i][j]].inUse = false;
                m_secondaryGraphicsContextData[&m_secondaryGraphicsContexts[i][j]].submittedToPrimary = false;
                m_secondaryGraphicsContextData[&m_secondaryGraphicsContexts[i][j]].id = RHI::MAX_SECONDARY_CONTEXTS * i + j;
            }
        }

        SRHI_CHECK_CRITICAL(m_computeContext.Init(&m_local, EContextType::Compute, false), "Failed to create Compute Context!");
        m_computeContext.GetCommandBuffer().SetDebugName("ComputeCB");

        SRHI_CHECK_CRITICAL(m_transferContext.Init(&m_local, EContextType::Transfer, false), "Failed to create Transfer Context!");
        m_transferContext.GetCommandBuffer().SetDebugName("TransferCB");

        SRHI_CHECK_CRITICAL(m_captureContext.Init(&m_local, EContextType::Graphics, false), "Failed to create Capture Context!");
        m_captureContext.GetCommandBuffer().SetDebugName("CaptureCB");

        m_timelineCompute = RHI::CreateUnique<TimelineSemaphore>(m_local.device.get(), 0);
        m_timelineGraphics = RHI::CreateUnique<TimelineSemaphore>(m_local.device.get(), 0);
        m_timelineTransfer = RHI::CreateUnique<TimelineSemaphore>(m_local.device.get(), 0);

        //! Named sync objects make sync-validation output name the offender (F-DEBUGMARKERS)
        m_timelineCompute->SetDebugName("TimelineCompute");
        m_timelineGraphics->SetDebugName("TimelineGraphics");
        m_timelineTransfer->SetDebugName("TimelineTransfer");

        return true;
    }

    template<ValidAPI API>
    void RenderHardwareInterface<API>::Destroy() {

        m_deferredExecutor.FlushAllDeferredCallbacks();

        m_local.DestroySwapchain();

        m_timelineTransfer.reset();
        m_timelineGraphics.reset();
        m_timelineCompute.reset();

        for (uint32_t i = 0; i < RHI::MAX_FRAMES_IN_FLIGHT; ++i) {
            m_graphicsContexts[i].Destroy();
        }

        for (uint32_t i = 0; i < RHI::MAX_FRAMES_IN_FLIGHT; ++i) {
            for (auto& ctx : m_secondaryGraphicsContexts[i]) {
                ctx.Destroy();
            }
        }

        m_transferContext.Destroy();
        m_computeContext.Destroy();
        m_captureContext.Destroy();

        m_local.DestroyBackend();

        //! Post-mortem summary: by this point every teardown leak report has been counted by the
        //! validation sink, so a non-zero error count here means the run was NOT clean (F-VALSINK)
        const ValidationStats validationStats = GetValidationStats();
        SRHI_LOG(Info, "Validation summary: {} error(s), {} warning(s)", validationStats.errorCount, validationStats.warningCount);
    }

    template<ValidAPI API>
    Buffer* RenderHardwareInterface<API>::HandleCreator::CreateBuffer(const BufferDescriptor &desc) {
        return new Buffer(m_backend->m_local.device.get(), desc);
    }

    template<ValidAPI API>
    Texture* RenderHardwareInterface<API>::HandleCreator::CreateTexture(const TextureDescriptor &desc) {
        return new Texture(m_backend->m_local.device.get(), desc);
    }

    template<ValidAPI API>
    Sampler* RenderHardwareInterface<API>::HandleCreator::CreateSampler(const SamplerDescriptor &desc) {
        return new Sampler(m_backend->m_local.device.get(), desc);
    }

    template<ValidAPI API>
    Shader* RenderHardwareInterface<API>::HandleCreator::CreateShader(std::span<uint8_t> bytecode, const ShaderDescriptor &desc) {
        return new Shader{m_backend->m_local.device.get(), bytecode, desc};
    }

    template<ValidAPI API>
    bool RenderHardwareInterface<API>::ResizeSwapchain(uint32_t width, uint32_t height) {
        return m_local.RecreateSwapchain(width, height);
    }

    template<ValidAPI API>
    void RenderHardwareInterface<API>::BeginFrame() {
        WaitForGraphicsContext();

        //! The slot's previous frame is now complete, so its timestamp results are readable
        //! We read them before re-recording the cb (which clears the results)
        m_lastFrameGPUTimeRanges = m_graphicsContexts[m_currentFrame].GetCommandBuffer().CollectTimeRanges();

        //! Process deferred callbacks runs only when we do have free GPU resources
        ProcessDeferredCallbacks();
    }

    template<ValidAPI API>
    void RenderHardwareInterface<API>::EndFrame() {
        //! Remember the graphics-timeline value this frame's final submit will reach
        m_frameSlotGraphicsValue[m_currentFrame] = m_timelineGraphicsValue.load(std::memory_order_acquire);

        m_currentFrame = (m_currentFrame + 1) % RHI::MAX_FRAMES_IN_FLIGHT;
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

        SRHI_LOG(Warning, "Failed to acquire secondary graphics context!");
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
        //! Bugfix, now we wait on the previous frame in flight of the same slot's timeline value instead of the latest value
        //! This actually enables frames in flight
        m_timelineGraphics->Wait(m_frameSlotGraphicsValue[m_currentFrame]);
    }

    template<ValidAPI API>
    RHIContext<API>::SubmitTimelinePayload RenderHardwareInterface<API>::GetTransferWaitPayload() {
        return { m_timelineTransfer.get(), m_timelineTransferValue.load(std::memory_order_acquire) };
    }

    template<ValidAPI API>
    RHIContext<API>::SubmitTimelinePayload RenderHardwareInterface<API>::ReserveTransferSignalPayload() {
        uint64_t newVal = m_timelineTransferValue.fetch_add(1, std::memory_order_acq_rel) + 1;
        return { m_timelineTransfer.get(), newVal };
    }

    template<ValidAPI API>
    RHIContext<API>::SubmitTimelinePayload RenderHardwareInterface<API>::GetGraphicsWaitPayload() {
        return { m_timelineGraphics.get(), m_timelineGraphicsValue.load(std::memory_order_acquire) };
    }

    template<ValidAPI API>
    RHIContext<API>::SubmitTimelinePayload RenderHardwareInterface<API>::ReserveGraphicsSignalPayload() {
        uint64_t newVal = m_timelineGraphicsValue.fetch_add(1, std::memory_order_acq_rel) + 1;
        return { m_timelineGraphics.get(), newVal };
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
    void RenderHardwareInterface<API>::FlushAllDeferredCallbacks() {
        m_deferredExecutor.FlushAllDeferredCallbacks();
    }


#ifdef SHIFT_VULKAN_BACKEND
    using ShiftSelectedAPI = RHI::Vulkan;
#endif
#ifdef SHIFT_DX12_BACKEND
    using ShiftSelectedAPI = RHI::DX12;
#endif
    using RenderBackend = RenderHardwareInterface<ShiftSelectedAPI>;
    using RenderContext = RHIContext<ShiftSelectedAPI>;
    using RenderContextEncoder = RHIEncoder<ShiftSelectedAPI>;
    using RenderBackendInterface = RenderHardwareInterface<ShiftSelectedAPI>::HandleCreator;
} // Shift

//! Backend-specific member specializations live in their own header).
//! Included last: the full RenderHardwareInterface template is defined by
//! now, and we just add specific template functions for specific API
#ifdef SHIFT_VULKAN_BACKEND
#include "RHI_VK.hpp"
#endif
#ifdef SHIFT_DX12_BACKEND
#include "DX12/RHI_DX12.hpp"
#endif

#endif //SHIFT_SRHI_HPP