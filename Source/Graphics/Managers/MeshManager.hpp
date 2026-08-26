//
// Created by otrush on 8/17/2026.
//

#ifndef SHIFT_MESHMANAGER_HPP
#define SHIFT_MESHMANAGER_HPP

#include <array>
#include <string>
#include <vector>

#include "Graphics/RHI/RHI.hpp"
#include "Loaders/ModelLoader/IModelLoader.hpp"

#include "BufferManager.hpp"
#include "BufferRangeAllocator.hpp"
#include "GenerationalPool.hpp"

namespace Shift::Graphics {

    struct Mesh {
        std::string name;
        BufferRangeAllocator::Range vertexRange;
        //! Counted in 32-bit indices
        BufferRangeAllocator::Range indexRange;
        //! indexRange.first + submesh.firstIndex
        std::vector<SubmeshDesc> submeshes;
        Bounds bounds;
    };

    //! Later this will be useful (maybe not)
    using MeshHandle = GenerationalPool<Mesh>::Handle;

    //! Wraps five merged buffers every scene mesh suballocates from
    class MeshManager {
    public:
        enum EStream : uint32_t {
            Positions = 0,
            Normals,
            Tangents,
            UVs,
            StreamCount
        };

        [[nodiscard]] bool Init(RenderBackend* rhi, BufferManager* buffers);

        //! Grab a range for a mesh and record transfer queue comamnds
        [[nodiscard]] MeshHandle UploadMesh(const MeshData& meshData, RenderContextEncoder& encoder);

        [[nodiscard]] const Mesh* Get(MeshHandle handle) const { return m_pool.Get(handle); }
        [[nodiscard]] bool IsValid(MeshHandle handle) const { return m_pool.IsValid(handle); }

        //! Device adress for frame constants
        [[nodiscard]] uint64_t GetStreamAddress(EStream stream) const;

        //! There can only be one
        [[nodiscard]] Buffer* GetIndexBuffer() const;

        [[nodiscard]] uint32_t GetUsedVertices() const { return m_vertexRanges.GetUsed(); }
        [[nodiscard]] uint32_t GetUsedIndices() const { return m_indexRanges.GetUsed(); }

        //! Release the staging buffers of recorded uploads, but the gpu has ti be done with them
        void FreeStagingBuffers();

        //! vtx and index buffers die with buffer manager and not here
        void Destroy();

    private:
        //! Stream helpers
        [[nodiscard]] static constexpr uint32_t StreamStride(EStream stream);
        [[nodiscard]] static constexpr const char* StreamName(EStream stream);

        //! Source pointer + byte size of one stream's data for a mesh, in EStream order
        struct StreamUpload {
            const void* data = nullptr;
            uint32_t size = 0;
        };

        RenderBackend* m_rhi = nullptr;
        RenderBackendInterface* m_backend = nullptr;
        BufferManager* m_buffers = nullptr;

        std::array<BufferHandle, StreamCount> m_streams{};
        BufferHandle m_indices{};

        BufferRangeAllocator m_vertexRanges;
        BufferRangeAllocator m_indexRanges;

        GenerationalPool<Mesh> m_pool;

        std::vector<Core::UniquePtr<Buffer>> m_usedStagingBuffers;
    };
}

#endif //SHIFT_MESHMANAGER_HPP
