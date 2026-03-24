cmake_minimum_required(VERSION 3.16)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  if(APPLE)
    project(MetalCraft LANGUAGES CXX OBJCXX)
  else()
    project(MetalCraft LANGUAGES CXX)
  endif()
endif()

enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(METALCRAFT_BUILD_TESTS "Build MetalCraft local tests" ON)
option(METALCRAFT_ENABLE_SHADER_TRANSLATOR "Build shader translation support" ON)

set(METALCRAFT_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")
set(METALCRAFT_EXTERNAL_DIR "${CMAKE_CURRENT_LIST_DIR}/../external/MobileGlues/MobileGlues-cpp")
set(METALCRAFT_GLSLANG_DIR "${METALCRAFT_EXTERNAL_DIR}/3rdparty/glslang")
set(METALCRAFT_SPIRVCROSS_DIR "${METALCRAFT_EXTERNAL_DIR}/3rdparty/SPIRV-Cross")

set(METALCRAFT_CORE_SOURCES
  src/IndirectCommandStream.cpp
  src/JNIBridge.cpp
  src/RenderDeviceFactory.cpp
  src/RingBuffer.cpp
  src/StateTracker.cpp
)

if(METALCRAFT_ENABLE_SHADER_TRANSLATOR)
  if(NOT EXISTS "${METALCRAFT_GLSLANG_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "Missing glslang dependency. Restore MobileGlues nested submodules first.")
  endif()
  if(NOT EXISTS "${METALCRAFT_SPIRVCROSS_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "Missing SPIRV-Cross dependency. Restore MobileGlues nested submodules first.")
  endif()

  set(BUILD_EXTERNAL OFF CACHE BOOL "" FORCE)
  set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
  set(ENABLE_HLSL OFF CACHE BOOL "" FORCE)
  set(ENABLE_OPT OFF CACHE BOOL "" FORCE)
  set(SKIP_GLSLANG_INSTALL ON CACHE BOOL "" FORCE)

  set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
  set(SPIRV_CROSS_CLI OFF CACHE BOOL "" FORCE)
  set(SPIRV_CROSS_STATIC ON CACHE BOOL "" FORCE)
  set(SPIRV_CROSS_ENABLE_C_API OFF CACHE BOOL "" FORCE)
  set(SPIRV_CROSS_ENABLE_CPP OFF CACHE BOOL "" FORCE)
  set(SPIRV_CROSS_ENABLE_GLSL ON CACHE BOOL "" FORCE)
  set(SPIRV_CROSS_ENABLE_HLSL OFF CACHE BOOL "" FORCE)
  set(SPIRV_CROSS_ENABLE_MSL ON CACHE BOOL "" FORCE)
  set(SPIRV_CROSS_ENABLE_REFLECT OFF CACHE BOOL "" FORCE)
  set(SPIRV_CROSS_ENABLE_UTIL OFF CACHE BOOL "" FORCE)

  list(APPEND METALCRAFT_CORE_SOURCES
    src/ShaderTranslator.cpp
  )

  add_subdirectory("${METALCRAFT_GLSLANG_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/glslang" EXCLUDE_FROM_ALL)
  add_subdirectory("${METALCRAFT_SPIRVCROSS_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/spirv-cross" EXCLUDE_FROM_ALL)
endif()


add_library(MetalCraftCore STATIC ${METALCRAFT_CORE_SOURCES})
target_compile_features(MetalCraftCore PUBLIC cxx_std_17)
target_include_directories(MetalCraftCore
  PUBLIC
    "${METALCRAFT_INCLUDE_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/.."
)
if(METALCRAFT_ENABLE_SHADER_TRANSLATOR)
  target_compile_definitions(MetalCraftCore PUBLIC METALCRAFT_HAS_SHADER_TRANSLATOR=1)
  target_link_libraries(MetalCraftCore
    PUBLIC
      glslang::glslang
      glslang::glslang-default-resource-limits
      glslang::SPIRV
      spirv-cross-msl
  )
endif()

if(APPLE)
  add_library(MetalCraft SHARED
    src/apple/MetalBuffer.mm
    src/apple/MetalCommandEncoder.mm
    src/apple/MetalDepthStencilManager.mm
    src/apple/MetalDevice.mm
    src/apple/MetalPipelineManager.mm
    src/apple/MetalTexture.mm
  )
  target_compile_options(MetalCraft PRIVATE -fobjc-arc)
  target_include_directories(MetalCraft
    PRIVATE
      "${METALCRAFT_INCLUDE_DIR}"
      "${CMAKE_CURRENT_LIST_DIR}/.."
  )
  target_link_libraries(MetalCraft
    PRIVATE
      MetalCraftCore
      "-framework Foundation"
      "-framework Metal"
      "-framework QuartzCore"
  )
  set_target_properties(MetalCraft PROPERTIES
    OUTPUT_NAME "MetalCraft"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
  )
endif()

