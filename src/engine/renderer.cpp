#include "renderer.h"

auto enable_stencil_test() -> void {
    gl->enable(GL_STENCIL_TEST);
    gl->stencil_mask(0xFF);
    gl->clear(GL_STENCIL_BUFFER_BIT);
    gl->stencil_func(GL_ALWAYS, 1, 0xFF);
    gl->stencil_op(GL_KEEP, GL_KEEP, GL_REPLACE);
}

auto enable_outline() -> void {
    gl->stencil_func(GL_NOTEQUAL, 1, 0xFF);
    gl->stencil_mask(0x00);
    gl->disable(GL_DEPTH_TEST);
}

auto disable_stencil_test() -> void {
    gl->disable(GL_STENCIL_TEST);
}
