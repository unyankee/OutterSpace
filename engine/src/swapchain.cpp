#include "swapchain.h"
#include "engine.h"

Swapchain::~Swapchain()
{
    if (m_swap_chain != VK_NULL_HANDLE)
    {
        for (uint32_t i = 0; i < m_swapchain_image_count; i++)
        {
            m_colour_attachments[i]->clean_view(m_engine.m_device.m_vulkan_device);
        }
    }
    if (m_surface != VK_NULL_HANDLE)
    {
        fpDestroySwapchainKHR(m_engine.m_device.m_vulkan_device, m_swap_chain, nullptr);
        vkDestroySurfaceKHR(m_engine.m_device.m_vk_instance, m_surface, nullptr);
    }
    m_surface    = VK_NULL_HANDLE;
    m_swap_chain = VK_NULL_HANDLE;
}

void Swapchain::init(const VkInstance& vk_instance, const VkDevice& vk_device)
{
    // get all needed PFN and init the surface
    fpGetPhysicalDeviceSurfaceSupportKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(vkGetInstanceProcAddr(vk_instance, "vkGetPhysicalDeviceSurfaceSupportKHR"));
    fpGetPhysicalDeviceSurfaceCapabilitiesKHR =
        reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(vkGetInstanceProcAddr(vk_instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));
    fpGetPhysicalDeviceSurfaceFormatsKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>(vkGetInstanceProcAddr(vk_instance, "vkGetPhysicalDeviceSurfaceFormatsKHR"));
    fpGetPhysicalDeviceSurfacePresentModesKHR =
        reinterpret_cast<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>(vkGetInstanceProcAddr(vk_instance, "vkGetPhysicalDeviceSurfacePresentModesKHR"));

    fpCreateSwapchainKHR    = reinterpret_cast<PFN_vkCreateSwapchainKHR>(vkGetDeviceProcAddr(vk_device, "vkCreateSwapchainKHR"));
    fpDestroySwapchainKHR   = reinterpret_cast<PFN_vkDestroySwapchainKHR>(vkGetDeviceProcAddr(vk_device, "vkDestroySwapchainKHR"));
    fpGetSwapchainImagesKHR = reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(vkGetDeviceProcAddr(vk_device, "vkGetSwapchainImagesKHR"));
    fpAcquireNextImageKHR   = reinterpret_cast<PFN_vkAcquireNextImageKHR>(vkGetDeviceProcAddr(vk_device, "vkAcquireNextImageKHR"));
    fpQueuePresentKHR       = reinterpret_cast<PFN_vkQueuePresentKHR>(vkGetDeviceProcAddr(vk_device, "vkQueuePresentKHR"));
    //----------------------------------------//

    // Windows-only surface at the moment
    VkResult                    err               = VK_SUCCESS;
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
    surfaceCreateInfo.sType                       = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.hinstance                   = (HINSTANCE)m_engine.HINSTANCE_windowInstance;
    surfaceCreateInfo.hwnd                        = (HWND)m_engine.m_HWND_window;
    err                                           = vkCreateWin32SurfaceKHR(vk_instance, &surfaceCreateInfo, nullptr, &m_surface);
    assert(err == VK_SUCCESS);

    // Need to find which of the available queues support presenting
    const std::vector<VkQueueFamilyProperties>& queueProps = m_engine.m_device.m_queue_family_properties;
    const uint32_t                              queueCount = queueProps.size();

    std::vector<VkBool32> supportsPresent(queueCount);
    for (uint32_t i = 0; i < queueCount; i++)
    {
        err = fpGetPhysicalDeviceSurfaceSupportKHR(m_engine.m_device.m_physical_device, i, m_surface, &supportsPresent[i]);
    }

    uint32_t formatCount;
    // the following line is spamming the console with some windows rt related issues, is already known my microsoft,
    // for some reason, under some SDK, this codepath is going through UNIVERSAL WINDOWS APPLICATION, that is what is
    // spamming the console is not producing any issue, apart from polluting the console if used... >.>
    err = fpGetPhysicalDeviceSurfaceFormatsKHR(m_engine.m_device.m_physical_device, m_surface, &formatCount, NULL);
    assert(formatCount > 0);

    std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
    err = fpGetPhysicalDeviceSurfaceFormatsKHR(m_engine.m_device.m_physical_device, m_surface, &formatCount, surfaceFormats.data());

    // If the surface format list only includes one entry with VK_FORMAT_UNDEFINED,
    // there is no preferred format, so we assume VK_FORMAT_B8G8R8A8_UNORM
    if ((formatCount == 1) && (surfaceFormats[0].format == VK_FORMAT_UNDEFINED))
    {
        m_color_format = VK_FORMAT_B8G8R8A8_UNORM;
        m_color_space  = surfaceFormats[0].colorSpace;
    }
    else
    {
        bool found_B8G8R8A8_UNORM = false;
        for (auto&& surfaceFormat : surfaceFormats)
        {
            if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM)
            {
                m_color_format       = surfaceFormat.format;
                m_color_space        = surfaceFormat.colorSpace;
                found_B8G8R8A8_UNORM = true;
                break;
            }
        }
        // if is not avaialble, fallback to the first compatible colour format
        if (!found_B8G8R8A8_UNORM)
        {
            m_color_format = surfaceFormats[0].format;
            m_color_space  = surfaceFormats[0].colorSpace;
        }
    }
}

void Swapchain::create_swapchain(bool vsync)
{
    VkSwapchainKHR previous_swapchain = m_swap_chain;
    auto&&         physical_device    = m_engine.m_device.m_physical_device;
    auto&&         vk_device          = m_engine.m_device.m_vulkan_device;

    VkResult result = fpGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, m_surface, &m_surface_capabilities);

    // Get available present modes
    uint32_t present_mode_count;
    fpGetPhysicalDeviceSurfacePresentModesKHR(physical_device, m_surface, &present_mode_count, NULL);
    assert(present_mode_count > 0);

    std::vector<VkPresentModeKHR> available_present_modes(present_mode_count);
    fpGetPhysicalDeviceSurfacePresentModesKHR(physical_device, m_surface, &present_mode_count, available_present_modes.data());

    if (m_surface_capabilities.currentExtent.width == (uint32_t)-1)
    {
        m_size.width  = m_size.width;
        m_size.height = m_size.height;
    }
    else
    {
        m_size        = m_surface_capabilities.currentExtent;
        m_size.width  = m_surface_capabilities.currentExtent.width;
        m_size.height = m_surface_capabilities.currentExtent.height;
    }

    VkPresentModeKHR desired_present_mode = VK_PRESENT_MODE_FIFO_KHR;

    if (!vsync)
    {
        for (size_t i = 0; i < present_mode_count; i++)
        {
            if (available_present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                desired_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
            if (available_present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
            {
                desired_present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            }
        }
    }

    // Determine the number of images
    uint32_t desired_swapchain_images_count = m_surface_capabilities.minImageCount + 1;
    if ((m_surface_capabilities.maxImageCount > 0) && (desired_swapchain_images_count > m_surface_capabilities.maxImageCount))
    {
        desired_swapchain_images_count = m_surface_capabilities.maxImageCount;
    }

    VkSurfaceTransformFlagsKHR preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    assert(m_surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR, "m_surface_capabilities does not support VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR");

    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    assert(m_surface_capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, "m_surface_capabilities does not support VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR");

    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    swapchain_create_info.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.pNext                    = NULL;
    swapchain_create_info.surface                  = m_surface;
    swapchain_create_info.minImageCount            = desired_swapchain_images_count;
    swapchain_create_info.imageFormat              = m_color_format;
    swapchain_create_info.imageColorSpace          = m_color_space;
    swapchain_create_info.imageExtent              = {m_size.width, m_size.height};
    swapchain_create_info.imageUsage               = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    swapchain_create_info.preTransform             = (VkSurfaceTransformFlagBitsKHR)preTransform;
    swapchain_create_info.imageArrayLayers         = 1;
    swapchain_create_info.imageSharingMode         = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.queueFamilyIndexCount    = 0;
    swapchain_create_info.pQueueFamilyIndices      = NULL;
    swapchain_create_info.presentMode              = desired_present_mode;
    swapchain_create_info.oldSwapchain             = previous_swapchain;
    // Setting clipped to VK_TRUE allows the implementation to discard rendering outside of the surface area
    swapchain_create_info.clipped        = VK_TRUE;
    swapchain_create_info.compositeAlpha = compositeAlpha;

    if (m_surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
    {
        swapchain_create_info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    assert(m_surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

    // Enable transfer destination on swap chain images if supported
    if (m_surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
    {
        swapchain_create_info.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    fpCreateSwapchainKHR(vk_device, &swapchain_create_info, nullptr, &m_swap_chain);

    //-------------------------------------------------------------//

    // Destroy old swapchain, cleaning...
    if (previous_swapchain != VK_NULL_HANDLE)
    {
        for (auto&& colour_attachment : m_colour_attachments)
        {
            colour_attachment->clean_view(vk_device);
        }
        fpDestroySwapchainKHR(vk_device, previous_swapchain, nullptr);
    }

    fpGetSwapchainImagesKHR(vk_device, m_swap_chain, &m_swapchain_image_count, NULL);
    // store the images in a tmp vector and sets them to the 'render texture' class
    std::vector<VkImage> tmp_swapchain_images(m_swapchain_image_count);
    fpGetSwapchainImagesKHR(vk_device, m_swap_chain, &m_swapchain_image_count, tmp_swapchain_images.data());

    m_colour_attachments.resize(m_swapchain_image_count);

    // Create the same amount of image views as this swapchain images has
    for (uint32_t i = 0; i < m_colour_attachments.size(); ++i)
    {
        TextureInfo info = {};
        // fake it, since the image is already created by the system, we need to create only the view
        info.extent           = {m_size.width, m_size.height};
        info.format           = m_color_format;
        info.imageType        = VkImageType::VK_IMAGE_TYPE_2D;
        info.m_current_layout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
        info.samples          = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
        info.usage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        // info.finalLayout = ;
        // info.loadOp = ;
        // info.storeOp = ;
        // info.stencilLoadOp = ;
        // info.stencilStoreOp = ;

        VkImageViewCreateInfo colorAttachmentView           = {};
        colorAttachmentView.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        colorAttachmentView.pNext                           = NULL;
        colorAttachmentView.format                          = m_color_format;
        colorAttachmentView.components                      = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
        colorAttachmentView.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        colorAttachmentView.subresourceRange.baseMipLevel   = 0;
        colorAttachmentView.subresourceRange.levelCount     = 1;
        colorAttachmentView.subresourceRange.baseArrayLayer = 0;
        colorAttachmentView.subresourceRange.layerCount     = 1;
        colorAttachmentView.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        colorAttachmentView.flags                           = 0;
        colorAttachmentView.image                           = tmp_swapchain_images[i];

        VkImageView tmp_image_view;
        vkCreateImageView(vk_device, &colorAttachmentView, nullptr, &tmp_image_view);

        m_colour_attachments[i]  = new ColourAttachment(info);
        auto&& colour_attachment = *m_colour_attachments[i];

        colour_attachment.set_image(tmp_swapchain_images[i]);
        colour_attachment.set_view(tmp_image_view);
    }
}

VkResult Swapchain::acquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t* imageIndex)
{
    // By setting timeout to UINT64_MAX we will always wait until the next image has been acquired or an actual error is
    // thrown With that we don't have to handle VK_NOT_READY
    return fpAcquireNextImageKHR(m_engine.m_device.m_vulkan_device, m_swap_chain, UINT64_MAX, presentCompleteSemaphore, (VkFence) nullptr, imageIndex);
}

VkResult Swapchain::queuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore)
{
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType            = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext            = NULL;
    presentInfo.swapchainCount   = 1;
    presentInfo.pSwapchains      = &m_swap_chain;
    presentInfo.pImageIndices    = &imageIndex;
    // Check if a wait semaphore has been specified to wait for before presenting the image
    if (waitSemaphore != VK_NULL_HANDLE)
    {
        presentInfo.pWaitSemaphores    = &waitSemaphore;
        presentInfo.waitSemaphoreCount = 1;
    }
    return fpQueuePresentKHR(queue, &presentInfo);
}
