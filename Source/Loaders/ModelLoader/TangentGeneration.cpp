//
// Created by otrush on 8/15/2026.
//
#include "Loaders/ModelLoader/MeshProcessing.hpp"

#include "mikktspace.h"

#include "Utility/Assertions.hpp"
#include "Utility/Logging/LogMacros.hpp"

namespace Shift::MeshProcessing {
    namespace {
        //! MikkTSpace addresses geometry as face + vertex-of-face and nothing else, which is why the
        //! mesh has to be unindexed first: corner f*3 + v is then a plain array subscript
        [[nodiscard]] size_t CornerOf(int face, int vertex) {
            return static_cast<size_t>(face) * 3 + static_cast<size_t>(vertex);
        }

        [[nodiscard]] VertexStreams& StreamsOf(const SMikkTSpaceContext* context) {
            return *static_cast<VertexStreams*>(context->m_pUserData);
        }

        int GetNumFaces(const SMikkTSpaceContext* context) {
            return static_cast<int>(StreamsOf(context).GetTriangleCount());
        }

        //! Triangles only
        int GetNumVerticesOfFace(const SMikkTSpaceContext*, const int) {
            return 3;
        }

        void GetPosition(const SMikkTSpaceContext* context, float outPosition[], const int face, const int vertex) {
            const glm::vec3& position = StreamsOf(context).positions[CornerOf(face, vertex)];
            outPosition[0] = position.x;
            outPosition[1] = position.y;
            outPosition[2] = position.z;
        }

        void GetNormal(const SMikkTSpaceContext* context, float outNormal[], const int face, const int vertex) {
            const glm::vec3& normal = StreamsOf(context).normals[CornerOf(face, vertex)];
            outNormal[0] = normal.x;
            outNormal[1] = normal.y;
            outNormal[2] = normal.z;
        }

        void GetTexCoord(const SMikkTSpaceContext* context, float outUV[], const int face, const int vertex) {
            const glm::vec2& uv = StreamsOf(context).uvs[CornerOf(face, vertex)];
            outUV[0] = uv.x;
            outUV[1] = uv.y;
        }

        //! Called for every corner of every face, so no tangent is left at its initial value
        void SetTSpaceBasic(const SMikkTSpaceContext* context, const float tangent[], const float sign,
                            const int face, const int vertex) {
            //! The sign is stored exactly as handed over: MikkTSpace defines
            //! bitangent = fSign * cross(normal, tangent), and glTF defines
            //! bitangent = cross(normal, tangent.xyz) * tangent.w. Same convention, no negation
            StreamsOf(context).tangents[CornerOf(face, vertex)] =
                glm::vec4{tangent[0], tangent[1], tangent[2], sign};
        }
    }

    bool GenerateTangentsIfMissing(VertexStreams& streams) {
        if (!streams.tangents.empty()) return true;

        Check(Warning, !streams.uvs.empty(), "MikkTSpace needs UVs and this mesh has none");
        Check(Warning, !streams.normals.empty(), "MikkTSpace needs normals and this mesh has none");
        Check(Warning, streams.indices.size() % 3 == 0, "MikkTSpace only handles triangle lists");
        //! Unindex gathers positions[index] with no range check of its own, so a producer other
        //! than GltfLoader (which validates in ProcessPrimitive) would get a heap read rather than
        //! the refusal the checks above imply
        for (const uint32_t index : streams.indices) {
            Check(Warning, index < streams.positions.size(), "An index points past the end of the vertex streams");
        }

        //! Copy so it remains unchanged at fail
        VertexStreams working = streams;

        Unindex(working);
        working.tangents.assign(working.positions.size(), glm::vec4{0.0f});

        SMikkTSpaceInterface mikkInterface{};
        mikkInterface.m_getNumFaces = GetNumFaces;
        mikkInterface.m_getNumVerticesOfFace = GetNumVerticesOfFace;
        mikkInterface.m_getPosition = GetPosition;
        mikkInterface.m_getNormal = GetNormal;
        mikkInterface.m_getTexCoord = GetTexCoord;
        mikkInterface.m_setTSpaceBasic = SetTSpaceBasic;
        //! m_setTSpace stays null: it delivers the bitangent and its magnitudes separately, which
        //! only matters for non-orthonormal tangent frames. glTF's TANGENT is xyz + sign
        mikkInterface.m_setTSpace = nullptr;

        SMikkTSpaceContext context{};
        context.m_pInterface = &mikkInterface;
        context.m_pUserData = &working;

        //! Generate
        if (genTangSpaceDefault(&context) == 0) {
            LogError("MeshProcessing: MikkTSpace refused the mesh ({} triangles), keeping it untouched",
                     working.GetTriangleCount());
            return false;
        }

        const uint32_t cornerCount = working.GetVertexCount();

        //! Put the index buffer back.
        const uint32_t weldedCount = WeldVertices(working);

        streams = std::move(working);

        LogInfo("MeshProcessing: generated tangents for {} triangles, {} corners welded to {} vertices",
                streams.GetTriangleCount(), cornerCount, weldedCount);
        return true;
    }
}
