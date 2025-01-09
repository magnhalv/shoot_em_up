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

* Spawn enemies
* Shoot enemies
* Explode enemies (play animation)
* Make enemies shoot

### GUI

* Window

### Cli

* Add separate color for input text
* Add auto-complete
* Add help
