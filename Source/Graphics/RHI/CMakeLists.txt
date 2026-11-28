option(SHIFT_RHI_IMGUI "Build the ImGui renderer backend" OFF)
set(SHIFT_RHI_IMGUI_TARGET "" CACHE STRING "ImGui target carrying the backend's imgui_impl, linked when SHIFT_RHI_IMGUI is ON")

set(SHIFT_RHI_BACKEND "Vulkan" CACHE STRING "RHI backend: Vulkan or DX12")
if (NOT SHIFT_RHI_BACKEND MATCHES "^(Vulkan|DX12)$")
    message(FATAL_ERROR "SHIFT_RHI_BACKEND must be Vulkan or DX12, got '${SHIFT_RHI_BACKEND}'")
endif()

find_package(Threads REQUIRED)

file(GLOB_RECURSE SHIFT_RHI_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
# Utility/Vulkan moves under Vulkan/Util for now, SHOULD BE CHNAGED
file(GLOB_RECURSE SHIFT_RHI_VULKAN_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/Vulkan/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../Utility/Vulkan/*.cpp
)
file(GLOB_RECURSE SHIFT_RHI_DX12_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/DX12/*.cpp)
file(GLOB SHIFT_RHI_RUNNER_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/Runner/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Samples/*.cpp
)
list(REMOVE_ITEM SHIFT_RHI_SOURCES ${SHIFT_RHI_RUNNER_SOURCES} ${SHIFT_RHI_VULKAN_SOURCES} ${SHIFT_RHI_DX12_SOURCES})
if (SHIFT_RHI_BACKEND STREQUAL "DX12")
    if (SHIFT_RHI_IMGUI)
        message(FATAL_ERROR "SHIFT_RHI_IMGUI has no DX12 backend yet")
    endif()
    list(APPEND SHIFT_RHI_SOURCES ${SHIFT_RHI_DX12_SOURCES})
else()
    if (NOT SHIFT_RHI_IMGUI)
        list(FILTER SHIFT_RHI_VULKAN_SOURCES EXCLUDE REGEX "VKImGuiBackend\\.cpp$")
    endif()
    list(APPEND SHIFT_RHI_SOURCES ${SHIFT_RHI_VULKAN_SOURCES})
endif()

add_library(shift_rhi STATIC ${SHIFT_RHI_SOURCES})
target_compile_features(shift_rhi PUBLIC cxx_std_20)
target_compile_definitions(shift_rhi PUBLIC SHIFT_VALIDATION=1)
if (SHIFT_RHI_BACKEND STREQUAL "DX12")
    target_compile_definitions(shift_rhi PUBLIC SHIFT_DX12_BACKEND)
    target_link_libraries(shift_rhi PUBLIC d3d12 dxgi dxguid)
else()
    target_compile_definitions(
            shift_rhi
            PUBLIC
            SHIFT_VULKAN_BACKEND
            VK_NO_PROTOTYPES
            VMA_STATIC_VULKAN_FUNCTIONS=0
            VMA_DYNAMIC_VULKAN_FUNCTIONS=0
    )
    target_link_libraries(shift_rhi PUBLIC volk)
endif()
# Sizes std::arrays in RHI headers
if (DEFINED SHIFT_RHI_MAX_FRAMES_IN_FLIGHT)
    target_compile_definitions(shift_rhi PUBLIC SHIFT_RHI_MAX_FRAMES_IN_FLIGHT=${SHIFT_RHI_MAX_FRAMES_IN_FLIGHT})
endif()

# Rooted at Source/ until I FIX AND MOVE THE INCLUDES
target_include_directories(shift_rhi PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../..
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../Vendor/local
)
target_link_libraries(shift_rhi PUBLIC Threads::Threads)

if (SHIFT_RHI_IMGUI)
    if (NOT TARGET "${SHIFT_RHI_IMGUI_TARGET}")
        message(FATAL_ERROR "SHIFT_RHI_IMGUI is ON but SHIFT_RHI_IMGUI_TARGET ('${SHIFT_RHI_IMGUI_TARGET}') is not a target")
    endif()
    target_compile_definitions(shift_rhi PUBLIC SHIFT_RHI_IMGUI)
    target_link_libraries(shift_rhi PUBLIC ${SHIFT_RHI_IMGUI_TARGET})
endif()

# Sample shaders, compiled at build time: <name>.vs / <name>.ps from mainVS / mainPS
set(SHIFT_RHI_SLANGC "${slang_binaries_SOURCE_DIR}/bin/slangc${CMAKE_EXECUTABLE_SUFFIX}")
set(SHIFT_RHI_SAMPLE_SHADERS_OUT ${CMAKE_CURRENT_BINARY_DIR}/SampleShaders)
file(GLOB SHIFT_RHI_SAMPLE_SHADERS ${CMAKE_CURRENT_SOURCE_DIR}/Samples/Shaders/*.slang)
if (SHIFT_RHI_BACKEND STREQUAL "DX12")
    set(SHIFT_RHI_SAMPLE_SHADER_EXT ".dxil")
    # The pinned DXC, never the one on PATH (the Vulkan SDK ships its own)
    set(SHIFT_RHI_SLANGC_TARGET -target dxil -profile sm_6_6 -dxc-path ${dxc_SOURCE_DIR}/bin/x64)
    set(SHIFT_RHI_SLANGC_DEPENDS ${dxc_SOURCE_DIR}/bin/x64/dxcompiler.dll)
else()
    set(SHIFT_RHI_SAMPLE_SHADER_EXT ".spv")
    set(SHIFT_RHI_SLANGC_TARGET -target spirv -profile glsl_450 -emit-spirv-directly -fvk-use-scalar-layout)
    set(SHIFT_RHI_SLANGC_DEPENDS)
endif()
set(SHIFT_RHI_SAMPLE_BYTECODE)
foreach(shader ${SHIFT_RHI_SAMPLE_SHADERS})
    get_filename_component(name ${shader} NAME_WE)
    foreach(stage IN ITEMS vs ps)
        if (stage STREQUAL "vs")
            set(entry mainVS)
            set(slangStage vertex)
        else()
            set(entry mainPS)
            set(slangStage fragment)
        endif()
        set(output ${SHIFT_RHI_SAMPLE_SHADERS_OUT}/${name}.${stage}${SHIFT_RHI_SAMPLE_SHADER_EXT})
        add_custom_command(
                OUTPUT ${output}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${SHIFT_RHI_SAMPLE_SHADERS_OUT}
                COMMAND ${SHIFT_RHI_SLANGC} ${shader} -entry ${entry} -stage ${slangStage}
                        ${SHIFT_RHI_SLANGC_TARGET} -matrix-layout-row-major -o ${output}
                DEPENDS ${shader} ${SHIFT_RHI_SLANGC} ${SHIFT_RHI_SLANGC_DEPENDS}
                VERBATIM
        )
        list(APPEND SHIFT_RHI_SAMPLE_BYTECODE ${output})
    endforeach()
endforeach()
add_custom_target(shift_rhi_sample_shaders DEPENDS ${SHIFT_RHI_SAMPLE_BYTECODE})

# Sample runner, headless
add_executable(shift_rhi_samples ${SHIFT_RHI_RUNNER_SOURCES})
add_dependencies(shift_rhi_samples shift_rhi_sample_shaders)
target_link_libraries(shift_rhi_samples PRIVATE shift_rhi)
target_include_directories(shift_rhi_samples PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Runner
        ${CMAKE_CURRENT_SOURCE_DIR}/Samples
        ${CMAKE_CURRENT_SOURCE_DIR}/Vendor
)
target_compile_definitions(shift_rhi_samples PRIVATE
        SHIFT_RHI_GOLDENS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Samples/Goldens"
        SHIFT_RHI_REPORT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Report"
        SHIFT_RHI_SAMPLE_SHADERS_DIR="${SHIFT_RHI_SAMPLE_SHADERS_OUT}"
        SHIFT_RHI_SAMPLE_SHADER_EXT="${SHIFT_RHI_SAMPLE_SHADER_EXT}"
)
