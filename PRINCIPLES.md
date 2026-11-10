# GreenC engineering principles

## Types express intent

Use semantic or explicitly sized types for new application and library data,
in the spirit of MISRA C's emphasis on explicit types and conversions. This is
a project discipline, not a claim of MISRA compliance.

The canonical aliases live in
`compiler/libs/sys/builtin/types_intrinsic.h`. Reuse them instead of defining
competing aliases in examples or platform backends.

- Use `u8`, `u16`, `u32`, `u64` and signed `i8`, `i16`, `i32`, `i64` when a
  value's width matters. Use `u8` for a color channel and `u32` for a packed
  32-bit pixel. Avoid bare `unsigned`, `signed`, `short`, `long`, or `int` for
  application data.
- Use `size_t` for object sizes, allocation sizes, element counts, and indexes.
  Do not substitute `u64` merely because the current target is 64-bit.
- Preserve `usize` / `isize` in existing syscall/runtime interfaces. Their
  current definitions are 64-bit; do not assume they are pointer-sized on every
  target or silently change their ABI.
- Use `f32` / `f64` for application floating-point data, `bool` from
  `<stdbool.h>` for predicates, and descriptive enums for distinct states.
  Use character types for text, not numeric byte storage.
- Choose signedness from the value's meaning, not a generic preference for
  unsigned arithmetic. Check bounds before narrowing or multiplying sizes.
  Make intended wrapping or truncation explicit; a cast alone is not a check.
- Keep required ABI types at boundaries: `int main(void)`, Win32 callback
  signatures and native API results, and graphics API types must retain their
  required types. Validate and convert them inside the backend. Do not leak
  platform types into portable interfaces.

Legacy code does not override these rules for new code. Apply improvements in
the touched scope rather than performing unrelated repository-wide rewrites.

## Reviewability and portability

Follow the concrete naming, control-flow spacing, ownership, and review rules
in `AGENTS.md`. Example application code should describe the application;
platform mechanisms belong behind the `ui_` interface. Review the resulting
code without relying on the chat to explain it.

## Library architecture and application customization

Treat code as two areas with distinct responsibilities:

- **Architect-maintained library:** public contracts, reusable mechanisms,
  sensible fallback defaults, resource management, and platform encapsulation.
  Keep these generic and independent of an individual program's preferences.
  Here that means `ui_window.h`, the private backend header, and `*_win32.c`.
- **Developer/user-maintained program:** application settings, content, and
  behavior. Here that means each example's `main.c`. Put its desired frame rate,
  title, framebuffer size, and animation choices here and pass settings through
  the public API. One program's choices must not alter another program.

Geometry and colors are application content as well. A renderer accepts
vertices and colors and preserves coordinate proportions; the program chooses
whether a triangle is equilateral, its size, and where it rotates. Test those
mathematical properties, not just whether the graphics API accepts the calls.

For example, a program declares `const u32 target_fps = 144;` and passes it to
`ui_window_create_fullscreen(title, target_fps)`. Passing zero explicitly opts
into the library's `UI_DEFAULT_TARGET_FPS` fallback. Editing the library's
default is an architectural change, not the normal customization workflow.

Expose needed configuration through parameters or configuration structures.
Keep platform validation and implementation inside the library. Avoid hiding
application-specific policy in shared code or relying on a macro in `main.c`
to configure a separately compiled backend. Review new changes against this
boundary as well as naming and style.
