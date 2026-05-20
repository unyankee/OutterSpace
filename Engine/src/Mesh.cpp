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

    bool Mesh::loadFromObj(const char* path)
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

        m_vertices.clear();
        m_indices.clear();

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
                unrolledVertices.push_back(faceVertices[kk + 1]);
                unrolledVertices.push_back(faceVertices[kk]);
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

        m_vertices.resize(uniqueVertexCount);
        m_indices.resize(totalIndices);

        meshopt_remapIndexBuffer(m_indices.data(), nullptr, totalIndices, remap.data());
        meshopt_remapVertexBuffer(m_vertices.data(), unrolledVertices.data(), totalIndices, sizeof(Vertex), remap.data());

        meshopt_optimizeVertexCache(m_indices.data(), m_indices.data(), totalIndices, uniqueVertexCount);
        buildMeshlets();

        fast_obj_destroy(mesh);

        return true;
    }

    void Mesh::buildMeshlets()
    {
        m_meshlets.clear();
        m_meshletVertices.clear();
        m_meshletTriangles.clear();

        if (m_indices.empty() || m_vertices.empty())
        {
            return;
        }

        size_t meshletBound = meshopt_buildMeshletsBound(m_indices.size(), MeshletMaxVertices, MeshletMaxTriangles);

        std::vector<meshopt_Meshlet> meshoptMeshlets(meshletBound);
        std::vector<unsigned int> meshletVertices(meshletBound * MeshletMaxVertices);
        std::vector<unsigned char> meshletTriangles(meshletBound * MeshletMaxTriangles * 3);

        size_t meshletCount = meshopt_buildMeshlets(
            meshoptMeshlets.data(),
            meshletVertices.data(),
            meshletTriangles.data(),
            m_indices.data(),
            m_indices.size(),
            &m_vertices[0].m_vx,
            m_vertices.size(),
            sizeof(Vertex),
            MeshletMaxVertices,
            MeshletMaxTriangles,
            0.0f
        );

        m_meshlets.reserve(meshletCount);
        m_meshletVertices.reserve(meshletCount * MeshletMaxVertices);
        m_meshletTriangles.reserve(meshletCount * MeshletMaxTriangles * 3);

        for (size_t i = 0; i < meshletCount; ++i)
        {
            meshopt_Meshlet& source = meshoptMeshlets[i];
            unsigned int* sourceVertices = meshletVertices.data() + source.vertex_offset;
            unsigned char* sourceTriangles = meshletTriangles.data() + source.triangle_offset;

            meshopt_optimizeMeshlet(sourceVertices, sourceTriangles, source.triangle_count, source.vertex_count);

            Meshlet meshlet{};
            meshlet.m_vertexOffset = static_cast<uint32_t>(m_meshletVertices.size());
            meshlet.m_triangleOffset = static_cast<uint32_t>(m_meshletTriangles.size());
            meshlet.m_vertexCount = source.vertex_count;
            meshlet.m_triangleCount = source.triangle_count;

            meshopt_Bounds bounds = meshopt_computeMeshletBounds(
                sourceVertices,
                sourceTriangles,
                source.triangle_count,
                &m_vertices[0].m_vx,
                m_vertices.size(),
                sizeof(Vertex)
            );

            memcpy(meshlet.m_center, bounds.center, sizeof(meshlet.m_center));
            meshlet.m_radius = bounds.radius;
            memcpy(meshlet.m_coneApex, bounds.cone_apex, sizeof(meshlet.m_coneApex));
            meshlet.m_coneCutoff = bounds.cone_cutoff;
            memcpy(meshlet.m_coneAxis, bounds.cone_axis, sizeof(meshlet.m_coneAxis));

            m_meshlets.push_back(meshlet);

            for (uint32_t vertexIndex = 0; vertexIndex < source.vertex_count; ++vertexIndex)
            {
                m_meshletVertices.push_back(sourceVertices[vertexIndex]);
            }

            for (uint32_t triangleIndex = 0; triangleIndex < source.triangle_count * 3; ++triangleIndex)
            {
                m_meshletTriangles.push_back(sourceTriangles[triangleIndex]);
            }
        }
    }

}
