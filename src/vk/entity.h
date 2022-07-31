#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <glm/glm.hpp>
#include <assimp/scene.h>
#include <vulkan/vulkan.h>
#include <vk/mesh.h>
#include <vk/material.h>

namespace VkRenderer {
    struct Entity {
        std::vector<Mesh> _meshes;
        glm::mat4 transform{1.0f};

        void load_model(const std::string &filePath);

        void process_node(aiNode *node, const aiScene *scene);

        Mesh process_mesh(aiMesh *mesh, const aiScene *scene);
    };

    class EntityManager {
    public:
        Entity *create_entity(const std::string &filePath, const std::string &name);

        Entity *get_entity(const std::string &name);

    private:
        std::unordered_map<std::string, Entity> _entities;
    };
}