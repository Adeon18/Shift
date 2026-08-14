//
// Created by otrush on 8/14/2026.
//

#ifndef SHIFT_GLOBALRESOURCESET_HPP
#define SHIFT_GLOBALRESOURCESET_HPP

#include "Graphics/RHI/RHI.hpp"

namespace Shift::Graphics {

    //! Set 0: the bindless set every pipeline shares
    class GlobalResourceSet {
    public:
        static constexpr uint32_t SET_INDEX = 0;

        //! 2 / 3 / 4 are RESERVED for Cube / 3D / 2DArray sampled-image arrays
        static constexpr uint32_t BINDING_SAMPLERS  = 0;
        static constexpr uint32_t BINDING_IMAGES_2D = 1;
        // static constexpr uint32_t BINDING_IMAGES_CUBE    = 2;
        // static constexpr uint32_t BINDING_IMAGES_3D      = 3;
        // static constexpr uint32_t BINDING_IMAGES_2DARRAY = 4;

        [[nodiscard]] static const PipelineLayoutDescriptor& Layout();

        [[nodiscard]] bool Init(RenderBackendInterface* backend);
        void Destroy();

        //! Point a slot of the 2D image array at a texture. slotIdx of a TextureHandle IS this index
        void WriteImage2D(uint32_t slot, const Texture& texture);
        void WriteSampler(uint32_t slot, const Sampler& sampler);
        //! Apply the chnages
        void Apply();

        [[nodiscard]] ResourceSet* Get() const { return m_set; }

    private:
        ResourceSet* m_set = nullptr;
    };
}

#endif //SHIFT_GLOBALRESOURCESET_HPP
