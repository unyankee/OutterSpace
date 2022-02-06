#pragma once

#include <VulkanSDK/1.2.198.1/Include/vulkan/vulkan.h>
#include <buffers.h>
#include <vector>

class Swapchain
{
  public:
    ~Swapchain();
    Swapchain(class Engine& engine) : m_engine(engine)
    {
    }
    void init(const VkInstance& vk_instance, const VkDevice& vk_device);
    void create_swapchain(bool vsync);
    void set_size(uint32_t width, uint32_t height)
    {
        m_size = {width, height};
    };
    const uint32_t width() const
    {
        return m_size.width;
    };
    const uint32_t height() const
    {
        return m_size.height;
    };
    const uint32_t image_count() const
    {
        return m_swapchain_image_count;
    };
    const VkImageView& view(const uint32_t i) const
    {
        return m_colour_attachments[i]->view();
    };
    const VkImage& image(const uint32_t i) const
    {
        return m_colour_attachments[i]->image();
    };
    const VkFormat& color_format() const
    {
        return m_color_format;
    }

    VkResult acquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t* imageIndex);
    VkResult queuePresent(VkQueue queue, uint32_t imageIndex, VkSemaphore waitSemaphore);

    std::vector<ColourAttachment*> m_colour_attachments;
    uint32_t                       m_swapchain_image_count;

  private:
    // PFN for swapchain windows creation
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR      fpGetPhysicalDeviceSurfaceSupportKHR;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR fpGetPhysicalDeviceSurfaceCapabilitiesKHR;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR      fpGetPhysicalDeviceSurfaceFormatsKHR;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR fpGetPhysicalDeviceSurfacePresentModesKHR;
    PFN_vkCreateSwapchainKHR                      fpCreateSwapchainKHR;
    PFN_vkDestroySwapchainKHR                     fpDestroySwapchainKHR;
    PFN_vkGetSwapchainImagesKHR                   fpGetSwapchainImagesKHR;
    PFN_vkAcquireNextImageKHR                     fpAcquireNextImageKHR;
    PFN_vkQueuePresentKHR                         fpQueuePresentKHR;

    VkSurfaceCapabilitiesKHR m_surface_capabilities;
    VkSwapchainKHR           m_swap_chain = VK_NULL_HANDLE;
    VkSurfaceKHR             m_surface;
    VkFormat                 m_color_format;
    VkColorSpaceKHR          m_color_space;
    VkExtent2D               m_size = {1280, 720};

    class Engine& m_engine;
};
