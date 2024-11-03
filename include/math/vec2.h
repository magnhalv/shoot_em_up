#ifndef VEC2_H
#define VEC2_H

#include <iostream>
#include <platform/types.h>

template <typename T> struct TVec2 {
  union {
    struct {
      T x;
      T y;
    };
    T v[2];
  };
  inline TVec2() : x(T(0)), y(T(0)) {
  }
  inline TVec2(T _x, T _y) : x(_x), y(_y) {
  }
  inline TVec2(T* fv) : x(fv[0]), y(fv[1]) {
  }

  auto print() -> void {
    std::cout << "x: " << x << "y: " << y << std::endl;
  }
};

typedef TVec2<f32> vec2;
typedef TVec2<i32> ivec2;

inline auto ivec2_to_vec2(ivec2 iv) -> vec2 {
  return vec2(static_cast<f32>(iv.x), static_cast<f32>(iv.y));
}

template <typename T> inline TVec2<T> operator+(const TVec2<T>& l, const TVec2<T>& r) {
  return { l.x + r.x, l.y + r.y };
}
#endif
