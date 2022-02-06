#pragma once

#include <VulkanSDK/1.2.198.1/Include/vulkan/vulkan.h>
#include <string>
#include <vector>

struct VulkanDevice
{
    struct Settings
    {
        bool m_enable_validation_layer = true;
        bool m_enableDebugMarkers      = false;
        bool m_fullscreen              = false;
        bool m_enable_vsync            = false;
        bool enable_imgui              = false;
    };
    // A simple collection of settings to set on device init time
    Settings m_settings;
    // Store all the extensions that are supported by the device, so later one can be used when querying
    std::vector<std::string> m_supported_instance_extensions;
    // Extensions requested by the user, on device creation
    std::vector<const char*> m_requested_extensions;

    // Collection of properties of this physical device, GPU name, API/Drivers...
    VkPhysicalDeviceProperties m_device_properties;
    // Collection of features that are requested to be enable when creating the device
    VkPhysicalDeviceFeatures m_enabled_features{};
    // Collection of features supported by the selected physical device
    VkPhysicalDeviceFeatures m_device_features;
    // Available memory types of the selected physical device
    VkPhysicalDeviceMemoryProperties m_device_memory_properties;
    // Extensions requested by the user, on logical device creation
    std::vector<const char*> m_enabledDeviceExtensions;

    void* m_deviceCreatepNextChain = nullptr;

    // Currently used default queues
    // VkQueue		m_graphics_queue;
    // VkQueue		m_compute_queue;
    // VkQueue		m_transfer_queue;

    // physical device supported depth format
    VkFormat m_depthFormat;

    VkInstance                 m_vk_instance;
    VkPhysicalDevice           m_physical_device;
    VkDevice                   m_vulkan_device;
    VkPhysicalDeviceProperties m_properties;
    // VkPhysicalDeviceProperties2 m_properties;

    // PFN_vkGetPhysicalDeviceProperties2KHR _vkGetPhysicalDeviceProperties2;

    VkPhysicalDeviceFeatures             m_features;
    VkPhysicalDeviceMemoryProperties     m_memoryProperties;
    std::vector<VkQueueFamilyProperties> m_queue_family_properties;
    std::vector<std::string>             m_supported_extensions;

    operator VkDevice() const
    {
        return m_vulkan_device;
    };

    VulkanDevice(){};
    ~VulkanDevice();

    void     create_device_instance();
    void     get_device_properties();
    uint32_t get_queue_family_index(VkQueueFlagBits queueFlags) const;
    VkResult create_logical_device();
    bool     extensionSupported(std::string extension);
};
