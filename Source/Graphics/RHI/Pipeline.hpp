//
// Created by otrush on 10/5/2024.
//

#ifndef SHIFT_PIPELINE_HPP
#define SHIFT_PIPELINE_HPP

#include <concepts>
#include <type_traits>
#include <vector>

#include "Base.hpp"
#include "Types.hpp"
#include "Shader.hpp"
#include "TextureFormat.hpp"

namespace Shift {
    //! Enum for stencil operations, 1:1 with Vulkan
    enum class EStencilOp : uint8_t {
        Keep = 0,
        Zero = 1,
        Replace = 2,
        IncrementClamp = 3,
        DecrementClamp = 4,
        Invert = 5,
        IncrementWrap = 6,
        DecrementWrap = 7,
    };

    //! 1:1 with Vulkan
    enum class EPolygonMode : uint8_t {
        Fill, Line, Point,
    };

    //! 1:1 with Vulkan, we ignore bit operators here
    enum class ECullMode : uint8_t {
        None = 0,
        Front = 0b01,
        Back = 0b10,
        Both = Front | Back
    };

    //! 1:1 with Vulkan
    enum class EWindingOrder : uint8_t {
        CounterClockwise,
        Clockwise
    };

    //! 1:1 with Vulkan, needed for blend attachment state
    enum class EColorWriteMask : uint8_t {
        Red = 1 << 0,
        Green = 1 << 1,
        Blue = 1 << 2,
        Alpha = 1 << 3,
        RGB = Red | Green | Blue,
        RGBA = RGB | Alpha
    };

    //! Guess what? 1:1 with Vulkan too!
    enum class EBlendFactor : uint8_t {
        Zero                    = 0,
        One                     = 1,
        SourceColor             = 2,
        OneMinusSourceColor     = 3,
        DestColor               = 4,
        OneMinusDestColor       = 5,
        SourceAlpha             = 6,
        OneMinusSourceAlpha     = 7,
        DestAlpha               = 8,
        OneMinusDestAlpha       = 9,
        ConstantColor           = 10,
        OneMinusConstantColor   = 11,
        ConstantAlpha           = 12,
        OneMinusConstantAlpha   = 13,
        SourceAlphaSaturate     = 14,
        Source1Color            = 15,
        OneMinusSource1Color    = 16,
        Source1Alpha            = 17,
        OneMinusSource1Alpha    = 18
    };

    //! 1:1 with Vulkan, except skip after Max
    enum class EBlendOperation : uint8_t {
        Add,
        Subtract,
        ReverseSubtract,
        Min,
        Max
    };

    //! 1:1 with Vulkan
    enum class EVertexInputRate : uint8_t {
        PerVertex, PerInstance
    };

    //! There are supposed to be 1:1 with Vulkan, but I will add them as I go
    enum class EVertexAttributeFormat {
        R32G32_SignedFloat = 103,
        R32G32B32_SignedFloat = 106,
        R32G32B32A32_SignedFloat = 109,
    };

    //! 1:1 with Vulkan
    enum class EPrimitiveTopology : uint8_t {
        PointList,
        LineList,
        LineStrip,
        TriangleList,
        TriangleStrip,
        TriangleFan,
        LineListAdjacency,
        LineStripAdjacency,
        TriangleListAdjacency,
        TriangleStripAdjacency,
        PatchList
    };

    //! 1:1 with Vulkan
    enum class EBindingType : uint8_t {
        Sampler,
        CombinedImageSampler,
        SampledImage,
        StorageImage,
        UniformTexelBuffer,
        StorageTexelBuffer,
        UniformBuffer,
        StorageBuffer,
        UniformBufferDynamic,
        StorageBufferDynamic,
        InputAttachment
    };

    //! 1:1 with Vulkan
    enum class EBindingVisibility {
        Vertex = 1 << 0,
        TesselationControl = 1 << 1,
        TesselationEvaluation = 1 << 2,
        Geometry = 1 << 3,
        Fragment = 1 << 4,
        Compute = 1 << 5,
//        //! Shift custom
//        VertexFragment = Vertex | Fragment,
//        AllGraphics = Vertex | TesselationControl | TesselationEvaluation | Geometry | Fragment,
//        All = Vertex | TesselationControl | TesselationEvaluation | Geometry | Fragment | Compute
    };
    DEFINE_ENUM_CLASS_BITWISE_OPERATORS(EBindingVisibility)

    //! The pipeline layout offline description structure
    struct PipelineLayoutDescriptor {
        struct LayoutBindingDesc {
            //! Main
            uint32_t binding = 0;
            EBindingType type = EBindingType::Sampler;
            EBindingVisibility stageFlags = EBindingVisibility::Vertex | EBindingVisibility::Fragment;
            //! Secondary
            uint32_t count = 1;
            bool isBindless = false;
            bool writable = false;
        };
        std::vector<LayoutBindingDesc> bindings;

        //! FEATURE: Add push constants support
    };

    //! The pipeline offline description structures (default values for all except viewport and scissor)
    struct PipelineDescriptor {
        std::vector<ShaderStageDesc> shaders;

        //! Batched vertex info
        struct VertexConfig {
            struct VertexBindingDesc {
                uint32_t binding = 0u;
                uint32_t stride = 0u;
                EVertexInputRate inputRate = EVertexInputRate::PerVertex;
            };
            std::vector<VertexBindingDesc> vertexBindings;

            struct VertexAttributeDesc {
                uint32_t location = 0u;
                uint32_t binding = 0u;
                uint32_t offset = 0u;
                EVertexAttributeFormat format = EVertexAttributeFormat::R32G32B32_SignedFloat;
            };
            std::vector<VertexAttributeDesc> attributeDescs;

        } vertexConfig;

        //! Input assembly: TriangleList by default
        EPrimitiveTopology topology = EPrimitiveTopology::TriangleList;

        //! Viewport info(most likely unused in Vulkan RHI due to me using a dynamic one
        struct ViewportDesc {
            float x, y, width, height, minDepth, maxDepth;
        } viewport;

        //! Scissor info(most likely unused in Vulkan RHI due to me using a dynamic one
        struct ScissorDesc {
            struct {
                int x;
                int y;
            } offset;
            struct {
                uint32_t x;
                uint32_t y;
            } extent;
        } scissor;

        //! Rasterizer config
        struct RasterizerStateDesc {
            bool depthClampEnable : 1 = false;
            bool rasterizerDiscardEnable : 1 = false;

            EPolygonMode polygoneMode = EPolygonMode::Fill;
            ECullMode cullMode = ECullMode::Back;
            EWindingOrder windingOrder = EWindingOrder::CounterClockwise;
            float lineWidth = 1.0f; // next-gen

            struct DepthBias {
                float clamp = 0.0f, constantFactor = 0.0f, slopeFactor = 0.0f;
                bool enable = false;
            } depthBias;
        } rasterizerStateDesc;

        struct MultisampleDesc {
            // TODO: Support multisampling
        } multisampleDesc;

        //! Entire color blend configuration, simular to Vulkan
        struct ColorBlendConfig {
            //! 1:1 with Vulkan
            enum class ELogicalOperation : uint8_t {
                Clear, AND, ANDReverse, Copy, ANDInverted, Noop, XOR, OR, NOR,
                Equivalent, Invert, ORReverse, CopyInverted, ORInverted, NAND, Set
            } logicalOperation = ELogicalOperation::Copy;

            bool logicalOpEnabled = false;
            struct ColorAttachmentConfig {
                bool blendEnabled = false;
                EColorWriteMask colorWriteMask = EColorWriteMask::RGBA;

                EBlendFactor sourceColorBlendFactor = EBlendFactor::One;
                EBlendFactor destinationColorBlendFactor = EBlendFactor::Zero;
                EBlendOperation colorBlendOperation = EBlendOperation::Add;

                EBlendFactor sourceAlphaBlendFactor = EBlendFactor::One;
                EBlendFactor destinationAlphaBlendFactor = EBlendFactor::Zero;
                EBlendOperation alphaBlendOperation = EBlendOperation::Add;

                ETextureFormat format = ETextureFormat::UNDEFINED;
            };
            std::vector<ColorAttachmentConfig> attachments;

            float blendConstants[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        } colorBlendConfig;

        //! Depth stencil configuration, similar to Vulakn as well
        struct DepthStencilConfig {
            ETextureFormat depthFormat = ETextureFormat::UNDEFINED;
            ETextureFormat stencilFormat = ETextureFormat::UNDEFINED;
            bool depthTestEnabled = false;
            bool depthWriteEnabled = false;
            ECompareOperation depthFunction = ECompareOperation::Never;

            // TODO: Add support for stencil and depth bounds 🥺
            //bool stencilTestEnabled = false;
        } depthStencilConfig;

        //! These "virtual" layouts will get picked up at pipeline creation by a
        //! DescriptorManager/PipelineLayoutCache and get either created or pulled from cache
        std::vector<PipelineLayoutDescriptor> descriptorLayouts;
    };

    template<typename Pipeline>
    concept IPipeline =
        std::is_default_constructible_v<Pipeline> &&
        std::is_destructible_v<Pipeline> &&
    requires(Pipeline InputPipeline, const PipelineDescriptor& Descriptor) {
        { InputPipeline.Destroy() } -> std::same_as<void>;
        { CONCEPT_CONST_VAR(Pipeline, InputPipeline).GetDescriptor() } -> std::same_as<const PipelineDescriptor&>;
    };
} // Shift

#endif //SHIFT_PIPELINE_HPP
