#pragma once

#include <string>

// Represents a property in the shader, converting the name of the resource
// the shader expects to be set, to an integer value, to speed up sets/gets
// meant to be created as few possibles as possible (because of the string to uint32 conversion)
struct Parameter
{
    Parameter(const std::string& s)
    {
        set_parameter_id(s);
    }
    Parameter()
    {
    }

    bool operator==(const Parameter& other) const
    {
        return other.m_parameter_id == m_parameter_id;
    }
    bool equal(const Parameter& other) const
    {
        return other.m_parameter_id == m_parameter_id;
    };
    bool is_valid() const
    {
        return m_set;
    };
    uint32_t get_parameter_id() const
    {
        return m_parameter_id;
    };

    void set_parameter_id(const std::string& s)
    {
        m_parameter_id = std::hash<std::string>{}(s);
        m_set          = true;
    }

    const bool is_bound() const
    {
        return m_bound_to_descriptor;
    };

    void set_bound()
    {
        m_bound_to_descriptor = true;
    };

  private:
    uint32_t m_parameter_id;
    bool     m_bound_to_descriptor = false;
    bool     m_set                 = false;
};

namespace std
{
template <> struct hash<Parameter>
{
    std::size_t operator()(const Parameter& parameter) const
    {
        return parameter.get_parameter_id();
    }
};
}; // namespace std
