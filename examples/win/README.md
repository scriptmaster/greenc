# Windows fullscreen interfaces

Build with MinGW-w64 GCC and Make (for example, MSYS2 UCRT64), from the
repository root:

```sh
make -C examples/win/cpui
make -C examples/win/gpui
./examples/win/cpui/cpui.exe
./examples/win/gpui/gpui.exe
```

Run one example at a time. Escape or Alt+F4 closes the window. Alt+Tab switches
to another application. Both examples cover the primary monitor with a
borderless window; they do not change the display mode or take exclusive control.

- `cpui`: the CPU generates an animated 640x360 BGRX framebuffer. GDI scales
  and presents it to the fullscreen window. Windows may accelerate presentation;
  the pixel generation is entirely on the CPU.
- `gpui`: OpenGL draws a rotating, colored triangle through a double-buffered
  hardware context. It reports an error if only the generic software pixel
  format is available. Requires an installed OpenGL GPU driver. Uses the Windows
  OpenGL 1.1 API, so no third-party loader or graphics SDK is required.

Both `main.c` files use only portable types and `ui_` calls. `ui_window.h`
declares the interface; `ui_window_win32.c` implements windowing, timing, CPU
framebuffer allocation, and GDI presentation. `ui_gpu_win32.c` implements the
OpenGL renderer. `ui_window_win32.h` is private to these backends. The CPU
example has no OpenGL dependency. A future Linux backend can implement the same
interface; only Windows is implemented today.

Create a window, initialize its renderer, pump events and draw, then destroy the
renderer before the window. The GPU context borrows its window. A framebuffer
owns its pixels; initialize it from `{0}` and destroy it after use. Pixel format
is top-down `0x00RRGGBB`. All UI operations run on the creating thread. The event
loop currently supports one window per application; closing it ends that loop.
The GPU drawing call accepts caller-owned vertices, colors, rotation, and a
background, then clears, draws, and presents. The example defines an equilateral
triangle centered at its centroid. The renderer preserves equal coordinate
scales on both axes in landscape and portrait windows. It does not choose the
triangle's shape or colors. This is a minimal triangle primitive, not a complete
graphics drawing API.

These are native C examples built directly with GCC. They do not yet go
through GreenC's partial parser or borrow checker. Both release their rendering
resources before destroying the window.

## Frame pacing

Each example owns its setting near the start of `main.c`:

```c
const u32 target_fps = 240;
```

Change this to 128, 144, or another target and rebuild that example with its
normal Make command. The value is passed to `ui_window_create_fullscreen`;
no shared header edits or backend compile flags are needed. Passing zero opts
into `UI_DEFAULT_TARGET_FPS` (240) in the library header. Targets from 1 to
1000 FPS are accepted. Each window stores its own target.

Library API/backend files are architect-maintained; program settings and
behavior belong to the example developer. See [PRINCIPLES.md](../../PRINCIPLES.md)
for this ownership boundary.

Both examples call `ui_window_wait_frame` after rendering. The Windows backend
uses performance-counter deadlines and a high-resolution waitable timer,
subtracting time already spent drawing and carrying fractional ticks between
frames. Late frames reset the schedule rather than running catch-up frames.
Minimized windows use a separate low-power pause. The CPU color animation's
16 ms step controls its animation speed, not its rendering rate.

The high-resolution timer requires Windows 10 version 1803 or newer. The target
controls application pacing; it does not change the monitor's refresh rate or
guarantee vertical synchronization. Rendering cost, GPU driver settings, desktop
composition, and OS scheduling can limit the achieved rate.

Timer reference:
[Microsoft CreateWaitableTimerExW documentation](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-createwaitabletimerexw).

Pixel format reference:
[Microsoft PIXELFORMATDESCRIPTOR documentation](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-pixelformatdescriptor).
