#pragma once

#include <unordered_map>
#include <string>
#include <vk/types.h>

namespace VkRenderer {
    struct Texture {
        AllocatedImage image;
        VkImageView imageView;
        VkDescriptorSet descriptor;
        std::string type;
    };

    class TextureManager {
    public:
        void init(ResourceHandles *resources);

        Texture *create_texture(const std::string &filePath, const std::string &typeName);

        Texture *get_texture(const std::string &name);

    private:
        ResourceHandles *_resources;
        Texture *_defaultTexture;
        std::unordered_map<std::string, Texture> _textures;
        VkRenderer::descriptor::Allocator *_descriptorAllocator;
    };
}