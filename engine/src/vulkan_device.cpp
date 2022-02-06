#include "vulkan_device.h"
#include <cassert>
#include <iostream>
#include <vulkan_debug.h>

VkBool32 getSupportedDepthFormat(VkPhysicalDevice physicalDevice, VkFormat* depthFormat)
{
    // Since all depth formats may be optional, we need to find a suitable depth format to use
    // Start with the highest precision packed format
    std::vector<VkFormat> depthFormats = {VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D16_UNORM};

    for (auto& format : depthFormats)
    {
        VkFormatProperties formatProps;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);
        // Format must support depth stencil attachment for optimal tiling
        if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            *depthFormat = format;
            return true;
        }
    }

    return false;
}

void VulkanDevice::get_device_properties()
{
    // Store Properties features, limits and properties of the physical device for later use
    // Device properties also contain limits and sparse properties
    // vkGetPhysicalDeviceProperties2(m_physical_device, &m_properties);

    vkGetPhysicalDeviceProperties(m_physical_device, &m_properties);
    vkGetPhysicalDeviceFeatures(m_physical_device, &m_features);
    vkGetPhysicalDeviceMemoryProperties(m_physical_device, &m_memoryProperties);

    // Queue family properties
    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queueFamilyCount, nullptr);
    assert(queueFamilyCount > 0);
    m_queue_family_properties.resize(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physical_device, &queueFamilyCount, m_queue_family_properties.data());

    // Get list of supported extensions
    uint32_t extCount = 0;
    vkEnumerateDeviceExtensionProperties(m_physical_device, nullptr, &extCount, nullptr);
    if (extCount > 0)
    {
        std::vector<VkExtensionProperties> extensions(extCount);
        if (vkEnumerateDeviceExtensionProperties(m_physical_device, nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
        {
            for (auto ext : extensions)
            {
                m_supported_extensions.push_back(ext.extensionName);
            }
        }
    }
}

VulkanDevice::~VulkanDevice()
{
    if (m_vulkan_device)
    {
        vkDestroyDevice(m_vulkan_device, nullptr);
    }
}

void VulkanDevice::create_device_instance()
{
    // VkApplicationInfo, contains the data about the application we intend to create, important to add the VULKAN API
    // version to it in this case, should not be modified, since it is in the extern dir of this project...
    VkApplicationInfo appInfo = {};
    appInfo.sType             = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName  = "OutterSpace";
    appInfo.pEngineName       = "OutterSpace";
    appInfo.apiVersion        = VK_API_VERSION_1_2;

    std::vector<const char*> instance_extensions = {VK_KHR_SURFACE_EXTENSION_NAME};
    // This surface extension is different for each OS...
    // Right now this is targetting only WINDOWS
    instance_extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

    // Get extensions supported by the instance and store them for later use
    uint32_t extensions_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, nullptr);
    if (extensions_count > 0)
    {
        std::vector<VkExtensionProperties> extensions(extensions_count);
        if (vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, &extensions.front()) == VK_SUCCESS)
        {
            for (VkExtensionProperties extension : extensions)
            {
                m_supported_instance_extensions.push_back(extension.extensionName);
            }
        }
    }

    // Add to the list of extensions, the ones requested via m_enabled_instance_extensions
    if (m_requested_extensions.size() > 0)
    {
        for (const char* enabledExtension : m_requested_extensions)
        {
            // Output message if requested extension is not available
            if (std::find(m_requested_extensions.begin(), m_requested_extensions.end(), enabledExtension) == m_requested_extensions.end())
            {
                std::cerr << "Enabled m_instance extension \"" << enabledExtension << "\" is not present at m_instance level\n";
            }
            instance_extensions.push_back(enabledExtension);
        }
    }

    // Information regarding the VK intance we intent to create
    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType                = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pNext                = NULL; // pointer to any structure extending this structure, or NULL in this case... nothing to extent
    instanceCreateInfo.pApplicationInfo     = &appInfo;
    if (instance_extensions.size() > 0)
    {
        if (m_settings.m_enable_validation_layer)
        {
            instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        instanceCreateInfo.enabledExtensionCount   = (uint32_t)instance_extensions.size();
        instanceCreateInfo.ppEnabledExtensionNames = instance_extensions.data();
    }

    // if the validation layer has been requested, it needs to find out if is being properly added
    // by simply checking the list of already requested extensions
    if (m_settings.m_enable_validation_layer)
    {
        const char* validation_layer_name = "VK_LAYER_KHRONOS_validation";
        // get the number of requested extensions
        uint32_t instance_layer_count;
        vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr);
        // the same function will overwrite the vector, with the right values
        std::vector<VkLayerProperties> instance_layer_properties(instance_layer_count);
        vkEnumerateInstanceLayerProperties(&instance_layer_count, instance_layer_properties.data());
        bool validation_layer_present = false;

        for (VkLayerProperties layer : instance_layer_properties)
        {
            if (strcmp(layer.layerName, validation_layer_name) == 0)
            {
                validation_layer_present = true;
                break;
            }
        }
        if (validation_layer_present)
        {
            instanceCreateInfo.ppEnabledLayerNames = &validation_layer_name;
            instanceCreateInfo.enabledLayerCount   = 1;
        }
        else
        {
            std::cerr << "Validation layer VK_LAYER_KHRONOS_validation not present, m_enable_validation_layer is disabled";
        }
    }

    if (vkCreateInstance(&instanceCreateInfo, nullptr, &m_vk_instance) != VK_SUCCESS)
    {
        assert("Vulkan instance could not be created !!");
    };

    // If requested, we enable the default validation layers for debugging
    if (m_settings.m_enable_validation_layer)
    {
        // The report flags determine what type of messages for the layers will be displayed
        // For validating (debugging) an application the error and warning bits should suffice
        VkDebugReportFlagsEXT debugReportFlags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
        // Additional flags include performance info, loader and layer debug messages, etc.
        vks::debug::setupDebugging(m_vk_instance, debugReportFlags, VK_NULL_HANDLE);
    }

    // Physical device
    uint32_t gpu_count = 0;
    // Get number of available physical devices
    vkEnumeratePhysicalDevices(m_vk_instance, &gpu_count, nullptr);

    if (gpu_count == 0)
    {
        assert("No dedicated gpu found !!");
    }
    // Enumerate devices
    std::vector<VkPhysicalDevice> physical_devices(gpu_count);
    vkEnumeratePhysicalDevices(m_vk_instance, &gpu_count, physical_devices.data());

    // Just select the first GPU...
    // TODO: In the future this could be extended to command line, or I guess that might be rebuilt to choose which GPU
    // to choose etc...
    uint32_t selectedDevice = 0;

    m_physical_device = physical_devices[selectedDevice];

    // vkGetPhysicalDeviceProperties(m_physical_device, &m_device_properties);
    vkGetPhysicalDeviceProperties(m_physical_device, &m_device_properties);
    vkGetPhysicalDeviceFeatures(m_physical_device, &m_device_features);
    vkGetPhysicalDeviceMemoryProperties(m_physical_device, &m_device_memory_properties);

    //----------------------------------------------------------------------//
    // TODO: get enabled features in here, need to search what actually is...

    //----------------------------------------------------------------------//

    get_device_properties();
    VkResult res = create_logical_device();
    if (res != VK_SUCCESS)
    {

        assert("Could not create a vk logical device !!");
    }

    // Get all queues from the device
    // vkGetDeviceQueue(m_vulkan_device, m_graphics_queue_id, 0, &m_graphics_queue);
    // vkGetDeviceQueue(m_vulkan_device, m_compute_queue_id, 0, &m_compute_queue);
    // vkGetDeviceQueue(m_vulkan_device, m_transfer_queue_id, 0, &m_transfer_queue);

    VkBool32 validDepthFormat = getSupportedDepthFormat(m_physical_device, &m_depthFormat);

    if (!validDepthFormat)
    {
        // TODO: ASSERT
    }
}

uint32_t VulkanDevice::get_queue_family_index(VkQueueFlagBits queueFlags) const
{
    // Dedicated queue for compute
    // Try to find a queue family index that supports compute but not graphics
    if (queueFlags & VK_QUEUE_COMPUTE_BIT)
    {
        for (uint32_t i = 0; i < static_cast<uint32_t>(m_queue_family_properties.size()); i++)
        {
            if ((m_queue_family_properties[i].queueFlags & queueFlags) && ((m_queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
            {
                return i;
            }
        }
    }

    // Dedicated queue for transfer
    // Try to find a queue family index that supports transfer but not graphics and compute
    if (queueFlags & VK_QUEUE_TRANSFER_BIT)
    {
        for (uint32_t i = 0; i < static_cast<uint32_t>(m_queue_family_properties.size()); i++)
        {
            if ((m_queue_family_properties[i].queueFlags & queueFlags) && ((m_queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) &&
                ((m_queue_family_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
            {
                return i;
            }
        }
    }

    // For other queue types or if no separate compute queue is present, return the first one to support the requested
    // flags
    for (uint32_t i = 0; i < static_cast<uint32_t>(m_queue_family_properties.size()); i++)
    {
        if (m_queue_family_properties[i].queueFlags & queueFlags)
        {
            return i;
        }
    }

    throw std::runtime_error("Could not find a matching m_graphics_queue family index");
}

VkResult VulkanDevice::create_logical_device()
{

    const VkQueueFlags requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
    // Desired queues need to be requested upon logical device creation
    // Due to differing queue family configurations of Vulkan implementations this can be a bit tricky, especially if
    // the application requests different queue types

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos{};

    // Get queue family indices for the requested queue family types
    // Note that the indices may overlap depending on the implementation

    uint32_t m_graphics_queue_id = VK_NULL_HANDLE;
    uint32_t m_compute_queue_id  = VK_NULL_HANDLE;
    uint32_t m_transfer_queue_id = VK_NULL_HANDLE;

    // TODO: this needs to be passed dowm, a priority for each queue... >.>
    float defaultQueuePriority[16];
    for (int i = 0; i < 16; ++i)
    {
        defaultQueuePriority[i] = 0.0f;
    }
    // Graphics queue
    if (requestedQueueTypes & VK_QUEUE_GRAPHICS_BIT)
    {
        m_graphics_queue_id = get_queue_family_index(VK_QUEUE_GRAPHICS_BIT);
        VkDeviceQueueCreateInfo queueInfo{};
        queueInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = m_graphics_queue_id;
        queueInfo.queueCount       = m_queue_family_properties[m_graphics_queue_id].queueCount;
        queueInfo.pQueuePriorities = defaultQueuePriority;
        queue_create_infos.push_back(queueInfo);
    }

    // Dedicated compute queue
    if (requestedQueueTypes & VK_QUEUE_COMPUTE_BIT)
    {
        m_compute_queue_id = get_queue_family_index(VK_QUEUE_COMPUTE_BIT);
        if (m_compute_queue_id != m_graphics_queue_id)
        {
            // If compute family index differs, we need an additional queue create info for the compute queue
            VkDeviceQueueCreateInfo queueInfo{};
            queueInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfo.queueFamilyIndex = m_compute_queue_id;
            queueInfo.queueCount       = m_queue_family_properties[m_compute_queue_id].queueCount;
            queueInfo.pQueuePriorities = defaultQueuePriority;
            queue_create_infos.push_back(queueInfo);
        }
    }

    // Dedicated transfer queue
    if (requestedQueueTypes & VK_QUEUE_TRANSFER_BIT)
    {
        m_transfer_queue_id = get_queue_family_index(VK_QUEUE_TRANSFER_BIT);
        if ((m_transfer_queue_id != m_graphics_queue_id) && (m_transfer_queue_id != m_compute_queue_id))
        {
            // If compute family index differs, we need an additional queue create info for the tranfer queue
            VkDeviceQueueCreateInfo queueInfo{};
            queueInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfo.queueFamilyIndex = m_transfer_queue_id;
            queueInfo.queueCount       = m_queue_family_properties[m_transfer_queue_id].queueCount;
            queueInfo.pQueuePriorities = defaultQueuePriority;
            queue_create_infos.push_back(queueInfo);
        }
    }

    // Create the logical device representation
    std::vector<const char*> device_extensions(m_enabledDeviceExtensions);

    // use swapchain, so, request one via extension
    {
        // If the device will be used for presenting to a display via a swapchain we need to request the swapchain
        // extension
        device_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        device_extensions.push_back(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
        device_extensions.push_back(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
        // device_extensions.push_back(VK_KHR_get_physical_device_properties2);
    }

    VkPhysicalDeviceSynchronization2FeaturesKHR synchronization2_feature = {};
    synchronization2_feature.sType                                       = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;
    synchronization2_feature.synchronization2                            = VK_TRUE;
    synchronization2_feature.pNext;

    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamic_rendering_feature = {};
    dynamic_rendering_feature.sType                                       = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
    dynamic_rendering_feature.dynamicRendering                            = VK_TRUE;
    dynamic_rendering_feature.pNext                                       = &synchronization2_feature;

    VkDeviceCreateInfo deviceCreateInfo   = {};
    deviceCreateInfo.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext                = &dynamic_rendering_feature;
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    ;
    deviceCreateInfo.pQueueCreateInfos = queue_create_infos.data();
    deviceCreateInfo.pEnabledFeatures  = &m_enabled_features;

    // If a pNext(Chain) has been passed, we need to add it to the device creation info
    VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};
    if (m_deviceCreatepNextChain)
    {
        physicalDeviceFeatures2.sType     = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        physicalDeviceFeatures2.features  = m_enabled_features;
        physicalDeviceFeatures2.pNext     = m_deviceCreatepNextChain;
        deviceCreateInfo.pEnabledFeatures = nullptr;
        deviceCreateInfo.pNext            = &physicalDeviceFeatures2;
    }

    // Enable the debug marker extension if it is present (likely meaning a debugging tool is present)
    if (extensionSupported(VK_EXT_DEBUG_MARKER_EXTENSION_NAME))
    {
        device_extensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
        m_settings.m_enableDebugMarkers = true;
    }

    if (device_extensions.size() > 0)
    {
        for (const char* enabledExtension : device_extensions)
        {
            if (!extensionSupported(enabledExtension))
            {
                std::cerr << "Enabled m_device extension \"" << enabledExtension << "\" is not present at m_device level\n";
            }
        }

        deviceCreateInfo.enabledExtensionCount   = (uint32_t)device_extensions.size();
        deviceCreateInfo.ppEnabledExtensionNames = device_extensions.data();
    }

    VkResult result = vkCreateDevice(m_physical_device, &deviceCreateInfo, nullptr, &m_vulkan_device);
    if (result != VK_SUCCESS)
    {
        return result;
    }

    return result;
}

bool VulkanDevice::extensionSupported(std::string extension)
{
    return (std::find(m_supported_extensions.begin(), m_supported_extensions.end(), extension) != m_supported_extensions.end());
}
