//
// Created by otrush on 1/7/2026.
//

#ifndef SHIFT_EDITORCONTEXT_H
#define SHIFT_EDITORCONTEXT_H

#include <cstdint>
#include <memory>
#include <functional>

#include "Scene/NodeID.hpp"

namespace Shift::Editor {

    enum class EGizmoOperation : uint32_t {
        Translate = 0,
        Rotate,
        Scale,
        Universal
    };

    //! Context to be shared between UI panels
    struct EditorContext {
        void* EngineOutputTextureID = nullptr;
        std::function<void(uint32_t w, uint32_t h)> OnViewportResize;
        //! Pick node by ndc coords, x right y up
        std::function<NodeID(float ndcX, float ndcY)> PickNodeAt;
        //! Set by the tree panel or by a viewport click, read by the inspector
        NodeID selectedNode = NULL_NODE;
        EGizmoOperation gizmoOperation = EGizmoOperation::Translate;
    };
}

#endif //SHIFT_EDITORCONTEXT_H