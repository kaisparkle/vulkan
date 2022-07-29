#pragma once

#include <unordered_map>
#include <vector>
#include <vk/common.h>

namespace VkRenderer::descriptor {
    class Allocator {
    public:
        struct PoolSizes {
            std::vector<std::pair<VkDescriptorType, float>> sizes = {
                    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.0f},
                    {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          4.0f},
                    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         2.0f},
                    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         2.0f},
                    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.0f},
                    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.0f},
                    {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   1.0f},
                    {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   1.0f},
                    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1.0f},
                    {VK_DESCRIPTOR_TYPE_SAMPLER,                0.5f},
                    {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       0.5f}
            };
        };

        VkDevice device;

        void init(VkDevice newDevice);

        bool allocate(VkDescriptorSet *set, VkDescriptorSetLayout layout);

        void reset_pools();

        void cleanup();

    private:
        PoolSizes descriptorSizes;
        std::vector<VkDescriptorPool> usedPools;
        std::vector<VkDescriptorPool> freePools;
        VkDescriptorPool currentPool{VK_NULL_HANDLE};

        VkDescriptorPool create_pool(int count, VkDescriptorPoolCreateFlags flags);

        VkDescriptorPool grab_pool();
    };

    class LayoutCache {
    public:
        struct LayoutInfo {
            std::vector<VkDescriptorSetLayoutBinding> bindings;

            bool operator==(const LayoutInfo &other) const;

            [[nodiscard]] size_t hash() const;
        };

        void init(VkDevice newDevice);

        VkDescriptorSetLayout create_descriptor_layout(VkDescriptorSetLayoutCreateInfo *info);

        void cleanup();

    private:
        struct LayoutHash {
            size_t operator()(const LayoutInfo &k) const {
                return k.hash();
            }
        };

        std::unordered_map<LayoutInfo, VkDescriptorSetLayout, LayoutHash> layoutCache;
        VkDevice device;
    };

    class Builder {
    public:
        static Builder begin(LayoutCache *layoutCache, Allocator *allocator);

        Builder &bind_buffer(uint32_t binding, VkDescriptorBufferInfo *bufferInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);

        Builder &bind_image(uint32_t binding, VkDescriptorImageInfo *imageInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);

        bool build(VkDescriptorSet &set, VkDescriptorSetLayout &layout);

        bool build(VkDescriptorSet &set);

    private:
        std::vector<VkWriteDescriptorSet> writes;
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        LayoutCache *cache;
        Allocator *alloc;
    };

    VkDescriptorSetLayoutBinding descriptor_set_layout_binding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding);

    VkWriteDescriptorSet write_descriptor_buffer(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorBufferInfo *bufferInfo, uint32_t binding);

    VkWriteDescriptorSet write_descriptor_image(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorImageInfo *imageInfo, uint32_t binding);
}
