#ifndef SHIFT_DX12PIPELINE_HPP
#define SHIFT_DX12PIPELINE_HPP

#include "DX12Device.hpp"
#include "../Common/Pipeline.hpp"

namespace Shift::DX12 {
    class Pipeline {
    public:
        [[nodiscard]] bool IsValid() const;
        [[nodiscard]] const PipelineDescriptor& GetDescriptor() const;

        ~Pipeline();
    };

    ASSERT_INTERFACE(IPipeline, Pipeline);
} // Shift::DX12

#endif //SHIFT_DX12PIPELINE_HPP
