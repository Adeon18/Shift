#ifndef SHIFT_MESH_HPP
#define SHIFT_MESH_HPP

#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "Utility/GUIDGenerator/GUIDGenerator.hpp"
#include "VertexStructures.hpp"

namespace shift::gfx {
    enum class MeshTextureType {
        Diffuse,
        NormalMap,
        MetallicRoughness
    };

    class Mesh {
    public:
        struct Triangle {
            uint16_t indices[3];
        };
        std::vector<gfx::Vertex> vertices;
        std::vector<Triangle> triangles;

        std::unordered_map<MeshTextureType, SGUID> textures;

        glm::mat4 meshToModel;
        glm::mat4 meshToModelInv;
    };
} // shift::gfx

#endif //SHIFT_MESH_HPP
