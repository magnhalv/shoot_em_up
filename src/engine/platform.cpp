#include "platform.h"

Platform *platform = nullptr;

auto load(Platform *in_platform) -> void {
    platform = in_platform;
}
