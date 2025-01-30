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
