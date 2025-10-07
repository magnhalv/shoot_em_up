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

- Figure a better way to deal with textures and other resources when switching between renderers.
- Make record work again
- Add freeze
- Software renderer:
    - Make texture UV coordinates work
- Consider start using perspective matrices
- Load assets as background tasks
    - [ ] When hot reloading, wait for all current tasks to be completed.
- Make the window movable
- Make enemies shoot
- Make enemies move
- Blink if hit
- Three lives

### Done

- Hot switch between SW and OpenGL renderer
* Load assets as background tasks
    * [X] Load asset from asset file in a background task
    * [X] Add platform function for reading file and opening File handles
    * [X] Load sound from asset file
    * [ ] When hot reloading, wait for all current tasks to be completed.
* Move the renderer to a separate .dll
