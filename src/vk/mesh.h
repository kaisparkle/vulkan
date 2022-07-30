#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <glm/vec3.hpp>
#include <vulkan/vulkan.h>
#include <vk/vertex.h>
#include <vk/types.h>

namespace VkRenderer {
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