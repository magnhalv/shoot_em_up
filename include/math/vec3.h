#ifndef VEC3_H
#define VEC3_H

#define VEC3_EPSILON 0.000001f

#include <platform/types.h>

#include "vec4.h"

struct vec3 {
	union {
		struct {
			f32 x;
			f32 y;
			f32 z;
		};
		f32 v[3]{};
	};
	inline vec3() : x(0.0f), y(0.0f), z(0.0f) { }
	inline vec3(f32 _x, f32 _y, f32 _z) :
		x(_x), y(_y), z(_z) { }
	inline explicit vec3(f32* fv) :
		x(fv[0]), y(fv[1]), z(fv[2]) { }
};

vec3 operator+(const vec3& l, const vec3& r);
vec3 operator-(const vec3& l, const vec3& r);
vec3 operator*(const vec3& v, f32 f);
vec3 operator*(const vec3& l, const vec3& r);
f32 dot(const vec3& l, const vec3& r);
f32 lenSq(const vec3& v);
f32 len(const vec3& v);
void normalize(vec3& v);
vec3 normalized(const vec3& v);
f32 angle(const vec3& l, const vec3& r);
vec3 project(const vec3& a, const vec3& b);
vec3 reject(const vec3& a, const vec3& b);
vec3 reflect(const vec3& a, const vec3& b);
vec3 cross(const vec3& l, const vec3& r);
vec3 lerp(const vec3& start, const vec3& target, f32 t);
vec3 slerp(const vec3& s, const vec3& e, f32 t);
vec3 nlerp(const vec3& s, const vec3& e, f32 t);
bool operator==(const vec3& l, const vec3& r);
bool operator!=(const vec3& l, const vec3& r);

void print_vec3(const vec3 &v);

vec3 to_vec3(const vec4 &v);
vec4 to_vec4(const vec3 &v, f32 w = 1.0);

#endif