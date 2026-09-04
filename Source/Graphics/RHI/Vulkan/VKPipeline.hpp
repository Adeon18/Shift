#ifndef SHIFT_VKPIPELINE_HPP
#define SHIFT_VKPIPELINE_HPP

#include <optional>
#include <span>

#include "VKDevice.hpp"
#include "VKShader.hpp"

#include "../Common/Pipeline.hpp"

namespace Shift::VK {
    class Pipeline {
        friend VK::CommandBuffer;
    public:
        //! Initialize a pipeline
        //! \param device
        //! \param descriptor The pipeline desc struct
        //! \param shaders The runtime built shader strcutures with type and Data
        //! \param descLayouts The desc layouts have to already be created, for now we expect the API to create them beforehand
        Pipeline(const Device* device, const PipelineDescriptor& descriptor, const std::vector<ShaderStageDesc>& shaders, std::span<VkDescriptorSetLayout> descLayouts);

        [[nodiscard]] bool IsValid() const { return m_valid; }

        //! For hot-reloading: builds a fresh GPU pipeline from the given stages and retires the
        //! previous one for deferred destruction (ReleaseRetired()).
        void Rebuild(bool createLayout, std::span<const ShaderStageDesc> stages);

        //! Releases the oldest pipeline state retired by Rebuild() (one per Rebuild).
        //! Driven by the RHI deferred executor once the GPU is done with it. RHI-level
        //! so the backend-agnostic RHI template can call it without touching VkPipeline.
        void ReleaseRetired();

        [[nodiscard]] const PipelineDescriptor& GetDescriptor() const { return m_desc; }

        ~Pipeline();
    private:

        void InitInternal(bool createLayout);

        //! API SPECIFIC, DO NOT USE UNLESS NESSESARY IN RHI SPECIFIC CODE (backend-only, friended)
        //! \return VkPipeline
        [[nodiscard]] VkPipeline VK_Get() const { return m_pipeline; }
        //! API SPECIFIC, DO NOT USE UNLESS NESSESARY IN RHI SPECIFIC CODE (backend-only, friended)
        //! \return VkPipelineLayout
        [[nodiscard]] VkPipelineLayout VK_GetLayout() const { return m_layout; }

        const Device* m_device = nullptr;

        VkPipeline m_pipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_layout = VK_NULL_HANDLE;
        //! Previous m_pipeline handles retired by Rebuild(), awaiting deferred destruction (FIFO)
        std::vector<VkPipeline> m_retiredHandles;

        PipelineDescriptor m_desc;
        bool m_valid = false;

        std::vector<ShaderStageDesc> m_shaders;
        std::vector<VkDescriptorSetLayout> m_descLayouts;
    };

    ASSERT_INTERFACE(IPipeline, Pipeline);
} // Shift::VK

#endif //SHIFT_VKPIPELINE_HPP
