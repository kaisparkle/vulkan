#include <iostream>
#include <stb_image.h>
#include <vk/utils.h>
#include <vk/info.h>
#include <vk/types.h>

#include "texture.h"

namespace VkRenderer {
    void TextureManager::init(ResourceHandles *resources) {
        _resources = resources;
        _descriptorAllocator = new VkRenderer::descriptor::Allocator{};
        _descriptorAllocator->init(_resources->device);
        _defaultTexture = create_texture("../assets/devtex/dev_grid.png", "texture_diffuse");
    }

    Texture *TextureManager::create_texture(const std::string &filePath, const std::string &typeName) {
        int texWidth, texHeight, texChannels;

        stbi_uc *pixels = stbi_load(filePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        if (!pixels) {
            std::cout << "Failed to load texture " << filePath << ", substituting for default" << std::endl;
            return _defaultTexture;
        }

        // create a staging buffer for texture data
        void *pixelPtr = pixels;
        VkDeviceSize imageSize = texWidth * texHeight * 4;
        VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
        AllocatedBuffer stagingBuffer = VkRenderer::utils::create_buffer(_resources->allocator, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

        // copy
        void *data;
        vmaMapMemory(_resources->allocator, stagingBuffer._allocation, &data);
        memcpy(data, pixelPtr, static_cast<size_t>(imageSize));
        vmaUnmapMemory(_resources->allocator, stagingBuffer._allocation);
        stbi_image_free(pixels);

        // create the vk texture
        Texture newTexture;
        VkExtent3D imageExtent;
        imageExtent.width = static_cast<uint32_t>(texWidth);
        imageExtent.height = static_cast<uint32_t>(texHeight);
        imageExtent.depth = 1;

        // allocate image
        VkImageCreateInfo imageInfo = VkRenderer::info::image_create_info(imageFormat, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);
        VmaAllocationCreateInfo allocInfo = VkRenderer::info::allocation_create_info(VMA_MEMORY_USAGE_GPU_ONLY, 0);
        vmaCreateImage(_resources->allocator, &imageInfo, &allocInfo, &newTexture.image._image, &newTexture.image._allocation, nullptr);

        // transfer from staging to texture
        VkRenderer::utils::immediate_submit(_resources, [=](VkCommandBuffer cmd) {
            VkImageSubresourceRange range;
            range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            range.baseMipLevel = 0;
            range.levelCount = 1;
            range.baseArrayLayer = 0;
            range.layerCount = 1;

            VkImageMemoryBarrier imageBarrierTransfer = {};
            imageBarrierTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarrierTransfer.pNext = nullptr;
            imageBarrierTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageBarrierTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageBarrierTransfer.image = newTexture.image._image;
            imageBarrierTransfer.subresourceRange = range;
            imageBarrierTransfer.srcAccessMask = 0;
            imageBarrierTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrierTransfer);

            VkBufferImageCopy copyRegion = {};
            copyRegion.bufferOffset = 0;
            copyRegion.bufferRowLength = 0;
            copyRegion.bufferImageHeight = 0;
            copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.imageSubresource.mipLevel = 0;
            copyRegion.imageSubresource.baseArrayLayer = 0;
            copyRegion.imageSubresource.layerCount = 1;
            copyRegion.imageExtent = imageExtent;

            vkCmdCopyBufferToImage(cmd, stagingBuffer._buffer, newTexture.image._image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

            VkImageMemoryBarrier imageBarrierReadable = imageBarrierTransfer;
            imageBarrierReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageBarrierReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageBarrierReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageBarrierReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrierReadable);
        });

        _resources->mainDeletionQueue.push_function([=]() {
            vmaDestroyImage(_resources->allocator, newTexture.image._image, newTexture.image._allocation);
        });
        vmaDestroyBuffer(_resources->allocator, stagingBuffer._buffer, stagingBuffer._allocation);

        VkImageViewCreateInfo imageViewInfo = VkRenderer::info::imageview_create_info(VK_FORMAT_R8G8B8A8_SRGB, newTexture.image._image, VK_IMAGE_ASPECT_COLOR_BIT);
        vkCreateImageView(_resources->device, &imageViewInfo, nullptr, &newTexture.imageView);
        _resources->mainDeletionQueue.push_function([=]() {
            vkDestroyImageView(_resources->device, newTexture.imageView, nullptr);
        });

        // create sampler
        VkSamplerCreateInfo samplerInfo = VkRenderer::info::sampler_create_info(VK_FILTER_LINEAR);
        VkSampler sampler;
        vkCreateSampler(_resources->device, &samplerInfo, nullptr, &sampler);

        // write to descriptor set
        VkDescriptorImageInfo descriptorImageInfo = {};
        descriptorImageInfo.sampler = sampler;
        descriptorImageInfo.imageView = newTexture.imageView;
        descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorSet newDescriptor;
        VkRenderer::descriptor::Builder::begin(_resources->descriptorLayoutCache, _descriptorAllocator)
                .bind_image(0, &descriptorImageInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                .build(newDescriptor);

        newTexture.descriptor = newDescriptor;
        newTexture.type = typeName;
        // just use file path as texture name for now
        _textures[filePath] = newTexture;

        std::cout << "Loaded texture " << filePath << std::endl;
        return &_textures[filePath];
    }

    Texture *TextureManager::get_texture(const std::string &name) {
        if (_textures.find(name) == _textures.end()) {
            // does not exist
            return nullptr;
        } else {
            return &_textures[name];
        }
    }
}