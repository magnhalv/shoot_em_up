#ifndef MAT4_H
#define MAT4_H

#include <stdio.h>

#include <platform/types.h>

#include "vec3.h"
#include "vec4.h"

#define MAT4_EPSILON 0.000001f

struct mat4 {
	union {
		f32 v[16]{};
		struct {
			vec4 right;
			vec4 up;
			vec4 forward;
			vec4 position;
		};
		struct {
			//            row 1     row 2     row 3     row 4
			/* column 1 */f32 xx; f32 xy; f32 xz; f32 xw;
			/* column 2 */f32 yx; f32 yy; f32 yz; f32 yw;
			/* column 3 */f32 zx; f32 zy; f32 zz; f32 zw;
			/* column 4 */f32 tx; f32 ty; f32 tz; f32 tw;
		};
		struct {
			f32 c0r0; f32 c0r1; f32 c0r2; f32 c0r3;
			f32 c1r0; f32 c1r1; f32 c1r2; f32 c1r3;
			f32 c2r0; f32 c2r1; f32 c2r2; f32 c2r3;
			f32 c3r0; f32 c3r1; f32 c3r2; f32 c3r3;
		};
		struct {
			f32 r0c0; f32 r1c0; f32 r2c0; f32 r3c0;
			f32 r0c1; f32 r1c1; f32 r2c1; f32 r3c1;
			f32 r0c2; f32 r1c2; f32 r2c2; f32 r3c2;
			f32 r0c3; f32 r1c3; f32 r2c3; f32 r3c3;
		};
	};
	// Include constructors here

	inline mat4() :
		xx(1), xy(0), xz(0), xw(0),
		yx(0), yy(1), yz(0), yw(0),
		zx(0), zy(0), zz(1), zw(0),
		tx(0), ty(0), tz(0), tw(1) {}

	inline mat4(f32* fv) :
		xx(fv[0]), xy(fv[1]), xz(fv[2]), xw(fv[3]),
		yx(fv[4]), yy(fv[5]), yz(fv[6]), yw(fv[7]),
		zx(fv[8]), zy(fv[9]), zz(fv[10]), zw(fv[11]),
		tx(fv[12]), ty(fv[13]), tz(fv[14]), tw(fv[15]) { }

	inline mat4(
		f32 _00, f32 _01, f32 _02, f32 _03,
		f32 _10, f32 _11, f32 _12, f32 _13,
		f32 _20, f32 _21, f32 _22, f32 _23,
		f32 _30, f32 _31, f32 _32, f32 _33) :
		xx(_00), xy(_01), xz(_02), xw(_03),
		yx(_10), yy(_11), yz(_12), yw(_13),
		zx(_20), zy(_21), zz(_22), zw(_23),
		tx(_30), ty(_31), tz(_32), tw(_33) { }
}; // end mat4 struct


bool operator==(const mat4& a, const mat4& b);
bool operator!=(const mat4& a, const mat4& b);
mat4 operator*(const mat4& m, f32 f);
mat4 operator+(const mat4& a, const mat4& b);
mat4 operator*(const mat4& a, const mat4& b);
vec4 operator*(const mat4& m, const vec4& v);
vec3 transform_vector(const mat4& m, const vec3& v);
vec3 transform_point(const mat4& m, const vec3& v);
vec3 transformPoint(const mat4& m, const vec3& v, f32& w);
void transpose(mat4& m);
mat4 transposed(const mat4& m);
f32 determinant(const mat4& m);
mat4 adjugate(const mat4& m);
mat4 inverse(const mat4& m);
void invert(mat4& m);
mat4 frustum(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f);
mat4 perspective(f32 fov_degrees, f32 aspect, f32 znear, f32 zfar);
mat4 create_ortho(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f);
mat4 lookAt(const vec3& position, const vec3& target, const vec3& up);

void print(const mat4& m);
#endif