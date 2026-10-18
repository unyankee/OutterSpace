#pragma once

#include <vector>
#include <deque>
#include <cstdint>

#include "ResourceManager.h"

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
        struct MeshData
        {
            uint32_t VertexCount = 0;
            uint32_t MeshletsCount = 0;
            uint32_t MeshleVertexIndexCount = 0;
            uint32_t MeshletTrianglesCount = 0;
        };
        uint32_t Id;
        MeshData Data;
    };
    
    
    struct MeshSystem
    {
        struct MeshRegistry
        {
            uint32_t MeshletCount = 0;
            uint32_t VertexOffset = 0;
            uint32_t MeshletOffset = 0;
            uint32_t MeshletVertexOffset = 0;
            uint32_t MeshletTriangleOffset = 0;
        };
        static_assert(sizeof(MeshRegistry) == 20);
        
        void Init(ResourceManager& resourceManager);
        void UpdateBuffers(ResourceManager& resourceManager);
        Mesh* registerMesh(const char* path, ResourceManager& resourceManager);

        bool loadFromObj(const char* path, std::vector<Vertex>& LocalVertices, std::vector<uint32_t>& LocalIndices,
                         std::vector<Meshlet>& LocalMeshlets,
                         std::vector<uint32_t>& LocalMeshletVertices,
                         std::vector<uint32_t>& LocalMeshletTriangles);
        
        void buildMeshlets(std::vector<Vertex>& LocalVertices, std::vector<uint32_t>& LocalIndices,
                         std::vector<Meshlet>& LocalMeshlets,
                         std::vector<uint32_t>& LocalMeshletVertices,
                         std::vector<uint32_t>& LocalMeshletTriangles);
        
        void update(ResourceManager& resourceManager);
        
        uint32_t LastUpdated = 0;
        MeshRegistry RequestedMeshRegistry;
        std::vector<MeshRegistry> Registry;
        std::deque<Mesh> Meshes;
        
        MeshRegistry CurrentMeshRegistry;
        
        // CPU side
        std::vector<Vertex> m_vertices;
        std::vector<uint32_t> m_indices;
        std::vector<Meshlet> m_meshlets;
        std::vector<uint32_t> m_meshletVertices;
        std::vector<uint32_t> m_meshletTriangles;
        // GPU side
        BufferHandle vbHandle;
        BufferHandle meshletHandle;
        BufferHandle meshletVertexHandle;
        BufferHandle meshletTriangleHandle;
        BufferHandle meshRegistryHandle;
        uint32_t uploadedRegistryCount = 0;
    };
    
}
