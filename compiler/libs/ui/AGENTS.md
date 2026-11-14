# Stable UI architecture

Inherit the root AGENTS.md and PRINCIPLES.md. This directory is reusable
library code, not a place to customize any program.

After this explicitly requested foundation change, treat these contracts as
frozen. Only explicit architecture/library requests or requests to fix a named
backend defect authorize edits here. Program work uses public configuration,
input events, renderer operations, and runtime setters. Report an API gap
instead of silently changing the boundary.

- Public API: ui_window.h. No Win32/OpenGL types or application defaults.
- Window backend: native styles, event queue, timing, framebuffer allocation.
- GPU backend: context, texture/triangle presentation and driver failures.
- Common renderer: owned canvas, opaque drawing, GPU attempt and CPU fallback.
  Keep a disposable child surface for GPU pixel-format isolation; a failed
  OpenGL attempt must not invalidate the GDI parent surface.
- Do not add editor text models, file paths, key bindings, layouts, preferred
  FPS, themes or demo content here. File replacement belongs in sys/fs.
- All owned handles need failure-path cleanup. Rendering owns its window only
  by borrowing it. Destroy renderers before their parent window.
- Changes must update README.md, all example callers, and focused integration
  coverage. Verify successful GPU presentation and deterministic GPU failure
  recovery separately; selecting CPU explicitly only tests the CPU path.
