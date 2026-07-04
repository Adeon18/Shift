//
// Created by otrush on 11/20/2025.
//

#ifndef SHIFT_CAPABILITIES_HP
#define SHIFT_CAPABILITIES_HP

#include <cstdint>
#include <string>
#include <vector>

namespace Shift {
    //! Snapshot of everything the API's debug/validation layer has reported since process start
    //! The counters live in static storage inside the active backend, so they survive RHI shutdown
    struct ValidationStats {
        uint64_t errorCount = 0;
        uint64_t warningCount = 0;
        uint64_t infoCount = 0;
        //! Most recent warning/error messages, oldest first, capped at a small fixed depth
        std::vector<std::string> lastMessages;
    };

    //! Defined by the active backend (Vulkan: the debug-utils messenger sink in VKUtilCore.cpp)
    [[nodiscard]] ValidationStats GetValidationStats();
    //! Zero the counters and drop the stored messages (test isolation)
    void ResetValidationStats();

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
        bool VK_synchronization2        = false;
        bool VK_bufferDeviceAddress     = false;
        bool VK_scalarBlockLayout       = false;
        bool VK_enableValidationLayers      = false;
        //! Programmatically enable the validation layer's synchronization validation
        bool VK_syncValidation          = false;
        bool VK_requireSwapchain        = false;
        bool VK_hostQueryReset = false;
        bool VK_presentWait = false;
        bool VK_maintenance1 = false;
    };

    //! Backend-agnostic application identity passed to the RHI at bring-up. The RHI parses
    //! the user-facing version strings into components here
    struct RHIAppInfo {
        std::string appName;
        uint32_t    appVersionMajor = 1;
        uint32_t    appVersionMinor = 0;
        uint32_t    appVersionPatch = 0;

        std::string engineName;
        uint32_t    engineVersionMajor = 1;
        uint32_t    engineVersionMinor = 0;
        uint32_t    engineVersionPatch = 0;
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
        RHICommonFeature VK_synchronization2;
        RHICommonFeature VK_bufferDeviceAddress;
        RHICommonFeature VK_scalarBlockLayout;
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
            bool synchronization2         = false;
            bool bufferDeviceAddress      = false;
            bool scalarBlockLayout        = false;
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