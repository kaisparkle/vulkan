#include <iostream>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <vk/check.h>

#include "model.h"

namespace VkRenderer {
    void Model::set_model(const std::string &filePath) {
        Assimp::Importer importer;
        const aiScene *modelScene = importer.ReadFile(filePath, aiProcess_Triangulate | aiProcess_FlipUVs);

        if (!modelScene || modelScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !modelScene->mRootNode) {
            std::cout << "Assimp error: " << importer.GetErrorString() << std::endl;
            return;
        }

        process_node(modelScene->mRootNode, modelScene);
    }

    void Model::set_model_matrix(glm::mat4 newModelMatrix) {
        _modelMatrix = newModelMatrix;
    }

    void Model::draw_model(VkCommandBuffer cmd) {
        for (auto &mesh: _meshes) {
            mesh.draw_mesh(cmd, _modelMatrix);
        }
    }

    void Model::process_node(aiNode *node, const aiScene *scene) {
        for (size_t i = 0; i < node->mNumMeshes; i++) {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            _meshes.push_back(process_mesh(mesh, scene));
        }
        for (size_t i = 0; i < node->mNumChildren; i++) {
            process_node(node->mChildren[i], scene);
        }
    }

    Mesh Model::process_mesh(aiMesh *mesh, const aiScene *scene) {
        Mesh newMesh;
        newMesh._material = _defaultMaterial;
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
            newMesh._vertices.push_back(newVertex);
        }
        for (size_t i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (size_t j = 0; j < face.mNumIndices; j++) {
                newMesh._indices.push_back(face.mIndices[j]);
            }
        }

        return newMesh;
    }

    void Model::upload_meshes(VmaAllocator &allocator, DeletionQueue &deletionQueue) {
        for (auto &mesh: _meshes) {
            mesh.upload_mesh(allocator, deletionQueue);
        }
    }

    Model *ModelManager::create_model(const std::string &filePath, const std::string &name, Material *defaultMaterial, VmaAllocator &allocator, DeletionQueue &deletionQueue) {
        Model newModel;
        newModel._defaultMaterial = defaultMaterial;
        newModel.set_model(filePath);
        newModel.upload_meshes(allocator, deletionQueue);
        models[name] = newModel;
        return &models[name];
    }
}