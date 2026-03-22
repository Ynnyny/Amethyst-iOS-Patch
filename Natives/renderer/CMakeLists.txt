cmake_minimum_required(VERSION 3.16)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  project(MetalCraft LANGUAGES CXX)
endif()

enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(METALCRAFT_BUILD_TESTS "Build MetalCraft local tests" ON)

set(METALCRAFT_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/include")

set(METALCRAFT_CORE_SOURCES
  src/JNIBridge.cpp
  src/RenderDeviceFactory.cpp
  src/RingBuffer.cpp
  src/StateTracker.cpp
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  list(APPEND METALCRAFT_CORE_SOURCES
    src/linux/DummyBuffer.cpp
    src/linux/DummyDevice.cpp
    src/linux/DummyTexture.cpp
  )
endif()

add_library(MetalCraftCore STATIC ${METALCRAFT_CORE_SOURCES})
target_compile_features(MetalCraftCore PUBLIC cxx_std_17)
target_include_directories(MetalCraftCore
  PUBLIC
    "${METALCRAFT_INCLUDE_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/.."
)

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
  set_target_properties(MetalCraft PROPERTIES OUTPUT_NAME "MetalCraft")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND METALCRAFT_BUILD_TESTS)
  add_executable(MetalCraftTests tests/MetalCraftTests.cpp)
  target_link_libraries(MetalCraftTests PRIVATE MetalCraftCore)
  add_test(NAME MetalCraftTests COMMAND MetalCraftTests)
endif()
