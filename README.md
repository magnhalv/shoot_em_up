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

* Load assets as background tasks
    * Load asset from asset file in a background task (Done)
    * Add platform function for reading file and opening File handles (Done)
    * Load sound from asset file
    * When hot reloading, wait for all current tasks to be completed.
* Make enemies shoot
* Make enemies move
* Blink if hit
* Three lives

### GUI

* Window

### Cli

* Add separate color for input text
* Add auto-complete
* Add help
