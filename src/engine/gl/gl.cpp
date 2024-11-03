#include "gl.h"

GLFunctions *gl = nullptr;

auto load_gl(GLFunctions *in_gl) -> void {
    gl = in_gl;
}