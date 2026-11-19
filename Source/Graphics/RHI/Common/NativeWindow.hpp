#ifndef SHIFT_NATIVEWINDOW_HPP
#define SHIFT_NATIVEWINDOW_HPP

#include <cstdint>

//! Kinda API Agnostic wrapper for window
namespace Shift {
    enum class EPlatform : uint8_t {
        None,
        Win32,
    };

    //! Win32: window = HWND, instance = HINSTANCE
    struct NativeWindow {
        EPlatform platform = EPlatform::None;
        void* window = nullptr;
        void* instance = nullptr;
    };
} // Shift

#endif //SHIFT_NATIVEWINDOW_HPP
