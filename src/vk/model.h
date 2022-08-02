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
    class Model {
    public:
        void set_model(const std::string &filePath);

        void upload_meshes(VmaAllocator &allocator, DeletionQueue &deletionQueue);

        void set_model_matrix(glm::mat4 newModelMatrix);

        void draw_model(VkCommandBuffer cmd);

        glm::mat4 _modelMatrix = glm::mat4{1.0f};
        std::vector<Mesh> _meshes;
        Material *_defaultMaterial;

    private:

        void process_node(aiNode *node, const aiScene *scene);

        Mesh process_mesh(aiMesh *mesh, const aiScene *scene);
    };

    class ModelManager {
    public:
        std::unordered_map<std::string, Model> models;

        Model *create_model(const std::string &filePath, const std::string &name, Material *defaultMaterial, VmaAllocator &allocator, DeletionQueue &deletionQueue);
    };
}