#pragma once

#include <unordered_map>
#include <vector>
#include <glm/vec3.hpp>
#include <vk/common.h>
#include <vk/types.h>

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

        static VertexInputDescription get_vertex_description();
    };

    struct Mesh {
        std::vector<Vertex> _vertices;
        AllocatedBuffer _vertexBuffer;

        bool load_from_obj(const char *filename);
    };

    class MeshManager {
    public:
        Mesh *create_mesh(const char *filename, const std::string &name);

        Mesh *get_mesh(const std::string &name);

    private:
        std::unordered_map<std::string, Mesh> _meshes;
    };
}