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

### IMGUI

- Text rendering 
- Buttons 
- Sliders
- Window

### Features

- Support explosions <-------
- Support tags and animations
    - Support multiple tags
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
