#include "VKImGuiBackend.hpp"

#include "Graphics/UI/imgui/imgui_impl_vulkan.h"

#include "VKSampler.hpp"
#include "VKTexture.hpp"
#include "VKCommandBuffer.hpp"

#include "Utility/Vulkan/VKUtilRHI.hpp"
#include "Utility/Logging/LogMacros.hpp"

namespace Shift::VK {

    void ImGuiBackend::Init(const RHILocal<RHI::Vulkan>& local) {
        ImGui_ImplVulkan_InitInfo info = {};
        info.Instance       = local.instance->Get();
        info.PhysicalDevice = local.device->GetPhysicalDevice();
        info.Device         = local.device->Get();
        info.QueueFamily    = *local.device->GetQueueFamilyIndices().graphicsFamily;
        info.Queue          = local.device->GetGraphicsQueue();
        info.PipelineCache  = VK_NULL_HANDLE;
        info.DescriptorPool = local.descAllocator->GetImGuiPool();

        //! TODO [BUG] This shit
        const uint32_t swapchainImageCount = static_cast<uint32_t>(local.swapchain->GetImages().size());
        info.MinImageCount = swapchainImageCount;
        info.ImageCount    = swapchainImageCount;

        // Validation/Error Checking
        info.CheckVkResultFn = [](VkResult err) {
            if (err != VK_SUCCESS) Log(Error, "ImGui Vulkan Error: {}\n", static_cast<uint32_t>(err));
        };

        info.UseDynamicRendering = true;

        static VkFormat mainColorFormat = Util::ShiftToVKTextureFormat(local.swapchain->GetFormat());

        info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        info.PipelineInfoMain.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
        info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &mainColorFormat;

        static VkFormat viewportColorFormat = VK_FORMAT_B8G8R8A8_UNORM;

        info.PipelineInfoForViewports.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        info.PipelineInfoForViewports.PipelineRenderingCreateInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
        info.PipelineInfoForViewports.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        info.PipelineInfoForViewports.PipelineRenderingCreateInfo.pColorAttachmentFormats = &viewportColorFormat;

        ImGui_ImplVulkan_Init(&info);
    }

    void ImGuiBackend::Shutdown() {
        ImGui_ImplVulkan_Shutdown();
    }

    void ImGuiBackend::BeginFrame() {
        ImGui_ImplVulkan_NewFrame();
    }

    void ImGuiBackend::RenderDrawData(ImDrawData* drawData, CommandBuffer& cmd) {
        ImGui_ImplVulkan_RenderDrawData(drawData, cmd.VK_Get());
    }

    void* ImGuiBackend::RegisterTexture(const Texture& texture, const Sampler& sampler) {
        return ImGui_ImplVulkan_AddTexture(
            sampler.VK_Get(), texture.GetView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    void ImGuiBackend::UnregisterTexture(void* textureId) {
        if (textureId) {
            ImGui_ImplVulkan_RemoveTexture(static_cast<VkDescriptorSet>(textureId));
        }
    }
} // Shift::VK
