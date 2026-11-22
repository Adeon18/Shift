#include "DX12Pipeline.hpp"

#include "../Common/NotImplemented.hpp"

namespace Shift::DX12 {
    bool Pipeline::IsValid() const { SRHI_NOT_IMPLEMENTED(); }
    const PipelineDescriptor& Pipeline::GetDescriptor() const { SRHI_NOT_IMPLEMENTED(); }

    Pipeline::~Pipeline() = default;
} // Shift::DX12
