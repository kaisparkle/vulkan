find_package(Vulkan REQUIRED)

add_library(vk-bootstrap STATIC)
target_sources(vk-bootstrap PRIVATE
        vk-bootstrap/src/VkBootstrapDispatch.h
        vk-bootstrap/src/VkBootstrap.h
        vk-bootstrap/src/VkBootstrap.cpp
        )
target_include_directories(vk-bootstrap PUBLIC vk-bootstrap/src)
target_link_libraries(vk-bootstrap PUBLIC Vulkan::Vulkan $<$<BOOL:UNIX>:${CMAKE_DL_LIBS}>)

add_library(vma INTERFACE)
target_include_directories(vma INTERFACE VulkanMemoryAllocator/include)

add_library(glm INTERFACE)
target_include_directories(glm INTERFACE glm)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE stb)

add_library(imgui STATIC)
target_sources(imgui PRIVATE
        imgui/imgui.cpp
        imgui/imgui_demo.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp
        imgui/backends/imgui_impl_vulkan.cpp
        imgui/backends/imgui_impl_sdl.cpp
        )
target_include_directories(imgui PUBLIC imgui PUBLIC imgui/backends)
target_link_libraries(imgui PUBLIC Vulkan::Vulkan sdl2)

add_library(implot STATIC)
target_sources(implot PRIVATE
        implot/implot.cpp
        implot/implot_demo.cpp
        implot/implot_items.cpp)
target_include_directories(implot PUBLIC implot)
target_link_libraries(implot PUBLIC imgui)

add_subdirectory(assimp)

if(NOT DEFINED ENV{SDL2_PATH})
    message(FATAL_ERROR "Please set the SDL2_PATH environment variable.")
endif()

add_library(sdl2 INTERFACE)
set(sdl2_DIR $ENV{SDL2_PATH} CACHE PATH "Path to SDL2")
if(WIN32)
    target_include_directories(sdl2 INTERFACE ${sdl2_DIR}/include ${sdl2_image_DIR}/include)
    target_link_directories(sdl2 INTERFACE ${sdl2_DIR}/lib/x64 ${sdl2_image_DIR}/lib/x64)
elseif(UNIX)
    target_include_directories(sdl2 INTERFACE ${sdl2_DIR}/include/SDL2 ${sdl2_image_DIR}/include/SDL2)
    target_link_directories(sdl2 INTERFACE ${sdl2_DIR}/lib ${sdl2_image_DIR}/lib)
endif()
target_link_libraries(sdl2 INTERFACE SDL2 SDL2main)