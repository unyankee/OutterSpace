#include "Mesh.h"

#include "meshoptimizer.h"
#include "Common/Common.h"

#include <extern/meshoptimizer/extern/fast_obj.h>
#include <string>
#include <cstdio>
#include <vector>
#include <cstring>

namespace ToyEngine
{
 
    void MeshSystem::Init(ResourceManager& resourceManager)
    {
        // For now, lets focus on pre allocating blindly quite some big buffers
        // that are going to be filled with vertex/meshlet data, 200mb each
        constexpr uint64_t DefaultBufferBytes = 200ull * 1024 * 1024;

        constexpr uint32_t DefaultVertexBufferAllocation              = DefaultBufferBytes / sizeof(Vertex);
        constexpr uint32_t DefaultMeshletsBufferAllocation            = DefaultBufferBytes / sizeof(Meshlet);
        constexpr uint32_t DefaultMeshletVertexIndexBufferAllocation  = DefaultBufferBytes / sizeof(uint32_t);
        constexpr uint32_t DefaultMeshletTriIndexBufferAllocation     = DefaultBufferBytes / sizeof(uint32_t);

        static_assert(sizeof(Vertex) == 32);
        static_assert(sizeof(Meshlet) == 64);
        
        vbHandle = resourceManager.createBuffer((uint32_t)(DefaultVertexBufferAllocation * sizeof(Vertex)),
                                               VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                               VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, nullptr);

        meshletHandle = resourceManager.createBuffer((uint32_t)(DefaultMeshletsBufferAllocation * sizeof(Meshlet)),
                                                                  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                                                  VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                                  nullptr);

        meshletVertexHandle = resourceManager.createBuffer(
            (uint32_t)(DefaultMeshletVertexIndexBufferAllocation * sizeof(uint32_t)),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, nullptr);

        meshRegistryHandle = resourceManager.createBuffer(static_cast<uint32_t>(sizeof(MeshRegistry) * MaxTransformsPerScene),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, nullptr);

        meshletTriangleHandle = resourceManager.createBuffer(
            (uint32_t)(DefaultMeshletTriIndexBufferAllocation * sizeof(uint32_t)),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, nullptr);
    }

    void MeshSystem::UpdateBuffers(ResourceManager& resourceManager)
    {
        // These are gpu data, they do not exist on cpu, so need an auxiliar buffer with the new data, and then copied over
        // in the subsection required
        // Get size between current data and new data, area that needs an update
        const MeshRegistry& NewRegistry = Registry[LastUpdated];
        const uint32_t SizeVB = RequestedMeshRegistry.VertexOffset - CurrentMeshRegistry.VertexOffset;
        const uint32_t SizeMeslets = RequestedMeshRegistry.MeshletOffset - CurrentMeshRegistry.MeshletOffset;
        const uint32_t SizeMesletVertexInd = RequestedMeshRegistry.MeshletVertexOffset - CurrentMeshRegistry.MeshletVertexOffset;
        const uint32_t SizeMesletTris = RequestedMeshRegistry.MeshletTriangleOffset - CurrentMeshRegistry.MeshletTriangleOffset;

        const Vertex* srcVertices = m_vertices.data() + CurrentMeshRegistry.VertexOffset;
        const Meshlet* srcMeshlets = m_meshlets.data() + CurrentMeshRegistry.MeshletOffset;
        const uint32_t* srcMeshVerts = m_meshletVertices.data() + CurrentMeshRegistry.MeshletVertexOffset;
        const uint32_t* srcMeshTris = m_meshletTriangles.data() + CurrentMeshRegistry.MeshletTriangleOffset;

        
        BufferHandle StagingvbHandle = resourceManager.createBuffer((uint32_t)(SizeVB * sizeof(Vertex)),
                                                                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, srcVertices);

        BufferHandle StagingMeshletHandle = resourceManager.createBuffer((uint32_t)(SizeMeslets * sizeof(Meshlet)),
                                                                         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, srcMeshlets);

        BufferHandle StagingMeshletVertexHandle = resourceManager.createBuffer(
            (uint32_t)(SizeMesletVertexInd * sizeof(uint32_t)),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, srcMeshVerts);

        BufferHandle StagingMeshletTriangleHandle = resourceManager.createBuffer(
            (uint32_t)(SizeMesletTris * sizeof(uint32_t)),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, srcMeshTris);
        
        
        resourceManager.copyToBufferRegion(StagingvbHandle, vbHandle, SizeVB * sizeof(Vertex), 0, CurrentMeshRegistry.VertexOffset * sizeof(Vertex));
        resourceManager.copyToBufferRegion(StagingMeshletHandle, meshletHandle, SizeMeslets * sizeof(Meshlet), 0, CurrentMeshRegistry.MeshletOffset * sizeof(Meshlet));
        resourceManager.copyToBufferRegion(StagingMeshletVertexHandle, meshletVertexHandle, SizeMesletVertexInd * sizeof(uint32_t), 0, CurrentMeshRegistry.MeshletVertexOffset * sizeof(uint32_t));
        resourceManager.copyToBufferRegion(StagingMeshletTriangleHandle, meshletTriangleHandle, SizeMesletTris * sizeof(uint32_t), 0, CurrentMeshRegistry.MeshletTriangleOffset * sizeof(uint32_t));        
        
        
        resourceManager.destroyBuffer(StagingvbHandle);
        resourceManager.destroyBuffer(StagingMeshletHandle);
        resourceManager.destroyBuffer(StagingMeshletVertexHandle);
        resourceManager.destroyBuffer(StagingMeshletTriangleHandle);
    }

    Mesh* MeshSystem::registerMesh(const char* path, ResourceManager& resourceManager)
    {
        //bad bad...
        Mesh mesh;
        mesh.Id = Meshes.size();
        
        std::vector<Vertex> LocalVertices;
        std::vector<uint32_t> LocalIndices;
        
        std::vector<Meshlet> LocalMeshlets;
        std::vector<uint32_t> LocalMeshletVertices;
        std::vector<uint32_t> LocalMeshletTriangles;
        
        loadFromObj(path, LocalVertices, LocalIndices, LocalMeshlets, LocalMeshletVertices,LocalMeshletTriangles);        
        
        MeshRegistry meshOffsets = RequestedMeshRegistry;
        meshOffsets.MeshletCount = static_cast<uint32_t>(LocalMeshlets.size());
        assert(Registry.size() < MaxTransformsPerScene && "Mesh registry exceeds configured capacity");
        Registry.push_back(meshOffsets);

        // naivest
        RequestedMeshRegistry.VertexOffset += LocalVertices.size();
        RequestedMeshRegistry.MeshletOffset += LocalMeshlets.size();
        RequestedMeshRegistry.MeshletTriangleOffset += LocalMeshletTriangles.size();
        RequestedMeshRegistry.MeshletVertexOffset += LocalMeshletVertices.size();
        
        m_vertices.insert(m_vertices.end(), LocalVertices.begin(), LocalVertices.end());
        m_indices.insert(m_indices.end(), LocalIndices.begin(), LocalIndices.end());
        m_meshlets.insert(m_meshlets.end(), LocalMeshlets.begin(), LocalMeshlets.end());
        m_meshletVertices.insert(m_meshletVertices.end(), LocalMeshletVertices.begin(), LocalMeshletVertices.end());
        m_meshletTriangles.insert(m_meshletTriangles.end(), LocalMeshletTriangles.begin(), LocalMeshletTriangles.end());
        
        mesh.Data.VertexCount = LocalVertices.size();
        mesh.Data.MeshleVertexIndexCount = LocalMeshletVertices.size();
        mesh.Data.MeshletTrianglesCount = LocalMeshletTriangles.size();
        mesh.Data.MeshletsCount = LocalMeshlets.size();
        
        Meshes.emplace_back(mesh);
        
        
        // cutting corners
        return &Meshes[mesh.Id];
    }

    bool MeshSystem::loadFromObj(const char* path, std::vector<Vertex>& LocalVertices,
                                 std::vector<uint32_t>& LocalIndices,
                                 std::vector<Meshlet>& LocalMeshlets,
                                 std::vector<uint32_t>& LocalMeshletVertices,
                                 std::vector<uint32_t>& LocalMeshletTriangles)
    {
        if (!path)
        {
            return false;
        }

        std::string fullPath = std::string(ENGINE_PROJECT_ROOT) + "/" + path;
        fastObjMesh* mesh = fast_obj_read(fullPath.c_str());

        if (!mesh)
        {
            printf("Error: Could not find mesh at %s\n", fullPath.c_str());
            return false;
        }

        LocalVertices.clear();
        LocalIndices.clear();

        std::vector<Vertex> unrolledVertices;
        unrolledVertices.reserve(mesh->face_count * 3);

        unsigned int globalIndexCursor = 0;

        for (unsigned int faceIdx = 0; faceIdx < mesh->face_count; ++faceIdx)
        {
            unsigned int fv = mesh->face_vertices[faceIdx];

            std::vector<Vertex> faceVertices;
            faceVertices.reserve(fv);

            for (unsigned int vIdx = 0; vIdx < fv; ++vIdx)
            {
                fastObjIndex mi = mesh->indices[globalIndexCursor + vIdx];

                Vertex v{};

                if (mi.p)
                {
                    v.m_vx = mesh->positions[3 * mi.p + 0];
                    v.m_vy = mesh->positions[3 * mi.p + 1];
                    v.m_vz = mesh->positions[3 * mi.p + 2];
                }

                if (mi.t)
                {
                    v.m_tu = mesh->texcoords[2 * mi.t + 0];
                    v.m_tv = mesh->texcoords[2 * mi.t + 1];
                }

                if (mi.n)
                {
                    v.m_nx = mesh->normals[3 * mi.n + 0];
                    v.m_ny = mesh->normals[3 * mi.n + 1];
                    v.m_nz = mesh->normals[3 * mi.n + 2];
                }

                faceVertices.push_back(v);
            }

            for (unsigned int kk = 1; kk < fv - 1; kk++)
            {
                unrolledVertices.push_back(faceVertices[0]);
                unrolledVertices.push_back(faceVertices[kk]);
                unrolledVertices.push_back(faceVertices[kk + 1]);
            }

            globalIndexCursor += fv;
        }

        size_t totalIndices = unrolledVertices.size();
        std::vector<unsigned int> remap(totalIndices);

        size_t uniqueVertexCount = meshopt_generateVertexRemap(
            remap.data(),
            nullptr,
            totalIndices,
            unrolledVertices.data(),
            totalIndices,
            sizeof(Vertex)
        );

        LocalVertices.resize(uniqueVertexCount);
        LocalIndices.resize(totalIndices);

        meshopt_remapIndexBuffer(LocalIndices.data(), nullptr, totalIndices, remap.data());
        meshopt_remapVertexBuffer(LocalVertices.data(), unrolledVertices.data(), totalIndices, sizeof(Vertex), remap.data());

        meshopt_optimizeVertexCache(LocalIndices.data(), LocalIndices.data(), totalIndices, uniqueVertexCount);
        buildMeshlets(LocalVertices, LocalIndices, LocalMeshlets, LocalMeshletVertices,LocalMeshletTriangles);

        fast_obj_destroy(mesh);

        return true;
    }

    void MeshSystem::buildMeshlets(std::vector<Vertex>& LocalVertices, std::vector<uint32_t>& LocalIndices,
                         std::vector<Meshlet>& LocalMeshlets,
                         std::vector<uint32_t>& LocalMeshletVertices,
                         std::vector<uint32_t>& LocalMeshletTriangles)
    {
        LocalMeshlets.clear();
        LocalMeshletVertices.clear();
        LocalMeshletTriangles.clear();

        if (LocalIndices.empty() || LocalVertices.empty())
        {
            return;
        }

        size_t meshletBound = meshopt_buildMeshletsBound(LocalIndices.size(), MeshletMaxVertices, MeshletMaxTriangles);

        std::vector<meshopt_Meshlet> meshoptMeshlets(meshletBound);
        std::vector<unsigned int> meshletVertices(meshletBound * MeshletMaxVertices);
        std::vector<unsigned char> meshletTriangles(meshletBound * MeshletMaxTriangles * 3);

        size_t meshletCount = meshopt_buildMeshlets(
            meshoptMeshlets.data(),
            meshletVertices.data(),
            meshletTriangles.data(),
            LocalIndices.data(),
            LocalIndices.size(),
            &LocalVertices[0].m_vx,
            LocalVertices.size(),
            sizeof(Vertex),
            MeshletMaxVertices,
            MeshletMaxTriangles,
            0.0f
        );

        LocalMeshlets.reserve(meshletCount);
        LocalMeshletVertices.reserve(meshletCount * MeshletMaxVertices);
        LocalMeshletTriangles.reserve(meshletCount * MeshletMaxTriangles * 3);

        for (size_t i = 0; i < meshletCount; ++i)
        {
            meshopt_Meshlet& source = meshoptMeshlets[i];
            unsigned int* sourceVertices = meshletVertices.data() + source.vertex_offset;
            unsigned char* sourceTriangles = meshletTriangles.data() + source.triangle_offset;

            meshopt_optimizeMeshlet(sourceVertices, sourceTriangles, source.triangle_count, source.vertex_count);

            Meshlet meshlet{};
            meshlet.m_vertexOffset = static_cast<uint32_t>(LocalMeshletVertices.size());
            meshlet.m_triangleOffset = static_cast<uint32_t>(LocalMeshletTriangles.size());
            meshlet.m_vertexCount = source.vertex_count;
            meshlet.m_triangleCount = source.triangle_count;

            meshopt_Bounds bounds = meshopt_computeMeshletBounds(
                sourceVertices,
                sourceTriangles,
                source.triangle_count,
                &LocalVertices[0].m_vx,
                LocalVertices.size(),
                sizeof(Vertex)
            );

            memcpy(meshlet.m_center, bounds.center, sizeof(meshlet.m_center));
            meshlet.m_radius = bounds.radius;
            memcpy(meshlet.m_coneApex, bounds.cone_apex, sizeof(meshlet.m_coneApex));
            meshlet.m_coneCutoff = bounds.cone_cutoff;
            memcpy(meshlet.m_coneAxis, bounds.cone_axis, sizeof(meshlet.m_coneAxis));

            LocalMeshlets.push_back(meshlet);

            for (uint32_t vertexIndex = 0; vertexIndex < source.vertex_count; ++vertexIndex)
            {
                LocalMeshletVertices.push_back(sourceVertices[vertexIndex]);
            }

            for (uint32_t triangleIndex = 0; triangleIndex < source.triangle_count * 3; ++triangleIndex)
            {
                LocalMeshletTriangles.push_back(sourceTriangles[triangleIndex]);
            }
        }
    }

    void MeshSystem::update(ResourceManager& resourceManager)
    {
        const bool RequiresUpdate = CurrentMeshRegistry.VertexOffset != RequestedMeshRegistry.VertexOffset;
        if (Registry.size() != uploadedRegistryCount)
        {
            if (!Registry.empty())
            {
                Buffer* registryBuffer = resourceManager.getBuffer(meshRegistryHandle);
                registryBuffer->copyDataToBuffer(Registry.data(), static_cast<uint32_t>(Registry.size() * sizeof(MeshRegistry)));
            }
            uploadedRegistryCount = static_cast<uint32_t>(Registry.size());
        }
        if (RequiresUpdate)
        {
            UpdateBuffers(resourceManager);
            
            LastUpdated = Registry.size();
            CurrentMeshRegistry = RequestedMeshRegistry;
        }
    }
}
