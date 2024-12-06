#ifndef SHIFT_VERTEXSTRUCTURES_HPP
#define SHIFT_VERTEXSTRUCTURES_HPP

#include <vector>
#include <glm/glm.hpp>

#include "Graphics/RHI/Pipeline.hpp"

namespace Shift {
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec3 normal;
        glm::vec3 tangent;
        glm::vec3 bitangent;
        glm::vec2 texCoord;

        [[nodiscard]] static PipelineDescriptor::VertexConfig GetVertexConfig() {
            PipelineDescriptor::VertexConfig conf;
            conf.vertexBindings.emplace_back(0u, sizeof(Vertex), EVertexInputRate::PerVertex);

            //! loc, bind, offset, format
            conf.attributeDescs.emplace_back(0u, 0u, offsetof(Vertex, pos), EVertexAttributeFormat::R32G32B32_SignedFloat);
            conf.attributeDescs.emplace_back(1u, 0u, offsetof(Vertex, color), EVertexAttributeFormat::R32G32B32_SignedFloat);
            conf.attributeDescs.emplace_back(2u, 0u, offsetof(Vertex, texCoord), EVertexAttributeFormat::R32G32_SignedFloat);
            conf.attributeDescs.emplace_back(3u, 0u, offsetof(Vertex, normal), EVertexAttributeFormat::R32G32B32_SignedFloat);
            conf.attributeDescs.emplace_back(4u, 0u, offsetof(Vertex, tangent), EVertexAttributeFormat::R32G32B32_SignedFloat);
            conf.attributeDescs.emplace_back(5u, 0u, offsetof(Vertex, bitangent), EVertexAttributeFormat::R32G32B32_SignedFloat);

            return conf;
        }
    };
} // Shift

#endif // SHIFT_VERTEXSTRUCTURES_HPP
