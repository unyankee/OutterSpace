#pragma once

#include <vector>
#include <cstdint>

namespace ToyEngine
{

    struct Vertex
    {
        float m_vx, m_vy, m_vz;
        float m_nx, m_ny, m_nz;
        float m_tu, m_tv;
    };

    struct Mesh
    {
        std::vector<Vertex> m_vertices;
        std::vector<uint32_t> m_indices;

        bool loadFromObj(const char* path);
    };

}
