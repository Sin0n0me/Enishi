#pragma once
#include <cstdint>
#include <engine_types/handle/handle_type.h>

struct wl_display;
struct wl_surface;

namespace enishi::platform {
    // Borrowed native handles. OS-specific conversion belongs at the implementation boundary.
    struct NativeWindowsHandle {
        void* hwnd;
        void* hinstance;
    };
    struct NativeWaylandHandle {
        wl_display* display;
        wl_surface* surface;
    };
    struct NativeX11Handle {
        void* display;
        unsigned long window;
    };
    struct NativeCocoaHandle {
        void* window;
    };
} // namespace enishi::platform

namespace enishi::platform {
    enum class WindowSystem : std::uint32_t {
        Windows,
        X11,
        Wayland,
        Cocoa,
        Automatic,
    };

    union NativeWindowHandle {
        NativeWindowsHandle windows;
        NativeWaylandHandle wayland;
        NativeX11Handle x11;
        NativeCocoaHandle cocoa;
    };

    struct WindowHandle {
        types::HandleId id;
        WindowSystem tag;
        NativeWindowHandle native_handle;
    };
} // namespace enishi::platform
