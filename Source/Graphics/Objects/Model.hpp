#ifndef SHIFT_MODEL_HPP
#define SHIFT_MODEL_HPP

#include "Graphics/Abstraction/Device/Device.hpp"
#include "Graphics/Abstraction/Commands/CommandPool.hpp"
#include "Graphics/Abstraction/Buffers/BasicBuffers.hpp"

#include "Mesh.hpp"

namespace sft::gfx {
    class Model {
    public:
        struct MeshRange
        {
            uint32_t vertexOffset; // offset in vertices
            uint32_t indexOffset; // offset in indices
            uint32_t vertexNum; // num of vertices
            uint32_t indexNum; // num of indices
        };

        [[nodiscard]] std::vector<Mesh>& GetMeshes() { return m_meshes; }
        [[nodiscard]] std::vector<MeshRange>& GetRanges() { return m_ranges; }
        [[nodiscard]] const VertexBuffer& GetVertexBufferPtr() { return *m_vertexBuffer; }
        [[nodiscard]] const IndexBuffer& GetIndexBufferRef() { return *m_indexBuffer; }

        //! Initializes the model GPU Vulkan buffer data with the CPU data stored in meshes
        void InitWithMeshData(const Device& m_device, const CommandBuffer& buffer);

    private:
        std::vector<Mesh> m_meshes;
        std::vector<MeshRange> m_ranges;
        std::unique_ptr<VertexBuffer> m_vertexBuffer;
        std::unique_ptr<IndexBuffer> m_indexBuffer;
    };
} // sft::gfx

#endif //SHIFT_MODEL_HPP
