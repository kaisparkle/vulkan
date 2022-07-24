# Add source to this project's executable.
add_executable(${CMAKE_PROJECT_NAME}
        main.cpp
        vk/vk_engine.cpp
        vk/vk_engine.h
        vk/vk_initializers.cpp
        vk/vk_initializers.h
        vk/vk_pipeline.cpp
        vk/vk_pipeline.h
        vk/vk_mesh.cpp
        vk/vk_mesh.h
        vk/vk_types.h
        vk/vk_check.h)

set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>")

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(${CMAKE_PROJECT_NAME} tinyobjloader glm vma vk-bootstrap Vulkan::Vulkan sdl2)

add_dependencies(${CMAKE_PROJECT_NAME} Shaders)