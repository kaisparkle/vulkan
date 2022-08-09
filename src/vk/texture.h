#pragma once

#include <unordered_map>
#include <string>
#include <vk/types.h>

namespace VkRenderer {
    struct Texture {
        AllocatedImage image;
        VkImageView imageView;
        VkSampler sampler;
        VkDescriptorSet descriptor;
        std::string type;
    };

    struct PBRTexture {
        VkDescriptorSet descriptor;
        std::vector<Texture*> textureMaps;
    };

    class TextureManager {
    public:
        void init(ResourceHandles *resources);

        Texture *create_texture(const std::string &filePath, const std::string &typeName);

        PBRTexture *create_pbr_texture(std::vector<Texture*> &textureMaps, const std::string &name);

        Texture *get_texture(const std::string &name);

        PBRTexture *get_pbr_texture(const std::string &name);

        Texture *_defaultTexture;

    private:
        ResourceHandles *_resources;
        std::unordered_map<std::string, Texture> _textures;
        std::unordered_map<std::string, PBRTexture> _pbrTextures;
        VkRenderer::descriptor::Allocator *_descriptorAllocator;
    };
}