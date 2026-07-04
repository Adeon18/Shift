include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)

# Window handling
FetchContent_Declare(glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.3.8
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        FIND_PACKAGE_ARGS 3.3.8)

set(GLM_TEST_MODE ON CACHE BOOL "" FORCE)
# Math
FetchContent_Declare(glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.1
        GIT_SHALLOW ON
        GIT_PROGRESS ON)

# GUI
FetchContent_Declare(imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG docking
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        FIND_PACKAGE_ARGS 1.90.4)

# Log
FetchContent_Declare(spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.11.0
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        FIND_PACKAGE_ARGS 1.11.0)

# 3D Models
# Assimp's own tests are useless to us and their gtest target breaks the `all` build
# outside a VS dev environment, so they are off
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG v5.3.1
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        FIND_PACKAGE_ARGS 5.3.1)

# DocTest test framework
set(DOCTEST_WITH_TESTS OFF CACHE BOOL "" FORCE)
set(DOCTEST_NO_INSTALL ON CACHE BOOL "" FORCE)

FetchContent_Declare(doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG v2.4.12
        GIT_SHALLOW ON
        GIT_PROGRESS ON)

# efsw - file watcher
set(STATIC_LIB ON CACHE BOOL "Build efsw as static library" FORCE)
set(BUILD_TEST_APP OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        efsw
        GIT_REPOSITORY https://github.com/SpartanJ/efsw.git
        GIT_TAG        1.0.0 # Or master
)

# Slang
set(SLANG_VERSION "2025.22.1")
set(SLANG_BASE_URL "https://github.com/shader-slang/slang/releases/download/v${SLANG_VERSION}")

if(WIN32)
    set(SLANG_ARCHIVE "slang-${SLANG_VERSION}-windows-x86_64.zip")
elseif(APPLE)
    set(SLANG_ARCHIVE "slang-${SLANG_VERSION}-macos-aarch64.zip") # or x86_64
else()
    set(SLANG_ARCHIVE "slang-${SLANG_VERSION}-linux-x86_64.zip")
endif()

FetchContent_Declare(
        slang_binaries
        URL "${SLANG_BASE_URL}/${SLANG_ARCHIVE}"
)
FetchContent_MakeAvailable(slang_binaries)

#set(slang_INCLUDE_DIRS "${slang_binaries_SOURCE_DIR}/include")

# --- Define the Target so main CMake can link it easily ---
if (NOT TARGET Slang::Slang)
    add_library(Slang::Slang SHARED IMPORTED GLOBAL)

    # 1. Setup Includes
    set_target_properties(Slang::Slang PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${slang_binaries_SOURCE_DIR}/include"
    )

    # 2. Setup Linker (.lib) and Runtime (.dll/.so) locations
    if(WIN32)
        set_target_properties(Slang::Slang PROPERTIES
                IMPORTED_IMPLIB "${slang_binaries_SOURCE_DIR}/lib/slang.lib"
                IMPORTED_LOCATION "${slang_binaries_SOURCE_DIR}/bin/slang.dll"
        )
    else()
        set_target_properties(Slang::Slang PROPERTIES
                IMPORTED_LOCATION "${slang_binaries_SOURCE_DIR}/lib/libslang.so"
        )
    endif()
endif()

# ------------ volk ------------
FetchContent_Declare(
        volk
        GIT_REPOSITORY https://github.com/zeux/volk.git
        GIT_TAG        ec6aa946cd4b8998e582f8ea2fc999a1cffdc100
)

set(VOLK_STATIC_DEFINES ON CACHE BOOL "" FORCE)
set(VOLK_INSTALL OFF CACHE BOOL "" FORCE)