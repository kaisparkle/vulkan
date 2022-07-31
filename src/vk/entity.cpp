#include <iostream>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include "entity.h"

namespace VkRenderer {
    void Entity::load_model(const std::string &filePath) {
        Assimp::Importer importer;
        const aiScene *modelScene = importer.ReadFile(filePath, aiProcess_Triangulate | aiProcess_FlipUVs);

        if (!modelScene || modelScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !modelScene->mRootNode) {
            std::cout << "Assimp error: " << importer.GetErrorString() << std::endl;
            return;
        }

        process_node(modelScene->mRootNode, modelScene);
    }

    void Entity::process_node(aiNode *node, const aiScene *scene) {
        for (size_t i = 0; i < node->mNumMeshes; i++) {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            _meshes.push_back(process_mesh(mesh, scene));
        }
        for (size_t i = 0; i < node->mNumChildren; i++) {
            process_node(node->mChildren[i], scene);
        }
    }

    Mesh Entity::process_mesh(aiMesh *mesh, const aiScene *scene) {
        Mesh newMesh;
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

    Entity *EntityManager::create_entity(const std::string &filePath, const std::string &name) {
        Entity newEntity;
        newEntity.load_model(filePath);
        _entities[name] = newEntity;
        return &_entities[name];
    }

    Entity *EntityManager::get_entity(const std::string &name) {
        return &_entities[name];
    }
}