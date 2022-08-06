#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

namespace VkRenderer {
    struct VertexInputDescription {
        std::vector<VkVertexInputBindingDescription> bindings;
        std::vector<VkVertexInputAttributeDescription> attributes;
        VkPipelineVertexInputStateCreateFlags flags = 0;
    };

    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 color;
        glm::vec2 uv;

        static VertexInputDescription get_vertex_description();
    };
}