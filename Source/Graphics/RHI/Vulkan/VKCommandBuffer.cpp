#include "VKCommandBuffer.hpp"

#include "Utility/Vulkan/VKDebugUtils.hpp"
#include "Utility/Vulkan/VKUtilInfo.hpp"
#include "Utility/Vulkan/VKUtilRHI.hpp"
#include <algorithm>
#include <iostream>
#include <array>
#include <winsock2.h>

#include "VKBuffer.hpp"
#include "VKPipeline.hpp"
#include "VKResourceSet.hpp"
#include "VKSemaphore.hpp"
#include "VKTexture.hpp"

#include "Config/EngineConfig.hpp"

namespace Shift::VK {
    CommandPool::CommandPool(const Device *device, EPoolQueueType type): m_device {device}, m_type {type} {
        m_commandPool = m_device->CreateCommandPool(
            Util::CreateCommandPoolInfo(m_device->GetQueueFamilyIndex(m_type)));
    }

    void CommandPool::Reset() const {
        vkResetCommandPool(m_device->Get(), m_commandPool, 0);
    }

    CommandPool::~CommandPool() {
        m_device->DestroyCommandPool(m_commandPool);
    }

    CommandBuffer::CommandBuffer(const Device* device, const Instance* ins, const CommandPool& commandPool, bool isSecondary):
        m_device(device), m_ins(ins), m_poolType(commandPool.GetType()), m_isSecondary(isSecondary) {

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool.GetPool();
        // Primary can be submitted to the queue, secondary can be called from primary buffers and inversely
        allocInfo.level = (m_isSecondary) ? VK_COMMAND_BUFFER_LEVEL_SECONDARY: VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        if ( VkCheck(vkAllocateCommandBuffers(m_device->Get(), &allocInfo, &m_buffer)) ) {
            Log(Error, "Failed to create VkCommandBuffer!");
            m_buffer = VK_NULL_HANDLE;
            return;
        }

        //! CONSPECT: GPU timing only makes sense on the primary graphics buffers that record a frame.
        //! Secondaries are draw-only as their work is inside the primary's ranges, transfer/compute
        //! queues have no spec-guaranteed timestamp support. But compute is woth adding in the future
        if (m_poolType == EPoolQueueType::Graphics && !m_isSecondary) {
            m_profiler.Init(m_device, Conf::MAX_GPU_TIME_RANGES_PER_FRAME);
        }
    }

    CommandBuffer::~CommandBuffer() {
        //! THis will fire only if the profiler is initted
        m_profiler.Destroy();
    }

    void CommandBuffer::Reset() {
        m_debugGroupTimed.clear();
        //! Handoffs go to 1 recording
        m_recordedHandoffs.clear();
        vkResetCommandBuffer(m_buffer, 0);
    }

    void CommandBuffer::SetDebugName(const char* name) const {
        Util::SetDebugName(m_device->Get(), VK_OBJECT_TYPE_COMMAND_BUFFER, reinterpret_cast<uint64_t>(m_buffer), name);
    }

    void CommandBuffer::PushDebugGroup(const char* label, const DebugLabelColor& color, bool timed) {
        Util::CmdBeginDebugLabel(m_buffer, label, color);
        m_debugGroupTimed.push_back(timed ? 1 : 0);
        if (timed) {
            m_profiler.PushRange(m_buffer, label, color);
        }
    }

    void CommandBuffer::PopDebugGroup() {
        const bool wasTimed = !m_debugGroupTimed.empty() && m_debugGroupTimed.back() != 0;
        if (!m_debugGroupTimed.empty()) {
            m_debugGroupTimed.pop_back();
        }
        if (wasTimed) {
            m_profiler.PopRange(m_buffer);
        }
        Util::CmdEndDebugLabel(m_buffer);
    }

    void CommandBuffer::InsertDebugLabel(const char* label, const DebugLabelColor& color) const {
        Util::CmdInsertDebugLabel(m_buffer, label, color);
    }

    void CommandBuffer::PushTimeRange(const char* name) {
        m_profiler.PushRange(m_buffer, name);
    }

    void CommandBuffer::PopTimeRange() {
        m_profiler.PopRange(m_buffer);
    }

    std::vector<GPUTimeRange> CommandBuffer::CollectTimeRanges() {
        return m_profiler.Collect();
    }


    // bool CommandBuffer::BeginCommandBufferSingleTime() const {
    //     return BeginCommandBuffer(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    // }
    //
    bool CommandBuffer::Begin() {
        assert(!m_isSecondary);

        m_debugGroupTimed.clear();
        //! Handoffs go to 1 recording
        m_recordedHandoffs.clear();

        auto info = Util::CreateBeginCommandBufferInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr);
        if ( VkCheckV(vkBeginCommandBuffer(m_buffer, &info), res) ) {
            Log(Error, "Failed to begin primary command buffer! Code: %d", static_cast<int>(res));
            return false;
        }

        //! Reset the timestamp pool + drop the previous recording's range records, no-op if uninitted
        m_profiler.ResetForRecording(m_buffer);

        return true;
    }

    bool CommandBuffer::BeginSecondary(const SecondaryBufferBeginPayload &payload) {
        assert(m_isSecondary);

        m_debugGroupTimed.clear();
        //! Handoffs go to 1 recording
        m_recordedHandoffs.clear();

        std::vector<VkFormat> colorFormats;

        colorFormats.resize(payload.colorFormats.size());
        for (uint32_t i = 0; i < payload.colorFormats.size(); ++i) {
            colorFormats[i] = Util::ShiftToVKTextureFormat(payload.colorFormats[i]);
        }

        auto inheritanceRenderingInfo = Util::CreateInheritanceRenderingInfo(
            colorFormats,
            Util::ShiftToVKTextureFormat(payload.depthFormat.value_or(ETextureFormat::UNDEFINED)),
             Util::ShiftToVKTextureFormat(payload.stencilFormat.value_or(ETextureFormat::UNDEFINED)),
            VK_SAMPLE_COUNT_1_BIT
        );

        VkCommandBufferInheritanceInfo inheritanceInfo{};
        inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        inheritanceInfo.pNext = &inheritanceRenderingInfo;


        auto info = Util::CreateBeginCommandBufferInfo(VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT | VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, &inheritanceInfo);
        if ( VkCheckV(vkBeginCommandBuffer(m_buffer, &info), res) ) {
            Log(Error, "Failed to begin secondary command buffer! Code: %d", static_cast<int>(res));
            return false;
        }

        return true;
    }

    bool CommandBuffer::End() const {
        if ( VkCheckV(vkEndCommandBuffer(m_buffer), res)) {
            Log(Error, "Failed to end command buffer! Code: %d", static_cast<int>(res));
            return false;
        }
        return true;
    }

    void CommandBuffer::CopyBufferToBuffer(const BufferOpDescriptor& srcBuf, const BufferOpDescriptor& dstBuf, uint32_t size) const {
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = srcBuf.offset; // Optional
        copyRegion.dstOffset = dstBuf.offset; // Optional
        copyRegion.size = size;

        vkCmdCopyBuffer(m_buffer, srcBuf.buffer->VK_Get(), dstBuf.buffer->VK_Get(), 1, &copyRegion);
    }

    void CommandBuffer::FillBuffer(const BufferOpDescriptor& dstBuf, uint64_t size, uint32_t value) const {
        assert(dstBuf.offset % 4 == 0 && size % 4 == 0 && "FillBuffer writes whole 32-bit words");
        vkCmdFillBuffer(m_buffer, dstBuf.buffer->VK_Get(), dstBuf.offset, size, value);
    }

    void CommandBuffer::CopyBufferToTexture(const BufferOpDescriptor& srcBuf, const TextureCopyDescriptor& dstTex) const {
        const TextureSubresourceRange& range = dstTex.subresourceRange;

        //! Ah this is some bullshit, if you are reading this, don't -- I will fix later
        assert(range.levelCount == 1 && "CopyBufferToTexture uploads a single mip level per call");

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = Util::ShiftToVKTextureAspect(range.aspect);
        region.imageSubresource.mipLevel = range.baseMipLevel;
        region.imageSubresource.baseArrayLayer = range.baseArrayLayer;
        region.imageSubresource.layerCount = range.layerCount;

        region.imageOffset = {dstTex.offset.x, dstTex.offset.y, dstTex.offset.z };
        region.imageExtent = {
            dstTex.size.x, dstTex.size.y, dstTex.size.z
    };

        vkCmdCopyBufferToImage(
            m_buffer,
            srcBuf.buffer->VK_Get(),
            dstTex.texture->VK_GetImage(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );
    }

    void CommandBuffer::CopyTextureToBuffer(const TextureCopyDescriptor& srcTex, const BufferOpDescriptor& dstBuf) const {
        const TextureSubresourceRange& range = srcTex.subresourceRange;
        assert(range.levelCount == 1 && "CopyTextureToBuffer reads a single mip level per call");
        VkBufferImageCopy region{};
        region.bufferOffset = dstBuf.offset;
        //! 0-s essentially make the buffer pick the size to fit image tightly
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = Util::ShiftToVKTextureAspect(range.aspect);
        region.imageSubresource.mipLevel = range.baseMipLevel;
        region.imageSubresource.baseArrayLayer = range.baseArrayLayer;
        region.imageSubresource.layerCount = range.layerCount;
        region.imageOffset = {srcTex.offset.x, srcTex.offset.y, srcTex.offset.z};
        region.imageExtent = {srcTex.size.x, srcTex.size.y, srcTex.size.z};

        vkCmdCopyImageToBuffer(
            m_buffer,
            srcTex.texture->VK_GetImage(),
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            dstBuf.buffer->VK_Get(),
            1,
            &region
        );
    }

    void CommandBuffer::VK_SetPipelineBarrier(
        std::span<VkImageMemoryBarrier2> imgSpan,
        std::span<VkMemoryBarrier2> memSpan,
        std::span<VkBufferMemoryBarrier2> bufMemSpan,
        VkDependencyFlags flags) const
    {
        VkDependencyInfo depInfo{};
        depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        depInfo.dependencyFlags = flags;
        depInfo.memoryBarrierCount = static_cast<uint32_t>(memSpan.size());
        depInfo.pMemoryBarriers = memSpan.data();
        depInfo.bufferMemoryBarrierCount = static_cast<uint32_t>(bufMemSpan.size());
        depInfo.pBufferMemoryBarriers = bufMemSpan.data();
        depInfo.imageMemoryBarrierCount = static_cast<uint32_t>(imgSpan.size());
        depInfo.pImageMemoryBarriers = imgSpan.data();

        vkCmdPipelineBarrier2(m_buffer, &depInfo);
    }

    void CommandBuffer::VK_SetPipelineBarrierImage(
        VkImageMemoryBarrier2 imgBarrier,
        VkDependencyFlags flags) const
    {
        VK_SetPipelineBarrier({&imgBarrier, 1}, {}, {}, flags);
    }


    void CommandBuffer::ExecuteSecondaryBuffers(std::span<CommandBuffer*> secondaryBuffs) const {
        assert(!m_isSecondary);
        assert(!secondaryBuffs.empty());

        std::vector<VkCommandBuffer> vkSecondaries;
        vkSecondaries.reserve(secondaryBuffs.size());
        for (auto* sec : secondaryBuffs) {
            assert(sec->m_isSecondary);
            vkSecondaries.push_back(sec->m_buffer);
        }

        vkCmdExecuteCommands(m_buffer, static_cast<uint32_t>(vkSecondaries.size()), vkSecondaries.data());
    }

    bool CommandBuffer::Submit(std::span<TimelineSemaphore*> waitSems, std::span<uint64_t> waitVals,
        std::span<TimelineSemaphore*> signalSems, std::span<uint64_t> sigVals)
    {
        return Submit(waitSems, waitVals, signalSems, sigVals, {}, {});
    }

    bool CommandBuffer::Submit(std::span<TimelineSemaphore*> waitTimeSems, std::span<uint64_t> waitVals,
        std::span<TimelineSemaphore*> signalTimeSems, std::span<uint64_t> sigVals,
        std::span<BinarySemaphore*> waitBinSems, std::span<BinarySemaphore*> signalBinSems)
    {
        assert(waitTimeSems.size() == waitVals.size());
        assert(signalTimeSems.size() == sigVals.size());

        std::vector<uint64_t> combinedWaitVals;
        std::vector<uint64_t> combinedSignalVals;

        combinedWaitVals.reserve(waitBinSems.size() + waitTimeSems.size());
        combinedWaitVals.insert(combinedWaitVals.end(), waitBinSems.size(), 0ull); // binaries
        combinedWaitVals.insert(combinedWaitVals.end(), waitVals.begin(), waitVals.end()); // timeline

        combinedSignalVals.reserve(signalBinSems.size() + signalTimeSems.size());
        combinedSignalVals.insert(combinedSignalVals.end(), signalBinSems.size(), 0ull); // binaries
        combinedSignalVals.insert(combinedSignalVals.end(), sigVals.begin(), sigVals.end()); // timeline

        VkTimelineSemaphoreSubmitInfo timelineInfo =
            Util::CreateTimelineSemaphoreSubmitInfo(combinedWaitVals, combinedSignalVals);

        std::vector<VkSemaphore> waitSemsVk;
        std::vector<VkSemaphore> sigSemsVk;

        std::vector<VkPipelineStageFlags> waitStages;

        for (auto& sem: waitBinSems) {
            waitSemsVk.push_back(sem->Get());
            //! Shared chain point with RHI's swapchain-acquire tracked-stage seeding
            waitStages.push_back(static_cast<VkPipelineStageFlags>(Util::ShiftToVKPipelineStageFlags2(BinarySemaphore::WAIT_STAGE)));
        }

        for (auto& sem: waitTimeSems) {
            waitSemsVk.push_back(sem->Get());
            waitStages.push_back(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
        }

        for (auto& sem: signalBinSems) {
            sigSemsVk.push_back(sem->Get());
        }

        for (auto& sem: signalTimeSems) {
            sigSemsVk.push_back(sem->Get());
        }

        VkSubmitInfo info = Util::CreateSubmitInfo(
                waitSemsVk,
                sigSemsVk,
                std::span{&m_buffer, 1},
                waitStages.data(),
                timelineInfo
        );

        VkQueue submitQueue;
        switch (m_poolType) {
            case EPoolQueueType::Graphics:
                submitQueue = m_device->GetGraphicsQueue();
                break;
            case EPoolQueueType::Transfer:
                submitQueue = m_device->GetTransferQueue();
                break;
            case EPoolQueueType::Compute:
                submitQueue = m_device->GetComputeQueue();
                break;
            default:
                Log(Error, "Invalid pool type!");
                return false;
        }

        if (int res = vkQueueSubmit(submitQueue, 1, &info, nullptr); res != VK_SUCCESS) {
            Log(Error, "Failed to submit to queue! Code: {}", res);
            return false;
        }

        return true;
    }


    void CommandBuffer::BindVertexBuffer(const BufferOpDescriptor& buffer, uint32_t bindIdx) const {
        std::vector<VkDeviceSize> offsets{static_cast<VkDeviceSize>(buffer.offset)};
        std::vector<VkBuffer> buffers{buffer.buffer->VK_Get()};
        vkCmdBindVertexBuffers(
                m_buffer,
                bindIdx,
                1,
                buffers.data(),
                offsets.data());
    }

    void CommandBuffer::BindVertexBuffers(std::span<BufferOpDescriptor> buffers, uint32_t firstBind) const {
        std::vector<VkBuffer> buffs;
        std::vector<VkDeviceSize> offsets;

        buffs.reserve(buffers.size());
        offsets.reserve(buffers.size());

        for (auto&[b, o] : buffers) {
            buffs.push_back(b->VK_Get());
            offsets.push_back(o);
        }

        vkCmdBindVertexBuffers(
                m_buffer,
                firstBind,
                static_cast<uint32_t>(buffers.size()),
                buffs.data(),
                offsets.data());
    }

    void CommandBuffer::BindIndexBuffer(const BufferOpDescriptor& buffer, EIndexSize indexSize) const {
        vkCmdBindIndexBuffer(m_buffer, buffer.buffer->VK_Get(), buffer.offset, Util::ShiftToVKIndexType(indexSize));
    }

    void CommandBuffer::BindGraphicsPipeline(const Pipeline& pipeline) const {
        vkCmdBindPipeline(m_buffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.VK_Get());
    }

    void CommandBuffer::SetPushConstants(const Pipeline& pipeline, const void* data, uint32_t size, uint32_t offset) const {
        const std::optional<PushConstantRange>& range = pipeline.GetDescriptor().pushConstants;
        if (!range) {
            Log(Error, "SetPushConstants on pipeline '{}', which declares no push-constant range", pipeline.GetDescriptor().name);
            return;
        }
        //! Offset that is assed is relative to defined push constant range offset
        vkCmdPushConstants(m_buffer,
                           pipeline.VK_GetLayout(),
                           Util::ShiftToVKBindingVisibility(range->stageFlags),
                           range->offset + offset, size, data);
    }

    void CommandBuffer::BeginRenderPass(const RenderPassDescriptor& desc,
            std::span<Texture*> colorTextures, std::span<const EResourceLayout> colorLayouts,
            std::optional<Texture*> depthTexture, EResourceLayout depthLayout) const {
        assert(desc.colorAttachments.size() == colorTextures.size());
        assert(colorLayouts.size() == colorTextures.size());
        assert(desc.depthAttachment.has_value() == depthTexture.has_value());

        std::vector<VkRenderingAttachmentInfo> colorInfo;
        std::optional<VkRenderingAttachmentInfo> depthInfo;
        for (uint32_t i = 0; i < desc.colorAttachments.size(); i++) {
            const Texture* colTex = colorTextures[i];
            const RenderPassDescriptor::RenderPassAttachmentInfo& att = desc.colorAttachments[i];
            //! The attachment layout is whatever this recording has transitioned the texture to (not the submitted one!!!!!!!)
            colorInfo.push_back(Util::CreateRenderingAttachmentInfo(
                    colTex->VK_GetView(),
                    Util::ShiftToVKResourceLayout(colorLayouts[i]),
                    Util::ShiftToVKClearColor(att.clearValue),
                    Util::ShiftToVKAttachmentLoadOperation(att.loadOperation),
                    Util::ShiftToVKAttachmentStoreOperation(att.storeOperation)
                )
            );
        }

        if (desc.depthAttachment.has_value()) {
            const RenderPassDescriptor::RenderPassAttachmentInfo& att = desc.depthAttachment.value();
            depthInfo = Util::CreateRenderingAttachmentInfo(
                    (*depthTexture)->VK_GetView(),
                    Util::ShiftToVKResourceLayout(depthLayout),
                    Util::ShiftToVKClearDepthStencil(att.clearValue),
                    Util::ShiftToVKAttachmentLoadOperation(att.loadOperation),
                    Util::ShiftToVKAttachmentStoreOperation(att.storeOperation)
            );
        }

        VkRenderingInfo renderInfo{};
        renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        if (desc.enableSecondaryCommandBuffers) { renderInfo.flags |= VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT; };
        renderInfo.renderArea = {.offset = Util::ShiftToVKOffset2D(desc.offset), .extent = Util::ShiftToVKExtent2D(desc.extent)};
        renderInfo.layerCount = 1;
        renderInfo.colorAttachmentCount = static_cast<uint32_t>(colorInfo.size());
        renderInfo.pColorAttachments = colorInfo.data();
        if (depthInfo.has_value()) {
            renderInfo.pDepthAttachment = &depthInfo.value();
        }

        vkCmdBeginRendering(m_buffer, &renderInfo);
    }

    void CommandBuffer::EndRenderPass() const {
        vkCmdEndRendering(m_buffer);
    }

    VkImageSubresourceRange CommandBuffer::WholeImageRange(const Texture& texture) {
        //! Derive the barrier aspect from the texture's real aspect
        //! VK_REMAINING_* covers every mip and array layer
        VkImageSubresourceRange subresourceRange{};
        subresourceRange.aspectMask = Util::ShiftToVKTextureAspect(texture.GetAspect());
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
        return subresourceRange;
    }

    void CommandBuffer::TransitionMipRange(const Texture& texture, uint32_t baseLevel, uint32_t levelCount,
                                           EResourceLayout oldLayout, EResourceLayout newLayout,
                                           EPipelineStageFlags srcStage, EPipelineStageFlags dstStage) const {
        TextureBarrier(texture,
            MakeTextureDependency(oldLayout, srcStage, newLayout, dstStage),
            oldLayout, newLayout,
            TextureSubresourceRange{
                .aspect = texture.GetAspect(),
                .baseMipLevel = baseLevel,
                .levelCount = levelCount,
                .baseArrayLayer = 0,
                .layerCount = texture.GetLevels()
            });
    }

    void CommandBuffer::TextureBarrier(const Texture& texture, const MemoryDependency& dependency,
                                       EResourceLayout oldLayout, EResourceLayout newLayout,
                                       const TextureSubresourceRange& range) const {
        //! Barriers can only be done by a primary buffer, as secondaries run only after beginrenderpass
        //! where transitions are forbidden
        assert(!m_isSecondary);

        //! Same-queue barrier, no ownership changes hands, so both families are IGNORED
        VK_SetPipelineBarrierImage(
            Util::CreateImageMemoryBarrier2(
                texture.VK_GetImage(),
                Util::ShiftToVKResourceLayout(oldLayout), Util::ShiftToVKResourceLayout(newLayout),
                Util::ShiftToVKPipelineStageFlags2(dependency.srcStage), Util::ShiftToVKAccessFlags2(dependency.srcAccess),
                Util::ShiftToVKPipelineStageFlags2(dependency.dstStage), Util::ShiftToVKAccessFlags2(dependency.dstAccess),
                VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
                Util::ShiftToVKSubresourceRange(range)),
            0);
    }

    bool CommandBuffer::NeedsOwnershipTransfer(EPoolQueueType dstQueue) const {
        return m_device->GetQueueFamilyIndex(m_poolType) != m_device->GetQueueFamilyIndex(dstQueue);
    }

    void CommandBuffer::ReleaseQueueOwnership(Texture& texture, EResourceLayout oldLayout, EPipelineStageFlags oldStage,
                                              EPoolQueueType dstQueue, EResourceLayout dstLayout, EPipelineStageFlags dstStage) {
        assert(!m_isSecondary);
        assert(NeedsOwnershipTransfer(dstQueue) && "Same family: the caller transitions instead");

        const uint32_t srcFamily = m_device->GetQueueFamilyIndex(m_poolType);
        const uint32_t dstFamily = m_device->GetQueueFamilyIndex(dstQueue);

        //! Release queue ownership, no dst stages are here as they are on the other queue
        const MemoryDependency release = MakeTextureDependency(oldLayout, oldStage, dstLayout, EPipelineStageFlags::NoneBit);
        VK_SetPipelineBarrierImage(
            Util::CreateImageMemoryBarrier2(
                texture.VK_GetImage(),
                Util::ShiftToVKResourceLayout(oldLayout), Util::ShiftToVKResourceLayout(dstLayout),
                Util::ShiftToVKPipelineStageFlags2(release.srcStage), Util::ShiftToVKAccessFlags2(release.srcAccess),
                Util::ShiftToVKPipelineStageFlags2(release.dstStage), Util::ShiftToVKAccessFlags2(release.dstAccess),
                srcFamily, dstFamily,
                WholeImageRange(texture)),
            0);

        m_recordedHandoffs.push_back(QueueOwnershipHandoffPayload{
            .texture = &texture,
            .srcQueue = m_poolType,
            .dstQueue = dstQueue,
            .oldLayout = oldLayout,
            .newLayout = dstLayout,
            .dstStage = dstStage
        });
    }

    void CommandBuffer::AcquireQueueOwnership(const QueueOwnershipHandoffPayload& handoff) {
        assert(!m_isSecondary);
        assert(handoff.texture != nullptr);
        //! The acquire half only completes the transfer when it executes on the destination
        //! family's queue. Recorded anywhere else it is a no-op that also consumes the handoff
        const uint32_t srcFamily = m_device->GetQueueFamilyIndex(handoff.srcQueue);
        const uint32_t dstFamily = m_device->GetQueueFamilyIndex(handoff.dstQueue);
        assert(dstFamily == m_device->GetQueueFamilyIndex(m_poolType));

        //! Acquire image based on stored data, no src stages are here as they were on the other queue
        const MemoryDependency acquire = MakeTextureDependency(handoff.oldLayout, EPipelineStageFlags::NoneBit, handoff.newLayout, handoff.dstStage);
        VK_SetPipelineBarrierImage(
            Util::CreateImageMemoryBarrier2(
                handoff.texture->VK_GetImage(),
                Util::ShiftToVKResourceLayout(handoff.oldLayout), Util::ShiftToVKResourceLayout(handoff.newLayout),
                Util::ShiftToVKPipelineStageFlags2(acquire.srcStage), Util::ShiftToVKAccessFlags2(acquire.srcAccess),
                Util::ShiftToVKPipelineStageFlags2(acquire.dstStage), Util::ShiftToVKAccessFlags2(acquire.dstAccess),
                srcFamily, dstFamily,
                WholeImageRange(*handoff.texture)),
            0);
    }

    std::vector<QueueOwnershipHandoffPayload> CommandBuffer::TakeRecordedHandoffs() {
        return std::move(m_recordedHandoffs);
    }

    void CommandBuffer::SetViewport(Viewport viewport) const {
        VkViewport v = VkViewport{viewport.x, viewport.y, viewport.width, viewport.height, viewport.minDepth, viewport.maxDepth};
        vkCmdSetViewport(m_buffer, 0, 1, &v);
    }

    void CommandBuffer::SetScissor(Rect2D scissor) const {
        VkRect2D s = VkRect2D{
            .offset = VkOffset2D{.x = scissor.offset.x, .y = scissor.offset.y},
            .extent = VkExtent2D{.width = scissor.extent.x, .height = scissor.extent.y}
        };
        vkCmdSetScissor(m_buffer, 0, 1, &s);
    }

    void CommandBuffer::BindResourceSet(const Pipeline& pipeline, uint32_t setIdx, const ResourceSet& set) const {
        std::array descriptorSets{set.VK_Get()};
        VK_BindDescriptorSets(descriptorSets, {}, pipeline.VK_GetLayout(), pipeline.VK_GetBindPoint(), setIdx);
    }

    void CommandBuffer::BindComputePipeline(const Pipeline& pipeline) const {
        vkCmdBindPipeline(m_buffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.VK_Get());
    }

    void CommandBuffer::Dispatch(const DispatchConfig& dispatchConf) const {
        vkCmdDispatch(m_buffer, dispatchConf.groupCountX, dispatchConf.groupCountY, dispatchConf.groupCountZ);
    }

    void CommandBuffer::BufferBarrier(const BufferOpDescriptor& buffer, uint64_t size, const MemoryDependency& dependency) const {
        VkBufferMemoryBarrier2 barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };
        barrier.srcStageMask = Util::ShiftToVKPipelineStageFlags2(dependency.srcStage);
        barrier.srcAccessMask = Util::ShiftToVKAccessFlags2(dependency.srcAccess);
        barrier.dstStageMask = Util::ShiftToVKPipelineStageFlags2(dependency.dstStage);
        barrier.dstAccessMask = Util::ShiftToVKAccessFlags2(dependency.dstAccess);
        //! Buffers are concurrent and no I am not negotiating
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer = buffer.buffer->VK_Get();
        barrier.offset = buffer.offset;
        barrier.size = size;

        std::array bufBarriers{barrier};
        VK_SetPipelineBarrier({}, {}, bufBarriers, 0);
    }

    void CommandBuffer::VK_BindDescriptorSets(const std::span<VkDescriptorSet> descriptorSets,
                                           const std::span<const std::uint32_t> dynamicOffsets,
                                           const VkPipelineLayout layout, const VkPipelineBindPoint bindPoint,
                                           std::uint32_t firstSet) const {
        vkCmdBindDescriptorSets(m_buffer,
                                bindPoint,
                                layout, firstSet,
                                static_cast<uint32_t>(descriptorSets.size()),descriptorSets.data(),
                                static_cast<uint32_t>(dynamicOffsets.size()), dynamicOffsets.data());
    }

    void CommandBuffer::DrawIndexed(const DrawIndexedConfig& drawConf) const {
        vkCmdDrawIndexed(m_buffer, drawConf.indexCount, drawConf.instanceCount, drawConf.firstIndex, drawConf.vertexOffset, drawConf.firstInstance);
    }

    void CommandBuffer::Draw(const DrawConfig& drawConf) const {
        vkCmdDraw(m_buffer, drawConf.vertexCount, drawConf.instanceCount, drawConf.firstVertex, drawConf.firstInstance);
    }

    void
    CommandBuffer::BlitTexture(const TextureBlitData& srcTexture, const TextureBlitData& dstTexture, const TextureBlitRegion& blitRegion, EFilterMode filter) const {

        auto ShiftToVKBlitRegion = [](const TextureBlitRegion& region) {
            VkImageBlit blit;
            blit.srcSubresource = VkImageSubresourceLayers{
                .aspectMask = Util::ShiftToVKTextureAspect(region.srcSubresource.aspect),
                .mipLevel = region.srcSubresource.baseMipLevel,
                .baseArrayLayer = region.srcSubresource.baseArrayLayer,
                .layerCount = region.srcSubresource.layerCount
            };
            blit.dstSubresource = VkImageSubresourceLayers{
                .aspectMask = Util::ShiftToVKTextureAspect(region.destSubresource.aspect),
                .mipLevel = region.destSubresource.baseMipLevel,
                .baseArrayLayer = region.destSubresource.baseArrayLayer,
                .layerCount = region.destSubresource.layerCount
            };

            blit.srcOffsets[0] = VkOffset3D{
                .x = region.srcOffsets[0].x,
                .y = region.srcOffsets[0].y,
                .z = region.srcOffsets[0].z
            };
            blit.srcOffsets[1] = VkOffset3D{
                .x = region.srcOffsets[1].x,
                .y = region.srcOffsets[1].y,
                .z = region.srcOffsets[1].z
            };

            blit.dstOffsets[0] = VkOffset3D{
                .x = region.dstOffsets[0].x,
                .y = region.dstOffsets[0].y,
                .z = region.dstOffsets[0].z
            };
            blit.dstOffsets[1] = VkOffset3D{
                .x = region.dstOffsets[1].x,
                .y = region.dstOffsets[1].y,
                .z = region.dstOffsets[1].z
            };

            return blit;
        };

        assert(blitRegion.srcSubresource.levelCount == 1 && "BlitTexture moves a single mip level per region");
        assert(blitRegion.destSubresource.levelCount == 1 && "BlitTexture moves a single mip level per region");

        VkImageBlit blit = ShiftToVKBlitRegion(blitRegion);

        vkCmdBlitImage(m_buffer,
                       srcTexture.texture->VK_GetImage(), Util::ShiftToVKResourceLayout(srcTexture.layout),
                       dstTexture.texture->VK_GetImage(), Util::ShiftToVKResourceLayout(dstTexture.layout),
                       1, &blit,
                       Util::ShiftToVKFilterMode(filter));
    }

    void CommandBuffer::GenerateMips(Texture& texture, EResourceLayout finalLayout,
                                     EPipelineStageFlags finalStage, EFilterMode filter) {
        assert(!m_isSecondary);
        assert((texture.GetUsageFlags() & ETextureUsageFlags::TransferSrc) != ETextureUsageFlags::None &&
               "GenerateMips blits OUT of the image, so it must carry TransferSrc usage");

        const uint32_t levels = texture.GetMipCount();

        int32_t w = static_cast<int32_t>(texture.GetWidth());
        int32_t h = static_cast<int32_t>(texture.GetHeight());

        const auto mipLayers = [&texture](uint32_t level) {
            return TextureSubresourceRange{
                .aspect = texture.GetAspect(),
                .baseMipLevel = level,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = texture.GetLevels()
            };
        };

        for (uint32_t level = 1; level < levels; ++level) {
            TransitionMipRange(texture, level - 1, 1,
                EResourceLayout::TransferDstOptimal, EResourceLayout::TransferSrcOptimal,
                EPipelineStageFlags::AllTransferBit, EPipelineStageFlags::AllTransferBit);

            const int32_t nextW = std::max(w >> 1, 1);
            const int32_t nextH = std::max(h >> 1, 1);

            TextureBlitRegion region{};
            region.srcSubresource = mipLayers(level - 1);
            region.srcOffsets[1] = Offset3D{w, h, 1};
            region.destSubresource = mipLayers(level);
            region.dstOffsets[1] = Offset3D{nextW, nextH, 1};

            BlitTexture({&texture, EResourceLayout::TransferSrcOptimal},
                        {&texture, EResourceLayout::TransferDstOptimal},
                        region, filter);

            w = nextW;
            h = nextH;
        }

        //! Last level is always dst optimal while others are src as they helped gen the other mips
        if (levels > 1) {
            TransitionMipRange(texture, 0, levels - 1,
                EResourceLayout::TransferSrcOptimal, finalLayout,
                EPipelineStageFlags::AllTransferBit, finalStage);
        }
        TransitionMipRange(texture, levels - 1, 1,
            EResourceLayout::TransferDstOptimal, finalLayout,
            EPipelineStageFlags::AllTransferBit, finalStage);
    }

} // Shift::VK
