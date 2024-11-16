# Hot reload OpenGL


## Build

This project is built using cmake. Recommended is using ninja and git bash. Example below using visual studio build tools: 

```
cmake -S . -B cmake-build -G "Ninja"
cmake --build cmake-build
cmake --build cmake-build --target ShootEmUp|engine_dyn|tests>
cmake --build cmake-build && ./cmake-build/shoot_em_up.exe
```

An example project for how to set up OpenGl with hot reloading.

## TODO 

* Add steady fps support
* Test playing sound while holding space
* Play laser sound effect when "shooting"

### GUI

* Window

### Cli

* Add separate color for input text
* Add auto-complete
* Add help
