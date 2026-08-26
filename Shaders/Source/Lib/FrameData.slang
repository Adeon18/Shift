//! Frame-scope data access.
//!
//! THis here is essentially the place where we pull data from buffers, since this might not be API agnostic, this
//! file's goal is to make it API agnostic.
//!
//! Lib/ modules are also the only shaders allowed to #include the shared header - that is what
//! keeps one struct definition serving both C++ and Slang.
#include "GPUShared.h"

[[vk::push_constant]] PushConstants pc;

FrameConstants LoadFrameConstants(GPUBufferRef frameConstantsRef)
{
    return *(FrameConstants*)frameConstantsRef;
}
