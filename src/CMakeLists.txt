# Add source to this project's executable.
add_executable(${CMAKE_PROJECT_NAME}
        main.cpp
        vk/info.cpp
        vk/info.h
        vk/pipeline.cpp
        vk/pipeline.h
        vk/descriptor.cpp
        vk/descriptor.h
        vk/vertex.cpp
        vk/vertex.h
        vk/mesh.cpp
        vk/mesh.h
        vk/material.cpp
        vk/material.h
        vk/object.cpp
        vk/object.h
        vk/types.h
        vk/check.h
        vk/renderer.cpp
        vk/renderer.h)

set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>")

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(${CMAKE_PROJECT_NAME} imgui tinyobjloader glm vma vk-bootstrap Vulkan::Vulkan sdl2)

add_dependencies(${CMAKE_PROJECT_NAME} Shaders)