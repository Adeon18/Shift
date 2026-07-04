cmake_minimum_required(VERSION 3.6.0)
set (CMAKE_CXX_STANDARD 20)

set(NAME Shift)

get_filename_component(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)

message(STATUS "using ${CMAKE_GENERATOR}")
if (CMAKE_GENERATOR STREQUAL "MinGW Makefiles")
    if (NOT MINGW_PATH)
      message(FATAL_ERROR "MINGW_PATH not set in .env.cmake")
    endif()
    set(USE_MINGW "True")
    set(CMAKE_C_COMPILER ${MINGW_PATH}/bin/gcc.exe)
    set(CMAKE_CXX_COMPILER  ${MINGW_PATH}/bin/g++.exe)
endif()

project(${NAME} VERSION 0.23.0)


include(CMake/dependancies.cmake)

FetchContent_MakeAvailable(glfw)
FetchContent_MakeAvailable(glm)
FetchContent_MakeAvailable(assimp)
FetchContent_MakeAvailable(spdlog)
FetchContent_MakeAvailable(efsw)
FetchContent_MakeAvailable(volk)
FetchContent_MakeAvailable(doctest)

# ----------------------------------------------------------------------------
# shift_core: the whole engine as a static library.
# The Shift app and shift_tests are thin executables on top of it
# ----------------------------------------------------------------------------
file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/Source/*.cpp)
list(REMOVE_ITEM SOURCES ${PROJECT_SOURCE_DIR}/Source/AppDefault.cpp)

add_library(shift_core STATIC ${SOURCES})
target_compile_features(shift_core PUBLIC cxx_std_20)
# PUBLIC on purpose: these defines shape headers (backend selection, volk prototypes,
# VMA mode), so every consumer (engine exe, tests) must compile with the same set
target_compile_definitions(
        shift_core
        PUBLIC
        SHIFT_ROOT="${PROJECT_SOURCE_DIR}"
        SHIFT_VALIDATION=1
        SHIFT_VULKAN_BACKEND

        VK_NO_PROTOTYPES
        IMGUI_IMPL_VULKAN_USE_VOLK
        # Since we use volk, VMA requiires this
        VMA_STATIC_VULKAN_FUNCTIONS=0
        VMA_DYNAMIC_VULKAN_FUNCTIONS=0
        _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING

        GLM_FORCE_DEPTH_ZERO_TO_ONE
        GLM_FORCE_RADIANS
)

# ----------------------------------------------------------------------------
# Shift, the application: just the entry point linked against the engine
# ----------------------------------------------------------------------------
add_executable(Shift ${PROJECT_SOURCE_DIR}/Source/AppDefault.cpp)
target_link_libraries(Shift PRIVATE shift_core)
set_property(TARGET Shift PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Build")

# ----------------------------------------------------------------------------
# shift_tests, doctest suite, registered with CTest. Separate target: building
# or breaking tests never touches the engine exe. Run via `ctest` (add -C <cfg>
# on multi-config generators) or by launching shift_tests directly.
# ----------------------------------------------------------------------------
enable_testing()

file(GLOB_RECURSE TEST_SOURCES ${PROJECT_SOURCE_DIR}/Tests/*.cpp)
add_executable(shift_tests ${TEST_SOURCES})
target_link_libraries(shift_tests PRIVATE shift_core doctest::doctest)

# One ctest entry running the whole exe: doctest's per-case discovery would execute the
# binary at build time, which is fragile with runtime DLLs (slang). Granularity via
# `shift_tests --test-case=<filter>` when needed.
add_test(NAME shift_tests COMMAND shift_tests)
# A regression that deadlocks (the executor's old under-lock callbacks) must fail, not hang
set_tests_properties(shift_tests PROPERTIES TIMEOUT 300)

if (WIN32)
  message(STATUS "CREATING BUILD FOR WINDOWS")

  if (USE_MINGW)
    target_include_directories(shift_core PUBLIC
      ${MINGW_PATH}/include
    )
    target_link_directories(shift_core PUBLIC
      ${MINGW_PATH}/lib
    )
  endif()
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)

  target_include_directories(shift_core PUBLIC
    ${PROJECT_SOURCE_DIR}/Source
    ${PROJECT_SOURCE_DIR}/Vendor/local
    ${Vulkan_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIRS}
    ${assimp_INCLUDE_DIRS}
    ${efsw_SOURCE_DIR}/include
    )

  target_link_libraries(shift_core PUBLIC
      glfw
      volk
      spdlog::spdlog
      Threads::Threads
      assimp
      Slang::Slang
      efsw
  )

  # CRITICAL: Copy the slang runtime DLLs next to every executable that links the engine
  function(shift_copy_slang_dlls TARGET_NAME)
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:Slang::Slang>
        $<TARGET_FILE_DIR:${TARGET_NAME}>
        COMMENT "Copying slang.dll next to ${TARGET_NAME}"
    )
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${slang_binaries_SOURCE_DIR}/bin/slang-compiler.dll"
        $<TARGET_FILE_DIR:${TARGET_NAME}>
        COMMENT "Copying slang-compiler.dll next to ${TARGET_NAME}"
    )
  endfunction()

  shift_copy_slang_dlls(Shift)
  shift_copy_slang_dlls(shift_tests)

elseif (UNIX)
    message(STATUS "CREATING BUILD FOR UNIX")
    target_include_directories(shift_core PUBLIC
      ${PROJECT_SOURCE_DIR}/Source
      ${PROJECT_SOURCE_DIR}/Vendor/local
    )
    target_link_libraries(shift_core PUBLIC
        glfw
        volk
        imgui
        spdlog::spdlog
        ${Vulkan_LIBRARIES}
        Slang::Slang
        efsw
    )
endif()
