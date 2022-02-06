#pragma once

#include <resource.h>
#include <string>

// struct still WIP, developed at the same time I am adding new resources to the shaders...
struct ReflectedResource // bad naming
{
    // copied from BaseType spirv_common.hpp
    enum ValueType
    {
        Unknown,
        Void,
        Boolean,
        SByte,
        UByte,
        Short,
        UShort,
        Int,
        UInt,
        Int64,
        UInt64,
        AtomicCounter,
        Half,
        Float,
        Double,
        Struct,
        Image,
        SampledImage,
        Sampler,
        AccelerationStructure,
        RayQuery,
        ///
        ///// Keep internal types at the end.
        /// ControlPointArray,
        /// Interpolant,
        /// Char
    };

    // ReflectedResource(Parameter p) : m_resource_name(p){};

    std::string  m_resource_name;
    ResourceType m_resource_type;
    uint32_t     m_location; // for vertex input layout >.>
    uint32_t     m_binding;
    uint32_t     m_descriptor_set;

    struct TypeData
    {
        std::string m_name;
        ValueType   m_value_type;
        // if this is 0, means that might be a non-defined size
        // like an array in a SSBO, what means the size is defined from the CPU size when
        // data is loaded
        uint32_t m_member_size;
        uint32_t m_rows;
        uint32_t m_cols;
    };
    // testing...
    std::vector<TypeData> m_members_type_data;

    bool m_read_only  = false;
    bool m_write_only = false;
};
