#pragma once

#include <vk/types.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <glm/glm.hpp>
#include <assimp/scene.h>
#include <vulkan/vulkan.h>
#include <vk/mesh.h>
#include <vk/material.h>
#include <vk/texture.h>

namespace VkRenderer {
    class Model {
    public:
        void set_model(const std::string &filePath, ResourceHandles *resources);

        void upload_meshes(ResourceHandles *resources);

        void set_model_matrix(glm::mat4 newModelMatrix);

        void draw_model(VkCommandBuffer cmd);

        glm::mat4 modelMatrix = glm::mat4{1.0f};
        std::vector<Mesh> meshes;
        Material *defaultMaterial;

    private:
        TextureManager *_textureManager;
        std::string _directory;

        void process_node(aiNode *node, const aiScene *scene);

        Mesh process_mesh(aiMesh *mesh, const aiScene *scene);
    };

    class ModelManager {
    public:
        std::unordered_map<std::string, Model> models;

        void init(ResourceHandles *resources);

        Model *create_model(const std::string &filePath, const std::string &name, Material *defaultMaterial);

    private:
        ResourceHandles *_resources;
    };
}