//
// Created by otrush on 11/20/2025.
//

#ifndef SHIFT_CAPABILITIES_HP
#define SHIFT_CAPABILITIES_HP

#include <string>

namespace Shift {
    // Central struct to define which features the engine wants to request
    struct RHIRequiredFeatures {
        bool robustBufferAccess      = false;
        bool fullDrawIndexUint32     = false;
        bool imageCubeArray          = false;
        bool independentBlend        = false;
        bool geometryShader          = false;
        bool tessellationShader      = false;
        bool multiDrawIndirect       = false;
        bool drawIndirectCount       = false;
        bool samplerAnisotropy       = true;
        bool depthClamp              = true;
        bool fillModeNonSolid        = false;
        bool pipelineStatisticsQuery = false;
        bool textureCompressionBC    = false;
        bool textureCompressionASTC  = false;
        bool textureCompressionETC2  = false;
        bool computeShader           = true;
        bool dualSrcBlend            = false;

        // Metal-specific
        bool MTL_argumentBuffers         = false;

        // Vulkan extensions
        bool VK_timelineSemaphore       = false;
        bool VK_descriptorIndexing      = false;
        bool VK_dynamicRendering      = false;
        bool VK_enableValidationLayers      = false;
        bool VK_requireSwapchain        = false;
        bool VK_hostQueryReset = false;
        bool VK_presentWait = false;
        bool VK_maintenance1 = false;
    };

    struct RHILimits {
        uint32_t maxTextureDimension1D = 0;
        uint32_t maxTextureDimension2D = 0;
        uint32_t maxTextureDimension3D = 0;
        uint32_t maxTextureArrayLayers = 0;
        uint32_t maxVertexAttributes = 0;
        uint32_t maxPushConstantsSize = 0;
        uint32_t maxSamplerAnisotropy = 0;
        uint32_t maxColorAttachments = 0;
        uint32_t maxComputeWorkGroupCount[3] = {0,0,0};
        uint32_t maxComputeWorkGroupSize[3] = {0,0,0};
        uint32_t maxComputeSharedMemorySize = 0;
    };

    struct RHICommonFeature {
        bool requested = false;  // set from RHIRequiredFeatures
        bool supported = false;  // set from physical device query
    };

    struct RHICommonFeatures {
        RHICommonFeature robustBufferAccess;
        RHICommonFeature fullDrawIndexUint32;
        RHICommonFeature imageCubeArray;
        RHICommonFeature independentBlend;
        RHICommonFeature geometryShader;
        RHICommonFeature tessellationShader;
        RHICommonFeature multiDrawIndirect;
        RHICommonFeature drawIndirectCount;
        RHICommonFeature samplerAnisotropy;
        RHICommonFeature depthClamp;
        RHICommonFeature fillModeNonSolid;
        RHICommonFeature pipelineStatisticsQuery;
        RHICommonFeature textureCompressionBC;
        RHICommonFeature textureCompressionASTC;
        RHICommonFeature textureCompressionETC2;
        RHICommonFeature computeShader;
        RHICommonFeature dualSrcBlend;

        // Vulkan convenience
        RHICommonFeature VK_timelineSemaphores;
        RHICommonFeature VK_descriptorIndexing;
        RHICommonFeature VK_dynamicRendering;
        RHICommonFeature VK_hostQueryReset;
        RHICommonFeature VK_presentWait;
        RHICommonFeature VK_maintenance1;
    };

    struct RHIVersion {
        uint32_t apiVersionMajor   = 0;
        uint32_t apiVersionMinor   = 0;
        uint32_t driverVersion     = 0;
        std::string deviceName;
    };

    struct RHICapabilities {
        RHICommonFeatures features;
        RHILimits limits;
        RHIVersion version;

        struct VulkanExtensions {
            bool timelineSemaphore        = false;
            bool descriptorIndexing       = false;
            bool dynamicRendering         = false;
            bool hostQueryReset           = false;
            bool samplerAnisotropy        = false;
            bool multiDrawIndirect        = false;
            bool drawIndirectCount        = false;
            bool presentWait              = false;
            bool maintenance1             = false;
        } vkExtensions;
    };
}

#endif //SHIFT_CAPABILITIES_HP