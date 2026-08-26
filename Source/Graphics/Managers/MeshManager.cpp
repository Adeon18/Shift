//
// Created by otrush on 8/17/2026.
//

#include "MeshManager.hpp"

#include "Config/EngineConfig.hpp"
#include "Utility/Assertions.hpp"

namespace Shift::Graphics {
    namespace {
        //! I should move such functions into 1 header
        [[nodiscard]] constexpr uint32_t AlignUp16(uint64_t value) {
            return static_cast<uint32_t>((value + 15ull) & ~15ull);
        }
    }

    //! Should this even be a fucntion?
    constexpr uint32_t MeshManager::StreamStride(EStream stream) {
        switch (stream) {
            case Positions: return static_cast<uint32_t>(sizeof(glm::vec3));
            case Normals:   return static_cast<uint32_t>(sizeof(glm::vec3));
            case Tangents:  return static_cast<uint32_t>(sizeof(glm::vec4));
            case UVs:       return static_cast<uint32_t>(sizeof(glm::vec2));
            default:        return 0;
        }
    }

    constexpr const char* MeshManager::StreamName(EStream stream) {
        switch (stream) {
            case Positions: return "MgdPositions";
            case Normals:   return "MgdNormals";
            case Tangents:  return "MgdTangents";
            case UVs:       return "MgdUVs";
            default:        return "MergedUnknown";
        }
    }

    bool MeshManager::Init(RenderBackend* rhi, BufferManager* buffers) {
        m_rhi = rhi;
        m_backend = rhi->CreateInterface();
        m_buffers = buffers;

        for (uint32_t i = 0; i < StreamCount; ++i) {
            const EStream stream = static_cast<EStream>(i);

            BufferDescriptor desc;
            desc.name = StreamName(stream);
            //! These a vtx for now as buffer contructor adds trsnfer support there
            //! These are reached by ptr reegardless but I will let it be as it looks correct anyway
            desc.type = EBufferType::Vertex;
            desc.size = static_cast<uint64_t>(StreamStride(stream)) * Conf::MAX_SCENE_VERTICES;
            desc.isDeviceAddressable = true;

            m_streams[i] = m_buffers->CreateBuffer(desc);

            Buffer* buffer = m_buffers->Get(m_streams[i]);
            CheckCritical(buffer != nullptr, "Merged vertex stream handle did not resolve!");
            CheckCritical(buffer->IsValid(), "Failed to allocate a merged vertex stream!");
            CheckCritical(buffer->GetDeviceAddress() != 0,
                          "A merged vertex stream reported address 0, so no shader could pull from it!");
        }

        {
            BufferDescriptor desc;
            desc.name = "MgdIndices";
            desc.type = EBufferType::Index;
            //! sizeof here reads better than just "4" and I will die on this hill
            desc.size = static_cast<uint64_t>(sizeof(uint32_t)) * Conf::MAX_SCENE_INDICES;

            m_indices = m_buffers->CreateBuffer(desc);

            Buffer* buffer = m_buffers->Get(m_indices);
            CheckCritical(buffer != nullptr, "Merged index buffer handle did not resolve!");
            CheckCritical(buffer->IsValid(), "Failed to allocate the merged index buffer!");
        }

        m_vertexRanges.Init(Conf::MAX_SCENE_VERTICES);
        m_indexRanges.Init(Conf::MAX_SCENE_INDICES);

        return true;
    }

    MeshHandle MeshManager::UploadMesh(const MeshData& meshData, RenderContextEncoder& encoder) {
        const VertexStreams& streams = meshData.streams;

        const uint32_t vertexCount = streams.GetVertexCount();
        const uint32_t indexCount = streams.GetIndexCount();

        BufferRangeAllocator::Range vertexRange;
        if (!m_vertexRanges.Allocate(vertexCount, &vertexRange)) {
            Log(Error, "Merged vertex streams are full: mesh '{}' wants {} vertices, largest free run is {} of {}",
                meshData.name, vertexCount, m_vertexRanges.GetLargestFreeRun(), m_vertexRanges.GetCapacity());
            return {};
        }

        BufferRangeAllocator::Range indexRange;
        if (!m_indexRanges.Allocate(indexCount, &indexRange)) {
            //! Hand the vertices back as the mesh must be in both
            m_vertexRanges.Free(vertexRange);
            Log(Error, "Merged index buffer is full: mesh '{}' wants {} indices, largest free run is {} of {}",
                meshData.name, indexCount, m_indexRanges.GetLargestFreeRun(), m_indexRanges.GetCapacity());
            return {};
        }

        //! One staging buffer per mesh holding all five regions back to back
        std::array<StreamUpload, StreamCount> uploads{};
        uploads[Positions] = {streams.positions.data(), vertexCount * StreamStride(Positions)};
        uploads[Normals]   = {streams.normals.data(),   vertexCount * StreamStride(Normals)};
        uploads[Tangents]  = {streams.tangents.data(),  vertexCount * StreamStride(Tangents)};
        uploads[UVs]       = {streams.uvs.data(),       vertexCount * StreamStride(UVs)};

        const uint32_t indexStride = static_cast<uint32_t>(sizeof(uint32_t));
        const uint32_t indexBytes = indexCount * indexStride;

        std::array<uint32_t, StreamCount + 1> stagingOffsets{};
        uint32_t stagingSize = 0;
        for (uint32_t i = 0; i < StreamCount; ++i) {
            stagingOffsets[i] = stagingSize;
            stagingSize = AlignUp16(static_cast<uint64_t>(stagingSize) + uploads[i].size);
        }
        //! Index
        stagingOffsets[StreamCount] = stagingSize;
        stagingSize = AlignUp16(static_cast<uint64_t>(stagingSize) + indexBytes);

        BufferDescriptor stagingDesc;
        stagingDesc.name = "MeshUploadStaging";
        stagingDesc.type = EBufferType::Staging;
        stagingDesc.size = stagingSize;

        Buffer* staging = m_backend->CreateBuffer(stagingDesc);
        if (!staging || !staging->IsValid()) {
            delete staging;
            m_vertexRanges.Free(vertexRange);
            m_indexRanges.Free(indexRange);
            Log(Error, "Failed to allocate a {} byte staging buffer for mesh '{}'", stagingSize, meshData.name);
            return {};
        }

        for (uint32_t i = 0; i < StreamCount; ++i) {
            const EStream stream = static_cast<EStream>(i);
            //! Fill and copy command per stream because we copy to diff buffers in the end
            staging->Fill(uploads[i].data, uploads[i].size, stagingOffsets[i]);

            encoder.CopyBufferToBuffer(
                {staging, stagingOffsets[i]},
                {m_buffers->Get(m_streams[i]), vertexRange.first * StreamStride(stream)},
                uploads[i].size);
        }

        staging->Fill(streams.indices.data(), indexBytes, stagingOffsets[StreamCount]);
        encoder.CopyBufferToBuffer(
            {staging, stagingOffsets[StreamCount]},
            {m_buffers->Get(m_indices), indexRange.first * indexStride},
            indexBytes);

        m_usedStagingBuffers.emplace_back(staging);

        Mesh* mesh = new Mesh{
            .name = meshData.name,
            .vertexRange = vertexRange,
            .indexRange = indexRange,
            .submeshes = meshData.submeshes,
            .bounds = meshData.bounds
        };

        Log(Info, "Uploaded mesh '{}': {} vertices at base {}, {} indices at base {}, {} submeshes",
            mesh->name, vertexCount, vertexRange.first, indexCount, indexRange.first, mesh->submeshes.size());

        return m_pool.Insert(mesh);
    }

    uint64_t MeshManager::GetStreamAddress(EStream stream) const {
        if (stream >= StreamCount) { return 0; }

        const Buffer* buffer = m_buffers->Get(m_streams[stream]);
        return buffer ? buffer->GetDeviceAddress() : 0;
    }

    Buffer* MeshManager::GetIndexBuffer() const {
        return m_buffers->Get(m_indices);
    }

    void MeshManager::FreeStagingBuffers() {
        m_usedStagingBuffers.clear();
    }

    void MeshManager::Destroy() {
        m_pool.ForEachLive([](uint32_t, Mesh* mesh) { delete mesh; });
        m_pool.Clear();

        m_usedStagingBuffers.clear();
    }
}
