#pragma once
#include <cstdint>
#include <engine_types/handle/handle_type.h>

#if !defined(USE_WINDOWS) && defined(WIN32)
#define USE_WINDOWS
#endif

#if !defined(USE_X11) && defined(__linux__)
#define USE_X11
#endif

// Windowシステム別のネイティブハンドル定義

// Windows
#if defined(USE_WINDOWS)
#include <Windows.h>

namespace enishi::platform {
    struct NativeWindowsHandle {
        HWND hwnd;
        HINSTANCE hinstance;
    };
} // namespace enishi::platform

#else

namespace enishi::platform {
    struct NativeWindowsHandle {};
} // namespace enishi::platform

#endif // defined(USE_WINDOWS)

// Wayland
#if defined(USE_WAYLAND)

struct wl_display;
struct wl_surface;

namespace enishi::platform {
    struct NativeWaylandHandle {
        wl_display* display;
        wl_surface* surface;
    };
} // namespace enishi::platform

#else

namespace enishi::platform {
    struct NativeWaylandHandle {};
} // namespace enishi::platform

#endif // defined(USE_WAYLAND)

// X11
#if defined(USE_X11)
#include <X11/Xlib.h>

namespace enishi::platform {
    struct NativeX11Handle {
        Display* display;
        ::Window window; // Xlib の Window は unsigned long
    };
} // namespace enishi::platform

#else

namespace enishi::platform {
    struct NativeX11Handle {};
} // namespace enishi::platform

#endif // defined(USE_X11)

#if defined(__APPLE__)

#else

#endif

namespace enishi::platform {
    enum class WindowSystem : std::uint32_t {
        Windows,
        X11,
        Wayland,
    };

    union NativeWindowHandle {
        NativeWindowsHandle windows;
        NativeWaylandHandle wayland;
        NativeX11Handle x11;
    };

    struct WindowHandle {
        types::HandleId id;
        WindowSystem tag;
        NativeWindowHandle native_handle;
    };
} // namespace enishi::platform