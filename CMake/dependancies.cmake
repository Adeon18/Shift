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
        GIT_TAG v1.90.4
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
FetchContent_Declare(assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG v5.3.1
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        FIND_PACKAGE_ARGS 5.3.1)
