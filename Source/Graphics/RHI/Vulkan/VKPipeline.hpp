#ifndef SHIFT_VKPIPELINE_HPP
#define SHIFT_VKPIPELINE_HPP

#include <optional>
#include <span>

#include "VKDevice.hpp"
#include "VKShader.hpp"

#include "Graphics/RHI/Pipeline.hpp"

namespace Shift::VK {
    class Pipeline {
    public:
        Pipeline() = default;
        Pipeline(const Pipeline&) = delete;
        Pipeline& operator=(const Pipeline&) = delete;

        //! Initialize a pipeline
        //! \param device
        //! \param descriptor The pipeline desc struct
        //! \param shaders The runtime built shader strcutures with type and Data
        //! \param descLayouts The desc layouts have to already be created, for now we expect the API to create them beforehand
        //! \return true if successful, false otherwise
        [[nodiscard]] bool Init(const Device* device, const PipelineDescriptor& descriptor, const std::vector<ShaderStageDesc>& shaders, std::span<VkDescriptorSetLayout> descLayouts);


        //! API SPECIFIC, DO NOT USE UNLESS NESSESARY IN RHI SPECIFIC CODE
        //! \return VkPipeline
        [[nodiscard]] VkPipeline VK_Get() const { return m_pipeline; }
        //! API SPECIFIC, DO NOT USE UNLESS NESSESARY IN RHI SPECIFIC CODE
        //! \return VkPipelineLayout
        [[nodiscard]] VkPipelineLayout VK_GetLayout() const { return m_layout; }
        [[nodiscard]] const PipelineDescriptor& GetDescriptor() const { return m_desc; }

        void Destroy();
        ~Pipeline() = default;
    private:
        const Device* m_device = nullptr;

        VkPipeline m_pipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_layout = VK_NULL_HANDLE;

        PipelineDescriptor m_desc;
    };

    ASSERT_INTERFACE(IPipeline, Pipeline);
} // Shift::VK

#endif //SHIFT_VKPIPELINE_HPP
