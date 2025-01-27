find_package(glfw3 3.4 REQUIRED)
find_package(VulkanMemoryAllocator REQUIRED)
find_package(glm REQUIRED)
find_package(Vulkan REQUIRED)

if (APPLE)
  set(SLANG_VERSION "2025.3.1")
  set(SLANG_OS "macos-aarch64")
  include(FetchContent)
  FetchContent_Populate(slang_zip URL https://github.com/shader-slang/slang/releases/download/v${SLANG_VERSION}/slang-${SLANG_VERSION}-${SLANG_OS}.zip QUIET)
  message(STATUS "Slang dir: ${slang_zip_SOURCE_DIR}")
  add_library(slang SHARED IMPORTED GLOBAL)
  set_target_properties(slang PROPERTIES
        IMPORTED_LOCATION ${slang_zip_SOURCE_DIR}/lib/libslang.dylib
        INTERFACE_INCLUDE_DIRECTORIES ${slang_zip_SOURCE_DIR}/include
    )
  add_library(slang::slang ALIAS slang)
else()
  find_package(slang REQUIRED)
endif()


add_library(engine engine.cpp src/vma.cpp src/stb_image.cpp src/core/window.cpp src/core/device.cpp src/rendering/renderer.cpp src/core/model.cpp src/core/pipeline.cpp)
target_include_directories(engine PUBLIC include . ${Vulkan_INCLUDES} libs/slang/include libs/stb)
target_link_libraries(engine PRIVATE math slang::slang glm::glm Vulkan::Vulkan GPUOpen::VulkanMemoryAllocator glfw)
target_precompile_headers(engine PRIVATE pch.h)
