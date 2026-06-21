#pragma once

#include <vector>
#include <cstdint>

namespace ToyEngine
{
    constexpr uint32_t MeshletMaxVertices = 64;
    constexpr uint32_t MeshletMaxTriangles = 124;

    struct Vertex
    {
        float m_vx, m_vy, m_vz;
        float m_nx, m_ny, m_nz;
        float m_tu, m_tv;
    };

    struct Meshlet
    {
        uint32_t m_vertexOffset = 0;
        uint32_t m_triangleOffset = 0;
        uint32_t m_vertexCount = 0;
        uint32_t m_triangleCount = 0;

        float m_center[3] = {};
        float m_radius = 0.0f;

        float m_coneApex[3] = {};
        float m_coneCutoff = 0.0f;

        float m_coneAxis[3] = {};
        float m_padding = 0.0f;
    };
    
    struct Mesh
    {
        std::vector<Vertex> m_vertices;
        std::vector<uint32_t> m_indices;
        std::vector<Meshlet> m_meshlets;
        std::vector<uint32_t> m_meshletVertices;
        std::vector<uint32_t> m_meshletTriangles;

        bool loadFromObj(const char* path);

    private:
        void buildMeshlets();
    };

    class MeshManager
    {
    public:
        MeshManager();
        ~MeshManager();



        
    };

}
