#pragma once

#include <VulkanSDK/1.2.198.1/Include/vulkan/vulkan.h>

#include <buffers_description.h>
#include <parameter.h>
#include <vector>

enum class DataType
{
    bool1,
    bool2,
    bool3,
    bool4,
    float1,
    float2,
    float3,
    float4,
    float2x2,
    float3x3,
    float4x4,
    int1,
    int2,
    int3,
    int4,
    int2x2,
    int3x3,
    int4x4,
    uint1,
    uint2,
    uint3,
    uint4,
    uint2x2,
    uint3x3,
    uint4x4,
    texture,
    sampler,
};

enum MemoryAccess
{
    readonly,
    writeonly,
    readwrite
};

struct ShaderStage
{
    VkShaderStageFlagBits m_stage;
};

struct VertexBufferReflection : ShaderStage
{
    Parameter m_name;
    uint32_t  m_location;
    DataType  m_data_type;
};

struct StructMember : ShaderStage
{
    Parameter m_name;
    DataType  m_data_type;
};

struct UniformBuffer : ShaderStage
{
    Parameter m_name;
    uint32_t  m_set;
    uint32_t  m_binding;

    std::vector<StructMember> m_struct_members;
};

struct StorageBuffer : ShaderStage
{
    Parameter    m_name;
    uint32_t     m_set;
    uint32_t     m_binding;
    MemoryAccess m_memory_access;

    std::vector<StructMember> m_struct_members;
};

struct TextureReflection : ShaderStage
{
    Parameter    m_name;
    uint32_t     m_binding;
    DataType     m_data_type;
    MemoryAccess m_memory_access; // missing to be filled yet by reflection...
};
typedef VertexBufferReflection FragmentOutput;

struct SamplerReflection : ShaderStage // TODO: expand this
{
    Parameter m_name;
    uint32_t  m_binding;
    DataType  m_data_type;
};

// Buffers

class PipelineResource
{
  public:
    const bool get_created() const
    {
        return m_created;
    };

  protected:
    void set_created()
    {
        m_created = true;
    };
    void unset_created()
    {
        m_created = false;
    };

  private:
    bool m_created = false;
};

class ColourAttachment : public PipelineResource
{
  public:
    ColourAttachment(struct TextureInfo info) : m_render_texture_info(info){};
    void create(const class Engine& engine);

    const VkImageView& view() const
    {
        return m_view;
    }
    const VkImage& image() const
    {
        return m_image;
    }

    void set_view(const VkImageView& view)
    {
        m_view = view;
    }
    void set_image(const VkImage& image)
    {
        m_image = image;
    }

    void clean_view(const VkDevice& vk_device)
    {
        vkDestroyImageView(vk_device, m_view, nullptr);
    }
    void clean_image(const VkDevice& vk_device)
    {
        vkDestroyImage(vk_device, m_image, nullptr);
    }
    void clean_memory(const VkDevice& vk_device)
    {
        vkFreeMemory(vk_device, m_memory, nullptr);
    }

    ~ColourAttachment()
    {
        if (!get_created())
        {
            return;
        }
        // clean_view();
        // clean_image();
        // clean_memory();
    };
    struct TextureInfo    m_render_texture_info;
    VkDeviceMemory        m_memory;
    VkImageView           m_view;
    VkImageView           m_depth_view_test; // TODO: remove
    VkImage               m_image = VK_NULL_HANDLE;
    VkExtent2D            m_extent;
    VkDescriptorImageInfo m_descriptor = {};
};

/* class Texture : public ColourAttachment
{
  public:
    VkDescriptorImageInfo m_descriptor;
};

class StorageTexture : public ColourAttachment
{
  public:
    VkDescriptorImageInfo m_descriptor;
};*/

class Sampler : public PipelineResource
{
  public:
};

// In order to completely implement this class, I need to create some kind of
// queue execution class, that holds all the command buffers, since it will be the best if
// could make use of an already created transfer_queue, specifying if this is deferred or straight away
// transfer operation (transfer_to())
class Buffer : public PipelineResource
{
  public:
    Buffer(const struct BufferInfo buffer_info) : m_buffer_info(buffer_info){};
    // Map the buffer if possible and return the mapped data, also maps it to m_mapped_memory
    // if can not be mapped, it will assert
    void map(const VkDevice& vulkan_device);
    // Unmap the buffer, assert if not mapped
    void  unmap(const VkDevice& vulkan_device);
    void* get_mapped_memory()
    {
        return m_mapped_memory;
    };
    // store the copy command into a command buffer, it will happen at some point in the rendergraph execution
    void transfer_to(class Engine& engine, Buffer& target_buffer, const uint32_t src_offset, const uint32_t dst_offset, const uint32_t size);
    // request an unused command buffer reserved for this operation, the copy command will happen straigh away
    // remove inmediate concepth, replace it with a transfer operation at the very begining of the graph (graph needs to be rebuilt if so??)
    void transfer_to_inmediate(class Engine& engine, Buffer& target_buffer, const uint32_t src_offset, const uint32_t dst_offset, const uint32_t size);
    void flush(const VkDevice& vulkan_device, const uint32_t offset, const uint32_t size);
    // Create the buffer, create its memory and bind them together, and create a view of this buffer whole range
    // (m_descriptor is the view)
    void create(const VkDevice& vulkan_device);

    const VkBuffer& get_buffer() const
    {
        return m_buffer;
    }
    const VkDescriptorBufferInfo& get_descriptor() const
    {
        return m_descriptor;
    }

    const struct BufferInfo& get_buffer_info() const
    {
        return m_buffer_info;
    };

    void destroy_resources(const VkDevice& vulkan_device);

  private:
    const struct BufferInfo m_buffer_info;
    // This is the pointer where the memory will be mapped,
    // might be the case that this buffer has the memory always mapped?
    // is a testing idea...
    void* m_mapped_memory = nullptr;

    VkMemoryRequirements m_memory_requirements;
    VkDeviceMemory       m_memory;
    VkBuffer             m_buffer;
    // buffer view, right now represents a view of the whole buffer... ideally,
    // this should be something that needs to be queried out of the buffer
    // so it can create multiple views out of the same buffer ?
    VkDescriptorBufferInfo m_descriptor;
};

class VertexBuffer : public Buffer
{
  public:
    VertexBuffer(const struct BufferInfo buffer_info, uint32_t vertex_count) : Buffer(buffer_info), m_vertex_count(vertex_count){};

    uint32_t get_vertex_count() const
    {
        return m_vertex_count;
    };

    void add_input_description(const VkVertexInputAttributeDescription& input_description)
    {
        m_input_descriptions.push_back(input_description);
    }
    void add_binding_description(const VkVertexInputBindingDescription& binding)
    {
        m_bindings.push_back(binding);
    }

  private:
    uint32_t                                       m_vertex_count;
    std::vector<VkVertexInputAttributeDescription> m_input_descriptions;
    std::vector<VkVertexInputBindingDescription>   m_bindings;
};

class IndexBuffer : public Buffer
{
  public:
    IndexBuffer(const struct BufferInfo buffer_info, uint32_t index_count) : Buffer(buffer_info), m_index_count(index_count){};
    uint32_t get_index_count() const
    {
        return m_index_count;
    };

  private:
    uint32_t m_index_count;
};
