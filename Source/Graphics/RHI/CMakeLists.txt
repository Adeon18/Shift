option(SHIFT_RHI_IMGUI "Build the ImGui renderer backend" OFF)
set(SHIFT_RHI_IMGUI_TARGET "" CACHE STRING "ImGui target carrying the backend's imgui_impl, linked when SHIFT_RHI_IMGUI is ON")

find_package(Threads REQUIRED)

# Utility/Vulkan moves under Vulkan/Util for now, SHOULD BE CHNAGED
file(GLOB_RECURSE SHIFT_RHI_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../Utility/Vulkan/*.cpp
)
if (NOT SHIFT_RHI_IMGUI)
    list(FILTER SHIFT_RHI_SOURCES EXCLUDE REGEX "VKImGuiBackend\\.cpp$")
endif()

add_library(shift_rhi STATIC ${SHIFT_RHI_SOURCES})
target_compile_features(shift_rhi PUBLIC cxx_std_20)
target_compile_definitions(
        shift_rhi
        PUBLIC
        SHIFT_VULKAN_BACKEND
        SHIFT_VALIDATION=1
        VK_NO_PROTOTYPES
        VMA_STATIC_VULKAN_FUNCTIONS=0
        VMA_DYNAMIC_VULKAN_FUNCTIONS=0
)
# Sizes std::arrays in RHI headers
if (DEFINED SHIFT_RHI_MAX_FRAMES_IN_FLIGHT)
    target_compile_definitions(shift_rhi PUBLIC SHIFT_RHI_MAX_FRAMES_IN_FLIGHT=${SHIFT_RHI_MAX_FRAMES_IN_FLIGHT})
endif()

# Rooted at Source/ until I FIX AND MOVE THE INCLUDES
target_include_directories(shift_rhi PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../Vendor/local
)
target_link_libraries(shift_rhi PUBLIC volk Threads::Threads)

if (SHIFT_RHI_IMGUI)
    if (NOT TARGET "${SHIFT_RHI_IMGUI_TARGET}")
        message(FATAL_ERROR "SHIFT_RHI_IMGUI is ON but SHIFT_RHI_IMGUI_TARGET ('${SHIFT_RHI_IMGUI_TARGET}') is not a target")
    endif()
    target_compile_definitions(shift_rhi PUBLIC SHIFT_RHI_IMGUI)
    target_link_libraries(shift_rhi PUBLIC ${SHIFT_RHI_IMGUI_TARGET})
endif()
