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

    void Model::update_transform() {
        glm::mat4 newTransform = glm::mat4{1.0f};

        // translate
        newTransform = glm::translate(newTransform, glm::vec3(translation[0], translation[1], translation[2]));

        // rotate by each XYZ value
        newTransform = glm::rotate(newTransform, glm::radians(rotation[0]), glm::vec3(1.0f, 0.0f, 0.0f));
        newTransform = glm::rotate(newTransform, glm::radians(rotation[1]), glm::vec3(0.0f, 1.0f, 0.0f));
        newTransform = glm::rotate(newTransform, glm::radians(rotation[2]), glm::vec3(0.0f, 0.0f, 1.0f));

        // scale
        newTransform = glm::scale(newTransform, glm::vec3(scale[0], scale[1], scale[2]));

        _modelMatrix = newTransform;
    }

    void Model::draw_model(VkCommandBuffer cmd) {
        for (auto &mesh: meshes) {
            mesh.draw_mesh(cmd, _modelMatrix);
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
            if (mesh->HasTangentsAndBitangents()) {
                newVertex.tangent.x = mesh->mTangents[i].x;
                newVertex.tangent.y = mesh->mTangents[i].y;
                newVertex.tangent.z = mesh->mTangents[i].z;
                newVertex.bitangent.x = mesh->mBitangents[i].x;
                newVertex.bitangent.y = mesh->mBitangents[i].y;
                newVertex.bitangent.z = mesh->mBitangents[i].z;
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
        std::vector<Texture*> textureMaps;
        textureMaps.push_back(create_texture(material, aiTextureType_BASE_COLOR, "texture_base"));
        textureMaps.push_back(create_texture(material, aiTextureType_NORMALS, "texture_normal"));
        textureMaps.push_back(create_texture(material, aiTextureType_DIFFUSE_ROUGHNESS, "texture_roughness"));
        textureMaps.push_back(create_texture(material, aiTextureType_METALNESS, "texture_metalness"));
        textureMaps.push_back(create_texture(material, aiTextureType_AMBIENT_OCCLUSION, "texture_ao"));

        // just use the base color path as name
        aiString str;
        material->GetTexture(aiTextureType_DIFFUSE, 0, &str);
        std::string fullPath = _directory + '/' + str.C_Str();
        PBRTexture *existingTexture = _textureManager->get_pbr_texture(fullPath);
        if(existingTexture) {
            newMesh._pbrTexture = existingTexture;
        } else {
            newMesh._pbrTexture = _textureManager->create_pbr_texture(textureMaps, fullPath);
        }

        newMesh._texture = textureMaps[0];

        return newMesh;
    }

    Texture *Model::create_texture(aiMaterial *material, aiTextureType textureType, const std::string &typeName) {
        if (!material->GetTextureCount(textureType)) {
            // no textures of this type
            return _textureManager->_defaultTexture;
        }

        aiString str;
        material->GetTexture(textureType, 0, &str);
        std::string fullPath = _directory + '/' + str.C_Str();
        // check if it already exists
        Texture *existingTexture = _textureManager->get_texture(fullPath);
        if(existingTexture) {
            return existingTexture;
        } else {
            // create new texture
            return _textureManager->create_texture(fullPath, typeName);
        }
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