#ifndef QUAT_H
#define QUAT_H

#include <stdio.h>

#include "vec3.h"
#include "vec4.h"
#include "mat4.h"

#define QUAT_EPSILON 0.000001f

struct quat {
    union {
        struct {
            float x;
            float y;
            float z;
            float w;
        };
        struct {
            vec3 vector;
            float scalar;
        };
        float v[4];
    };

    inline quat() : x(0), y(0), z(0), w(1) { }
    inline quat(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) { }
};

quat fromTo(const vec3& from, const vec3& to);
vec3 getAxis(const quat& quat);
float getAngle(const quat& quat);
quat angle_axis(float angle, const vec3& axis);

quat operator+(const quat& a, const quat&b);
quat operator-(const quat& a, const quat&b);
quat operator*(const quat &a, float b);
quat operator*(const quat &a, const quat& b);
vec3 operator*(const quat &q, const vec3& v);
quat operator-(const quat& q);
bool operator==(const quat& a, const quat& b);
bool operator!=(const quat& a, const quat& b);
quat operator^(const quat& q, float f);

bool sameOrientation(const quat& a, const quat& b);
float dot(const quat& a, const quat& b);
float lenSq(const quat& q);
float len(const quat& q);

void normalize(quat& q);
quat normalized(const quat& q);
quat conjugate(const quat& q);
quat inverse(const quat& q);

quat mix(const quat& from, const quat& to, float dt);
quat nlerp(const quat& from, const quat& to, float dt);
quat slerp(const quat& q0, const quat& q1, float dt);

quat lookRotation(const vec3& direction, const vec3& up);

mat4 quatToMat4(const quat& q);
quat mat4ToQuat(const mat4& m);

void print(const quat& to);

#endif
