#ifndef SHIFT_RESOURCESTATETRACKER_HPP
#define SHIFT_RESOURCESTATETRACKER_HPP

#include <utility>
#include <vector>

#include "Common/Sync.hpp"
#include "Common/Texture.hpp"
#include "Common/Buffer.hpp"
#include "Common/CommandBuffer.hpp"
#include "Common/Types.hpp"

#ifdef SHIFT_VULKAN_BACKEND
#include "Vulkan/VKRHI_Impl.hpp"
#endif

namespace Shift {
    //! Every mip and array layer of the texture
    inline TextureSubresourceRange WholeTextureRange(const Texture& texture) {
        return TextureSubresourceRange{
            .aspect = texture.GetAspect(),
            .baseMipLevel = 0,
            .levelCount = texture.GetMipCount(),
            .baseArrayLayer = 0,
            .layerCount = texture.GetLevels()
        };
    }

    //! A tracker that auto tracks resource (buffer + texture) states by logging what transitions happened and whether submit fixed them
    template<typename API>
    class ResourceStateTracker {
    public:
        //! Add a barrier and track a new state, nothing if the transition is meaningless
        void TransitionTexture(CommandBuffer& cb, Texture& texture, EResourceLayout newLayout, EPipelineStageFlags newStage);

        //! Add a barrier and track a new state, nothing if the transition is meaningless, whole buffer
        void TransitionBuffer(CommandBuffer& cb, Buffer& buffer, EPipelineStageFlags stage, EAccessFlags access);

        //! Submit must succeed to call this, as this moves the texture sync state to the actual texture struct which signals cimmision
        void CommitSubmitted();

        void Clear() {
            m_textureStates.clear();
            m_bufferStates.clear();
        }

        //! The texture state at this point of the recording - NOT SUBMISSION!
        [[nodiscard]] EResourceLayout GetLayout(const Texture& texture) const { return Peek(texture).layout; }
        [[nodiscard]] EPipelineStageFlags GetStage(const Texture& texture) const { return Peek(texture).stage; }

        //! Set state at recording, submission stage is in the texture
        void SetState(Texture& texture, EResourceLayout layout, EPipelineStageFlags stage) {
            TextureSyncState& state = Resolve(texture);
            state.layout = layout;
            state.stage = stage;
        }

        //! The acquire's read of the image is ordered against later GPU work only through the acquire
        //! semaphore, so the first barrier after it must chain from that wait's stage, not from last
        //! frame's bottom of the pipe. Written straight into the committed state, as the frame's
        //! recording only starts after the acquire
        //! This shit is not present in dx12 so I will get fucked later
        static void OnSwapchainAcquire(Texture& texture, EPipelineStageFlags acquireWaitStage) {
            texture.GetSyncStateMutable().stage = acquireWaitStage;
        }

    private:
        //! This recording's texture state, create one from the texture sync state if absent
        TextureSyncState& Resolve(Texture& texture);

        //! This recording's texture state, if absent,. do not create but return one from texture
        [[nodiscard]] const TextureSyncState& Peek(const Texture& texture) const;

        //! This recording's buffer state, create one from the buffer sync state if absent
        BufferSyncState& Resolve(Buffer& buffer);

        std::vector<std::pair<Texture*, TextureSyncState>> m_textureStates;
        std::vector<std::pair<Buffer*, BufferSyncState>> m_bufferStates;
    };

    template<typename API>
    TextureSyncState& ResourceStateTracker<API>::Resolve(Texture& texture) {
        for (auto& [tex, state] : m_textureStates) {
            if (tex == &texture) { return state; }
        }
        return m_textureStates.emplace_back(&texture, texture.GetSyncState()).second;
    }

    template<typename API>
    const TextureSyncState& ResourceStateTracker<API>::Peek(const Texture& texture) const {
        for (const auto& [tex, state] : m_textureStates) {
            if (tex == &texture) { return state; }
        }
        return texture.GetSyncState();
    }

    template<typename API>
    BufferSyncState& ResourceStateTracker<API>::Resolve(Buffer& buffer) {
        for (auto& [buf, state] : m_bufferStates) {
            if (buf == &buffer) { return state; }
        }
        return m_bufferStates.emplace_back(&buffer, buffer.GetSyncState()).second;
    }

    template<typename API>
    void ResourceStateTracker<API>::TransitionTexture(CommandBuffer& cb, Texture& texture, EResourceLayout newLayout, EPipelineStageFlags newStage) {
        const TextureSyncState& syncState = Resolve(texture);
        if (syncState.layout == newLayout && syncState.stage == newStage) { return; }
        MemoryDependency memDep = MakeTextureDependency(syncState.layout, syncState.stage, newLayout, newStage);
        cb.TextureBarrier(texture, memDep, syncState.layout, newLayout, WholeTextureRange(texture));
        SetState(texture, newLayout, newStage);
    }

    //! This actually took me klike 2 days to come up with and I forgot how it works already....
    template<typename API>
    void ResourceStateTracker<API>::TransitionBuffer(CommandBuffer& cb, Buffer& buffer, EPipelineStageFlags stage, EAccessFlags access) {
        BufferSyncState& syncState = Resolve(buffer);
        const bool isWrite = Any(access & ACCESS_ALL_WRITE_MASK);
        MemoryDependency memDep;
        memDep.dstAccess = access;
        memDep.dstStage = stage;
        if (isWrite) {
            //! Reads only neeed exec dependency
            memDep.srcAccess = syncState.writeAccess;
            memDep.srcStage = syncState.readStages | syncState.writeStage;
        } else {
            //! This read already got its barrier
            const bool alreadyRead = (syncState.readStages & stage) == stage && (syncState.readAccess & access) == access;
            if (alreadyRead) { return; }
            memDep.srcAccess = syncState.writeAccess;
            memDep.srcStage = syncState.writeStage;
        }

        //! Nothing to wait for on a buffer the has not yet tocuhed
        if (memDep.srcStage != EPipelineStageFlags::NoneBit) {
            cb.BufferBarrier({&buffer, 0}, buffer.GetSize(), memDep);
        }

        if (isWrite) {
            syncState.writeStage = stage;
            //! DO not store the read access as we do not wait for it
            syncState.writeAccess = access & ACCESS_ALL_WRITE_MASK;
            syncState.readStages = EPipelineStageFlags::NoneBit;
            syncState.readAccess = EAccessFlags::NoneBit;
        } else {
            syncState.readStages |= stage;
            syncState.readAccess |= access;
        }
    }

    template<typename API>
    void ResourceStateTracker<API>::CommitSubmitted() {
        for (auto& texPair: m_textureStates) {
            texPair.first->GetSyncStateMutable() = texPair.second;
        }

        for (auto& bufPair: m_bufferStates) {
            bufPair.first->GetSyncStateMutable() = bufPair.second;
        }
    }
} // Shift

#endif //SHIFT_RESOURCESTATETRACKER_HPP
