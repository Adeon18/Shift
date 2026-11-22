#ifndef SHIFT_RHI_NOTIMPLEMENTED_HPP
#define SHIFT_RHI_NOTIMPLEMENTED_HPP

namespace Shift {
    //! Thrown by backend stubs only, essentially the fucntion name is thrown be it not implemented
    struct RHINotImplemented {
        const char* function;
    };
} // Shift

#define SRHI_NOT_IMPLEMENTED() throw ::Shift::RHINotImplemented{__FUNCTION__}

#endif //SHIFT_RHI_NOTIMPLEMENTED_HPP
