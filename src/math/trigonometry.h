#include <platform/types.h>

auto inline radians(f32 degrees) -> f32 {
  return degrees * (PI / 180.0f);
}
