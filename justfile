run: build
  ./cmake-build/shoot_em_up.exe
  
gen: env
  ./scripts/generate.bat

env:
  ./scripts/env.bat

build: env
  ./scripts/build.bat

test: env
  cmake --build cmake-build --target tests
  ./cmake-build/tests/tests.exe
