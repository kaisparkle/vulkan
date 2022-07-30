#pragma once

#include <unordered_map>
#include <string>
#include <vulkan/vulkan.h>

namespace VkRenderer {
    struct MaterialCreateInfo {
        const char *vertShaderPath;
        const char *fragShaderPath;
        uint32_t setLayoutCount;
        VkDescriptorSetLayout *setLayouts;
        VkDevice device;
        VkExtent2D extent;
        VkRenderPass renderPass;
        const char *name;
    };

    struct Material {
        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;
    };

    class MaterialManager {
    public:
        Material *create_material(MaterialCreateInfo *info);

        Material *get_material(const std::string &name);

        void cleanup(VkDevice device);

    private:
        std::unordered_map<std::string, Material> _materials;

        static bool load_shader_module(VkDevice device, const char *filePath, VkShaderModule *outShaderModule);
    };
}