[working-directory: 'data']
assets-build: build
  time ../cmake-build/asset_builder.exe
  
gen: env
  ./scripts/generate.bat

env:
  ./scripts/env.bat

build: env
  time ./scripts/build.bat

test: env
  cmake --build cmake-build --target tests
  ./cmake-build/tests/tests.exe


engine:
  { time ./scripts/compile.bat engine; } > engine.log 2>&1

main:
  { time ./scripts/compile.bat main; } > main.log 2>&1

software_renderer:
  { time ./scripts/compile.bat software_renderer; }  > software_renderer.log 2>&1

opengl_renderer:
  { time ./scripts/compile.bat opengl_renderer; }  > opengl_renderer.log 2>&1

[parallel]
build-all: engine main software_renderer opengl_renderer
    cat main.log
    cat engine.log
    cat software_renderer.log
    cat opengl_renderer.log

[working-directory: 'data']
run: 
  time just build-all
  ../bin/app/shoot_em_up.exe

