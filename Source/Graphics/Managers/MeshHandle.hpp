//
// Created by otrush on 9/18/2026.
//

#ifndef SHIFT_MESHHANDLE_HPP
#define SHIFT_MESHHANDLE_HPP

#include "GenerationalPool.hpp"

namespace Shift::Graphics {
    struct Mesh;

    //! This is serapare to disconnect scene from mesh manager
    using MeshHandle = GenerationalPool<Mesh>::Handle;
}

#endif //SHIFT_MESHHANDLE_HPP
