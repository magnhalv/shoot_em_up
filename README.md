# Shoot em UP

## Build

### CMAKE + Ninja

```
cmake -S . -B cmake-build -G "Ninja"
cmake --build cmake-build
cmake --build cmake-build --target ShootEmUp|engine_dyn|tests>
cmake --build cmake-build && ./cmake-build/shoot_em_up.exe
```

## TODO 

### Optimizing software renderer

- SIMD (SSE/AVX).
- Eliminate per-pixel inverse model transform

  Lines 301-307: You compute inv_model * (screen_point - translation) for every pixel inside the quad. Instead, precompute
  the UV gradient per scanline. You can calculate du/dx, du/dy, dv/dx, dv/dy once before the loop, then just increment U and
   V as you step across pixels. This replaces a matrix multiply per pixel with an add per pixel.

- Remove redundant bounds checks inside the clipped loop

  Lines 281-286: You check screen_point.x < 0 || screen_point.x >= buffer->width etc., but you already clipped
  min_x/max_x/min_y/max_y to the clip rect (lines 249-252). If your clip rects are correct, these checks are dead code
  burning cycles every pixel.

- Avoid the per-pixel dot-product quad test for axis-aligned quads

  Lines 288-293: You do 4 dot products per pixel to test if the point is inside the quad. For unrotated quads (rotation ==
  0), you can skip this entirely since the bounding box is the quad. A fast path for rotation == 0.0f would eliminate
  significant work for the common case.

- Use a lookup table for sRGB <-> linear conversion

  srgb255_to_linear1 and linear1_to_srgb255 call square and sqrtf per channel (lines 121-145). A 256-entry LUT for
  sRGB-to-linear and a 256-entry LUT for linear-to-sRGB would replace sqrtf with a table lookup. Even with SIMD, sqrtf is
  expensive.

- Eliminate the Y-flip copy in end_frame

  Lines 723-728: You allocate a full-screen buffer and copy every row just to flip Y. Instead, set biHeight to a positive
  value in BITMAPINFO (line 82 — you currently negate it), which makes GDI interpret the bitmap as bottom-up natively. Then
  render with Y=0 at the bottom. This saves a full-frame memcpy every frame.

### IMGUI

- Optimize software renderer.
- Decide how how to handle and pack colors. Should probably be 1.0, not 255.0
- Text rendering 
- Buttons 
- Sliders
- Window

### Features

- Support background
    - Scrolling background
- Support tags and animations
    - Support multiple tags (speed forward)
- CLI
- Mix background music and other sounds
- Background texture moving
- Make a slider through the level
- Make an editor (i.e. being able to add enemies at certain points)

- Consider start using perspective matrices
- Load assets as background tasks
    - [ ] When hot reloading, wait for all current tasks to be completed.
- Make the window movable
- Make enemies shoot
- Make enemies move
- Blink if hit
- Three lives

### Core

- Make matrix row major
- Support matrix operations using SIMD
- Hashmap

### Build system

- Unity build
- Remove all references to C++ stdlib

### Compelted features

- Hot reload Engine
- Record and replay
- Hot switch between SW and OpenGL renderer
- Load assets as background tasks
