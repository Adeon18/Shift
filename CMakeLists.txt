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

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/Source/*.cpp)
 
add_executable(${PROJECT_NAME} ${SOURCES})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Build")
target_compile_definitions(
        ${PROJECT_NAME}
        PRIVATE
        SHIFT_ROOT="${PROJECT_SOURCE_DIR}"
        SHIFT_VALIDATION=1
        SHIFT_VULKAN_BACKEND

        VK_NO_PROTOTYPES
        IMGUI_IMPL_VULKAN_USE_VOLK
        # Since we use volk, VMA requiires this
        VMA_STATIC_VULKAN_FUNCTIONS=0
        VMA_DYNAMIC_VULKAN_FUNCTIONS=0
        _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING
)
target_compile_definitions(
        ${PROJECT_NAME}
        PUBLIC
        GLM_FORCE_DEPTH_ZERO_TO_ONE
        GLM_FORCE_RADIANS
)
 
if (WIN32)
  message(STATUS "CREATING BUILD FOR WINDOWS")
 
  if (USE_MINGW)
    target_include_directories(${PROJECT_NAME} PUBLIC
      ${MINGW_PATH}/include
    )
    target_link_directories(${PROJECT_NAME} PUBLIC
      ${MINGW_PATH}/lib
    )
  endif()
  set(THREADS_PREFER_PTHREAD_FLAG ON)
  find_package(Threads REQUIRED)
 
  target_include_directories(${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/Source
    ${PROJECT_SOURCE_DIR}/Vendor/local
    ${Vulkan_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIRS}
    ${assimp_INCLUDE_DIRS}
    ${efsw_SOURCE_DIR}/include
    )
 
#  target_link_directories(${PROJECT_NAME} PUBLIC
#    ${Vulkan_LIBRARIES}
#  )
 
  target_link_libraries(${PROJECT_NAME}
      glfw
      volk
      spdlog::spdlog
      Threads::Threads
      assimp
      Slang::Slang
      efsw
  )

  # --- CRITICAL: Copy slang.dll to output folder ---
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:Slang::Slang>
      $<TARGET_FILE_DIR:${PROJECT_NAME}>
      COMMENT "Copying slang.dll to output directory"
  )

  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${slang_binaries_SOURCE_DIR}/bin/slang-compiler.dll"
          $<TARGET_FILE_DIR:${PROJECT_NAME}>
          COMMENT "Copying slang-compiler.dll to output directory"
  )

elseif (UNIX)
    message(STATUS "CREATING BUILD FOR UNIX")
    target_include_directories(${PROJECT_NAME} PUBLIC
      ${PROJECT_SOURCE_DIR}/Source
      ${PROJECT_SOURCE_DIR}/Vendor/local
    )
    target_link_libraries(${PROJECT_NAME}
        glfw
        volk
        imgui
        spdlog::spdlog
        ${Vulkan_LIBRARIES}
        Slang::Slang
        efsw
    )
endif()
