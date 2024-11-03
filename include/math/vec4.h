#ifndef H_VEC4
#define H_VEC4

#include <platform/types.h>
#include <iostream>

template<typename T>
struct TVec4 {
	union {
		struct {
			T x;
			T y;
			T z;
			T w;
		};
		T v[4];
	};
	inline TVec4<T>() : x((T)0), y((T)0), z((T)0), w((T)0) { }
	inline TVec4<T>(T _x, T _y, T _z, T _w) :
		x(_x), y(_y), z(_z), w(_w) { }
	inline explicit TVec4<T>(T* fv) :
		x(fv[0]), y(fv[1]), z(fv[2]), w(fv[3]) { }

  auto print() {
    std::cout << "x: " << x << ", y: " << y << ", z: " << z << ", w:" << w << std::endl;
  }
};

typedef TVec4<f32> vec4;
typedef TVec4<i32> ivec4;
typedef TVec4<u32> uivec4;

#endif
