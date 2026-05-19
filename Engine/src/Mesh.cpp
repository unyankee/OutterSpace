#include "Mesh.h"

#include "meshoptimizer.h"
#include "Common/Common.h"

#include <extern/meshoptimizer/extern/fast_obj.h>
#include <string>
#include <cstdio>
#include <vector>

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

        fast_obj_destroy(mesh);

        return true;
    }

}
