//
// Created by otrush on 1/7/2026.
//

#ifndef SHIFT_EDITORCONTEXT_H
#define SHIFT_EDITORCONTEXT_H

#include <memory>
#include <functional>

#include "Scene/NodeID.hpp"

namespace Shift::Editor {

    //! Context to be shared between UI panels
    struct EditorContext {
        void* EngineOutputTextureID = nullptr;
        std::function<void(uint32_t w, uint32_t h)> OnViewportResize;
        //! Set by the tree panel, read by the inspector
        NodeID selectedNode = NULL_NODE;
    };
}

#endif //SHIFT_EDITORCONTEXT_H