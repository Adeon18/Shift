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
        m_textureStates.clear();
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

        m_textureStates.clear();
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

        //! Secondaries never record but this is just in case
        m_textureStates.clear();
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

    void CommandBuffer::CopyBufferToTexture(const BufferOpDescriptor& srcBuf, const TextureCopyDescriptor& dstTex) const {
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        //! TODO: [BUG] Subresource range don't wotrk
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

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

    void CommandBuffer::VK_TransferImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
                                            VkPipelineStageFlags2 srcStage, VkPipelineStageFlags2 dstStage,
                                            VkImageSubresourceRange subresourceRange) const {
        //! Same-queue barrier: no ownership changes hands, so both families are IGNORED
        VK_SetPipelineBarrierImage(
            Util::CreateImageMemoryBarrier2(
                image, oldLayout, newLayout, srcStage, dstStage,
                VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
                subresourceRange),
            0);
    }

    void CommandBuffer::VK_TransferImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
                                            VkPipelineStageFlags2 srcStage, VkPipelineStageFlags2 dstStage, bool isDepth) const {

        VkImageSubresourceRange subresourceRange{};
        subresourceRange.aspectMask = (isDepth) ? VK_IMAGE_ASPECT_DEPTH_BIT: VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = 1;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = 1;

        VK_TransferImageLayout(image, oldLayout, newLayout, srcStage, dstStage, subresourceRange);
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
            //! Shared chain point with Swapchain::AquireNextImage's tracked-stage seeding
            waitStages.push_back(static_cast<VkPipelineStageFlags>(BINARY_WAIT_DST_STAGES));
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

        //! Submit was succesfull, so we record the latest texture's submitted state into them
        for (auto& [texture, state] : m_textureStates) {
            texture->VK_CommitSubmittedState(state.layout, state.stage);
        }
        m_textureStates.clear();

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

    void CommandBuffer::BeginRenderPass(const RenderPassDescriptor& desc, std::span<Texture*> colorTextures, std::optional<Texture*> depthTexture) const {
        assert(desc.colorAttachments.size() == colorTextures.size());
        assert(desc.depthAttachment.has_value() == depthTexture.has_value());

        std::vector<VkRenderingAttachmentInfo> colorInfo;
        std::optional<VkRenderingAttachmentInfo> depthInfo;
        for (uint32_t i = 0; i < desc.colorAttachments.size(); i++) {
            const Texture* colTex = colorTextures[i];
            const RenderPassDescriptor::RenderPassAttachmentInfo& att = desc.colorAttachments[i];
            //! The attachment layout is whatever this recording has transitioned the texture to (not the submitted one!!!!!!!)
            colorInfo.push_back(Util::CreateRenderingAttachmentInfo(
                    colTex->VK_GetView(),
                    PeekTextureState(*colTex).layout,
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
                    PeekTextureState(**depthTexture).layout,
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

    CommandBuffer::TextureTrackedState& CommandBuffer::ResolveTextureState(Texture& texture) {
        for (auto& [tex, state] : m_textureStates) {
            if (tex == &texture) { return state; }
        }
        //! Initialize from tetxure's last submitted state
        return m_textureStates.emplace_back(
            &texture, TextureTrackedState{texture.VK_GetSubmittedLayout(), texture.VK_GetSubmittedStage()}).second;
    }

    CommandBuffer::TextureTrackedState CommandBuffer::PeekTextureState(const Texture& texture) const {
        for (const auto& [tex, state] : m_textureStates) {
            if (tex == &texture) { return state; }
        }
        return {texture.VK_GetSubmittedLayout(), texture.VK_GetSubmittedStage()};
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

    void CommandBuffer::TransitionTexture(Texture& texture, EResourceLayout newLayout, EPipelineStageFlags newStageFlags) {
        //! Barriers can only be done by a primary buffer, as secondaries run only after beginrenderpass
        //! where transitions are forbidden
        assert(!m_isSecondary);

        const VkImageLayout dstLayout = Util::ShiftToVKResourceLayout(newLayout);
        const VkPipelineStageFlags2 dstStage = Util::ShiftToVKPipelineStageFlags2(newStageFlags);

        TextureTrackedState& trackedState = ResolveTextureState(texture);

        //! Early exit as no layout to change
        if (trackedState.layout == dstLayout && trackedState.stage == dstStage) {
            return;
        }

        VK_TransferImageLayout(
            texture.VK_GetImage(),
            trackedState.layout,
            dstLayout,
            trackedState.stage,
            dstStage,
            WholeImageRange(texture)
        );

        trackedState = {dstLayout, dstStage};
    }

    void CommandBuffer::ReleaseQueueOwnership(Texture& texture, EPoolQueueType dstQueue,
                                              EResourceLayout dstLayout, EPipelineStageFlags dstStage) {
        assert(!m_isSecondary);

        const uint32_t srcFamily = m_device->GetQueueFamilyIndex(m_poolType);
        const uint32_t dstFamily = m_device->GetQueueFamilyIndex(dstQueue);

        //! Nothing to hand over when both queue are on 1 family
        if (srcFamily == dstFamily) {
            TransitionTexture(texture, dstLayout, dstStage);
            return;
        }

        const VkImageLayout newLayout = Util::ShiftToVKResourceLayout(dstLayout);
        const VkPipelineStageFlags2 acquireStage = Util::ShiftToVKPipelineStageFlags2(dstStage);

        //! The handoff and the post-release state both need the pre-release one
        const TextureTrackedState trackedState = ResolveTextureState(texture);
        const VkImageLayout oldLayout = trackedState.layout;
        const VkPipelineStageFlags2 srcStage = trackedState.stage;

        //! Release queue ownership, no dst stages are here as they are on the other queue
        VK_SetPipelineBarrierImage(
            Util::CreateImageMemoryBarrier2(
                texture.VK_GetImage(),
                oldLayout, newLayout,
                srcStage, VK_PIPELINE_STAGE_2_NONE,
                srcFamily, dstFamily,
                WholeImageRange(texture)),
            0);

        m_recordedHandoffs.push_back(QueueOwnershipHandoff{
            .texture = &texture,
            .srcFamily = srcFamily,
            .dstFamily = dstFamily,
            .oldLayout = oldLayout,
            .newLayout = newLayout,
            .dstStage = acquireStage
        });

        //! For the rest of THIS recording has the correct stage and layout but it is left
        //! kind of useless as the other queue will soon get it
        ResolveTextureState(texture) = {newLayout, VK_PIPELINE_STAGE_2_NONE};
    }

    void CommandBuffer::AcquireQueueOwnership(const QueueOwnershipHandoff& handoff) {
        assert(!m_isSecondary);
        assert(handoff.texture != nullptr);
        //! The acquire half only completes the transfer when it executes on the destination
        //! family's queue. Recorded anywhere else it is a no-op that also consumes the handoff
        assert(handoff.dstFamily == m_device->GetQueueFamilyIndex(m_poolType));

        //! Acquire image based on stored data
        VK_SetPipelineBarrierImage(
            Util::CreateImageMemoryBarrier2(
                handoff.texture->VK_GetImage(),
                handoff.oldLayout, handoff.newLayout,
                VK_PIPELINE_STAGE_2_NONE, handoff.dstStage,
                handoff.srcFamily, handoff.dstFamily,
                WholeImageRange(*handoff.texture)),
            0);

        //! Log the texture
        ResolveTextureState(*handoff.texture) = {handoff.newLayout, handoff.dstStage};
    }

    std::vector<QueueOwnershipHandoff> CommandBuffer::TakeRecordedHandoffs() {
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
                .mipLevel = region.srcSubresource.levelCount,
                .baseArrayLayer = region.srcSubresource.baseArrayLayer,
                .layerCount = region.srcSubresource.layerCount
            };
            blit.dstSubresource = VkImageSubresourceLayers{
                .aspectMask = Util::ShiftToVKTextureAspect(region.destSubresource.aspect),
                .mipLevel = region.destSubresource.levelCount,
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

        VkImageBlit blit = ShiftToVKBlitRegion(blitRegion);

        vkCmdBlitImage(m_buffer,
                       srcTexture.texture->VK_GetImage(), PeekTextureState(*srcTexture.texture).layout,
                       dstTexture.texture->VK_GetImage(), PeekTextureState(*dstTexture.texture).layout,
                       1, &blit,
                       Util::ShiftToVKFilterMode(filter));
    }
} // Shift::VK
