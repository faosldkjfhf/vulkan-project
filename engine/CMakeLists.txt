find_package(glfw3 3.4 REQUIRED)
find_package(VulkanMemoryAllocator REQUIRED)
find_package(glm REQUIRED)
find_package(Vulkan REQUIRED)

if (APPLE)
  find_package(fmt REQUIRED)
else()
  add_subdirectory(libs/fmt EXCLUDE_FROM_ALL)
endif()

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


add_library(engine
  engine.cpp
  src/vma.cpp src/stb_image.cpp src/tinyobjloader.cc
  src/core/window.cpp src/core/device.cpp
  src/rendering/renderer.cpp src/core/model.cpp
  src/core/pipeline.cpp src/core/texture.cpp
  src/core/descriptors.cpp
  src/rendering/render_graph.cpp
  libs/imgui/imgui.cpp libs/imgui/imgui_draw.cpp
  libs/imgui/imgui_tables.cpp libs/imgui/imgui_widgets.cpp
  libs/imgui/backends/imgui_impl_vulkan.cpp libs/imgui/backends/imgui_impl_glfw.cpp)


if (APPLE)
  target_include_directories(engine PUBLIC libs/slang/include)
endif()

target_include_directories(engine PUBLIC include . ${Vulkan_INCLUDES} libs/stb libs/tinyobjloader libs/imgui libs/imgui/backends)
target_link_libraries(engine PRIVATE fmt::fmt slang::slang glm::glm Vulkan::Vulkan GPUOpen::VulkanMemoryAllocator glfw)
target_precompile_headers(engine PRIVATE pch.h)
