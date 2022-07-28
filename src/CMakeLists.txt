# Add source to this project's executable.
add_executable(${CMAKE_PROJECT_NAME}
        main.cpp
        vk/common.h
        vk/init/info.cpp
        vk/init/info.h
        vk/init/pipeline.cpp
        vk/init/pipeline.h
        vk/init/descriptor.cpp
        vk/init/descriptor.h
        vk/mesh.cpp
        vk/mesh.h
        vk/types.h
        vk/check.h
        vk/renderer.cpp
        vk/renderer.h)

set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>")

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(${CMAKE_PROJECT_NAME} tinyobjloader glm vma vk-bootstrap Vulkan::Vulkan sdl2)

add_dependencies(${CMAKE_PROJECT_NAME} Shaders)