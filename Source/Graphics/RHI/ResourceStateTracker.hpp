#ifndef SHIFT_RESOURCESTATETRACKER_HPP
#define SHIFT_RESOURCESTATETRACKER_HPP

#include <utility>
#include <vector>

#include "Common/Sync.hpp"
#include "Common/Texture.hpp"
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

        //! Submit must succeed to call this, as this moves the texture sync state to the actual texture struct which signals cimmision
        void CommitSubmitted();

        void Clear() { m_textureStates.clear(); }

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

        std::vector<std::pair<Texture*, TextureSyncState>> m_textureStates;
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
    void ResourceStateTracker<API>::TransitionTexture(CommandBuffer& cb, Texture& texture, EResourceLayout newLayout, EPipelineStageFlags newStage) {
        const TextureSyncState& syncState = Resolve(texture);
        if (syncState.layout == newLayout && syncState.stage == newStage) { return; }
        MemoryDependency memDep = MakeTextureDependency(syncState.layout, syncState.stage, newLayout, newStage);
        cb.TextureBarrier(texture, memDep, syncState.layout, newLayout, WholeTextureRange(texture));
        SetState(texture, newLayout, newStage);
    }

    template<typename API>
    void ResourceStateTracker<API>::CommitSubmitted() {
        for (auto& texPair: m_textureStates) {
            texPair.first->GetSyncStateMutable() = texPair.second;
        }
    }
} // Shift

#endif //SHIFT_RESOURCESTATETRACKER_HPP
