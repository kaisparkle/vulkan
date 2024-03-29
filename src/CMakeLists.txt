# Add source to this project's executable.
add_executable(${CMAKE_PROJECT_NAME}
        main.cpp
        vk/types.h
        vk/info.cpp
        vk/info.h
        vk/pipeline.cpp
        vk/pipeline.h
        vk/descriptor.cpp
        vk/descriptor.h
        vk/vertex.cpp
        vk/vertex.h
        vk/texture.cpp
        vk/texture.h
        vk/mesh.cpp
        vk/mesh.h
        vk/material.cpp
        vk/material.h
        vk/model.cpp
        vk/model.h
        vk/utils.cpp
        vk/utils.h
        vk/check.h
        vk/renderer.cpp
        vk/renderer.h
        camera.h
        camera.cpp)

set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>")

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(${CMAKE_PROJECT_NAME} stb assimp imgui implot glm vma vk-bootstrap Vulkan::Vulkan sdl2)

add_dependencies(${CMAKE_PROJECT_NAME} Shaders)