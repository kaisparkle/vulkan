#include <iostream>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <vk/check.h>
#include <vk/utils.h>
#include <vk/info.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "model.h"

namespace VkRenderer {
    void Model::set_model(const std::string &filePath, ResourceHandles *resources) {
        _textureManager = new TextureManager;
        _textureManager->init(resources);

        Assimp::Importer importer;
        const aiScene *modelScene = importer.ReadFile(filePath, aiProcess_Triangulate | aiProcess_FlipUVs);

        if (!modelScene || modelScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !modelScene->mRootNode) {
            std::cout << "Assimp error: " << importer.GetErrorString() << std::endl;
            return;
        }

        _directory = filePath.substr(0, filePath.find_last_of('/'));
        process_node(modelScene->mRootNode, modelScene);
    }

    void Model::set_model_matrix(glm::mat4 newModelMatrix) {
        modelMatrix = newModelMatrix;
    }

    void Model::draw_model(VkCommandBuffer cmd) {
        for (auto &mesh: meshes) {
            mesh.draw_mesh(cmd, modelMatrix);
        }
    }

    void Model::process_node(aiNode *node, const aiScene *scene) {
        for (size_t i = 0; i < node->mNumMeshes; i++) {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(process_mesh(mesh, scene));
        }
        for (size_t i = 0; i < node->mNumChildren; i++) {
            process_node(node->mChildren[i], scene);
        }
    }

    Mesh Model::process_mesh(aiMesh *mesh, const aiScene *scene) {
        Mesh newMesh;
        newMesh._material = defaultMaterial;
        for (size_t i = 0; i < mesh->mNumVertices; i++) {
            Vertex newVertex;
            newVertex.position.x = mesh->mVertices[i].x;
            newVertex.position.y = mesh->mVertices[i].y;
            newVertex.position.z = mesh->mVertices[i].z;
            if (mesh->HasNormals()) {
                newVertex.normal.x = mesh->mNormals[i].x;
                newVertex.normal.y = mesh->mNormals[i].y;
                newVertex.normal.z = mesh->mNormals[i].z;
                newVertex.color.x = mesh->mNormals[i].x;
                newVertex.color.y = mesh->mNormals[i].y;
                newVertex.color.z = mesh->mNormals[i].z;
            } else {
                newVertex.color = glm::vec3(1.0f, 1.0f, 1.0f);
            }
            if (mesh->mTextureCoords[0]) {
                newVertex.uv.x = mesh->mTextureCoords[0][i].x;
                newVertex.uv.y = mesh->mTextureCoords[0][i].y;
            }
            newMesh._vertices.push_back(newVertex);
        }
        for (size_t i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (size_t j = 0; j < face.mNumIndices; j++) {
                newMesh._indices.push_back(face.mIndices[j]);
            }
        }

        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
        for (size_t i = 0; i < material->GetTextureCount(aiTextureType_DIFFUSE); i++) {
            aiString str;
            material->GetTexture(aiTextureType_DIFFUSE, i, &str);
            std::string fullPath = _directory + '/' + str.C_Str();

            // check if the texture already exists
            Texture *existingTexture = _textureManager->get_texture(fullPath);
            if (existingTexture) {
                // use existing texture
                newMesh._texture = existingTexture;
            } else {
                // create new texture
                newMesh._texture = _textureManager->create_texture(fullPath, "texture_diffuse");
            }
        }

        return newMesh;
    }

    void Model::upload_meshes(ResourceHandles *resources) {
        for (auto &mesh: meshes) {
            mesh.upload_mesh(resources);
        }
    }

    void ModelManager::init(ResourceHandles *resources) {
        _resources = resources;
    }

    Model *ModelManager::create_model(const std::string &filePath, const std::string &name, Material *defaultMaterial) {
        Model newModel;
        newModel.defaultMaterial = defaultMaterial;
        newModel.set_model(filePath, _resources);
        newModel.upload_meshes(_resources);
        models[name] = newModel;

        return &models[name];
    }
}