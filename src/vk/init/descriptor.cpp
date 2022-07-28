#include <algorithm>
#include <vk/common.h>
#include <vk/init/info.h>

#include "descriptor.h"

namespace vkinit::descriptor {
    VkDescriptorPool Allocator::create_pool(int count, VkDescriptorPoolCreateFlags flags) {
        // create pool size array
        std::vector<VkDescriptorPoolSize> sizes;
        sizes.reserve(descriptorSizes.sizes.size());
        for (auto sz: descriptorSizes.sizes) {
            sizes.push_back({sz.first, uint32_t(sz.second * count)});
        }

        // create the pool itself
        VkDescriptorPoolCreateInfo poolInfo = vkinit::info::descriptor_pool_create_info(count, (uint32_t) sizes.size(), sizes.data(), flags);
        VkDescriptorPool descriptorPool;
        vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);

        return descriptorPool;
    }

    VkDescriptorPool Allocator::grab_pool() {
        // check if there are reusable pools available
        if (!freePools.empty()) {
            // pop a pool off the back
            VkDescriptorPool pool = freePools.back();
            freePools.pop_back();
            return pool;
        } else {
            // create a new one
            return Allocator::create_pool(1000, 0);
        }
    }

    void Allocator::init(VkDevice newDevice) {
        device = newDevice;
    }

    bool Allocator::allocate(VkDescriptorSet *set, VkDescriptorSetLayout layout) {
        // initialize the currentPool handle if needed
        if (currentPool == VK_NULL_HANDLE) {
            currentPool = grab_pool();
            usedPools.push_back(currentPool);
        }

        // attempt to allocate set
        VkDescriptorSetAllocateInfo allocInfo = vkinit::info::descriptor_set_allocate_info(currentPool, 1, &layout);
        VkResult allocResult = vkAllocateDescriptorSets(device, &allocInfo, set);

        // check result
        bool needReallocate = false;
        switch (allocResult) {
            case VK_SUCCESS:
                return true;
            case VK_ERROR_FRAGMENTED_POOL:
            case VK_ERROR_OUT_OF_POOL_MEMORY:
                needReallocate = true;
                break;
            default:
                return false;
        }

        if (needReallocate) {
            // allocate a new pool and try again
            currentPool = grab_pool();
            usedPools.push_back(currentPool);
            allocInfo.descriptorPool = currentPool;
            allocResult = vkAllocateDescriptorSets(device, &allocInfo, set);

            // if this fails something is very wrong
            if (allocResult == VK_SUCCESS) return true;
        }

        return false;
    }

    void Allocator::reset_pools() {
        // reset all used pools and move to free pools
        for (auto usedPool: usedPools) {
            vkResetDescriptorPool(device, usedPool, 0);
            freePools.push_back(usedPool);
        }

        usedPools.clear();
        currentPool = VK_NULL_HANDLE;
    }

    void Allocator::cleanup() {
        // destroy all pools
        for (auto freePool: freePools) {
            vkDestroyDescriptorPool(device, freePool, nullptr);
        }
        for (auto usedPool: usedPools) {
            vkDestroyDescriptorPool(device, usedPool, nullptr);
        }
    }

    bool LayoutCache::LayoutInfo::operator==(const LayoutInfo &other) const {
        // compare each field of the bindings
        if (other.bindings.size() != bindings.size()) {
            return false;
        } else {
            for (size_t i = 0; i < bindings.size(); i++) {
                if (other.bindings[i].binding != bindings[i].binding) return false;
                if (other.bindings[i].descriptorType != bindings[i].descriptorType) return false;
                if (other.bindings[i].descriptorCount != bindings[i].descriptorCount) return false;
                if (other.bindings[i].stageFlags != bindings[i].stageFlags) return false;
            }
        }

        // all fields match
        return true;
    }

    size_t LayoutCache::LayoutInfo::hash() const {
        size_t result = std::hash<size_t>()(bindings.size());

        for (const VkDescriptorSetLayoutBinding &binding: bindings) {
            // pack binding data
            size_t bindingHash = binding.binding | binding.descriptorType << 8 | binding.descriptorCount << 16 | binding.stageFlags << 24;

            // shuffle packed data and xor with main hash
            result ^= std::hash<size_t>()(bindingHash);
        }

        return result;
    }

    void LayoutCache::init(VkDevice newDevice) {
        device = newDevice;
    }

    VkDescriptorSetLayout LayoutCache::create_descriptor_layout(VkDescriptorSetLayoutCreateInfo *info) {
        LayoutInfo layoutInfo;
        layoutInfo.bindings.reserve(info->bindingCount);
        bool isSorted = true;
        int lastBinding = -1;

        // copy from direct info struct into ours
        for (size_t i = 0; i < info->bindingCount; i++) {
            layoutInfo.bindings.push_back(info->pBindings[i]);

            // check that the bindings are in increasing order
            if (info->pBindings[i].binding > lastBinding) {
                lastBinding = info->pBindings[i].binding;
            } else {
                isSorted = false;
            }
        }

        // sort bindings if needed
        if (!isSorted) {
            std::sort(layoutInfo.bindings.begin(), layoutInfo.bindings.end(), [](VkDescriptorSetLayoutBinding &a, VkDescriptorSetLayoutBinding &b) {
                return a.binding < b.binding;
            });
        }

        // try grab from cache
        auto it = layoutCache.find(layoutInfo);
        if (it != layoutCache.end()) {
            return (*it).second;
        } else {
            // not found, create a new one
            VkDescriptorSetLayout layout;
            vkCreateDescriptorSetLayout(device, info, nullptr, &layout);

            // add to cache
            layoutCache[layoutInfo] = layout;
            return layout;
        }
    }

    void LayoutCache::cleanup() {
        // destroy all layouts
        for (const auto &pair: layoutCache) {
            vkDestroyDescriptorSetLayout(device, pair.second, nullptr);
        }
    }

    Builder Builder::begin(LayoutCache *layoutCache, Allocator *allocator) {
        Builder builder;
        builder.cache = layoutCache;
        builder.alloc = allocator;

        return builder;
    }

    Builder &Builder::bind_buffer(uint32_t binding, VkDescriptorBufferInfo *bufferInfo, VkDescriptorType type, VkShaderStageFlags stageFlags) {
        // create binding
        VkDescriptorSetLayoutBinding newBinding = descriptor_set_layout_binding(type, stageFlags, binding);
        bindings.push_back(newBinding);

        // create write - temporary dstSet, will be set by build function
        VkWriteDescriptorSet newWrite = write_descriptor_buffer(type, nullptr, bufferInfo, binding);
        writes.push_back(newWrite);

        return *this;
    }

    Builder &Builder::bind_image(uint32_t binding, VkDescriptorImageInfo *imageInfo, VkDescriptorType type, VkShaderStageFlags stageFlags) {
        // create binding
        VkDescriptorSetLayoutBinding newBinding = descriptor_set_layout_binding(type, stageFlags, binding);
        bindings.push_back(newBinding);

        // create write - temporary dstSet, will be set by build function
        VkWriteDescriptorSet newWrite = write_descriptor_image(type, nullptr, imageInfo, binding);
        writes.push_back(newWrite);

        return *this;
    }

    bool Builder::build(VkDescriptorSet &set, VkDescriptorSetLayout &layout) {
        // build layout first
        VkDescriptorSetLayoutCreateInfo layoutInfo = vkinit::info::descriptor_set_layout_create_info(bindings.size(), bindings.data());
        layout = cache->create_descriptor_layout(&layoutInfo);

        // allocate descriptor
        bool success = alloc->allocate(&set, layout);
        if (!success) {
            return false;
        }

        // write descriptor
        for (VkWriteDescriptorSet &write: writes) {
            // set the proper dstSet value
            write.dstSet = set;
        }

        vkUpdateDescriptorSets(alloc->device, writes.size(), writes.data(), 0, nullptr);

        return true;
    }

    bool Builder::build(VkDescriptorSet &set) {
        VkDescriptorSetLayout layout;
        return build(set, layout);
    }

    VkDescriptorSetLayoutBinding descriptor_set_layout_binding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding) {
        VkDescriptorSetLayoutBinding setBinding = {};
        setBinding.binding = binding;
        setBinding.descriptorCount = 1;
        setBinding.descriptorType = type;
        setBinding.pImmutableSamplers = nullptr;
        setBinding.stageFlags = stageFlags;

        return setBinding;
    }

    VkWriteDescriptorSet write_descriptor_buffer(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorBufferInfo *bufferInfo, uint32_t binding) {
        VkWriteDescriptorSet write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = nullptr;
        write.dstBinding = binding;
        write.dstSet = dstSet;
        write.descriptorCount = 1;
        write.descriptorType = type;
        write.pBufferInfo = bufferInfo;

        return write;
    }

    VkWriteDescriptorSet write_descriptor_image(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorImageInfo *imageInfo, uint32_t binding) {
        VkWriteDescriptorSet write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = nullptr;
        write.dstBinding = binding;
        write.dstSet = dstSet;
        write.descriptorCount = 1;
        write.descriptorType = type;
        write.pImageInfo = imageInfo;

        return write;
    }
}