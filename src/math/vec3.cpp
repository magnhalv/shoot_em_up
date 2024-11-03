#include <cmath>

#include <math/vec3.h>

vec3 operator+(const vec3 &l, const vec3 &r) {
    return {l.x + r.x, l.y + r.y, l.z + r.z};
}

vec3 operator-(const vec3 &l, const vec3 &r) {
    return {l.x - r.x, l.y - r.y, l.z - r.z};
}

vec3 operator*(const vec3 &v, f32 f) {
    return {v.x * f, v.y * f, v.z * f};
}

vec3 operator*(const vec3 &l, const vec3 &r) {
    return {l.x * r.x, l.y * r.y, l.z * r.z};
}

f32 dot(const vec3 &l, const vec3 &r) {
    return l.x * r.x + l.y * r.y + l.z * r.z;
}

f32 lenSq(const vec3 &v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

f32 len(const vec3 &v) {
    f32 lenSq = v.x * v.x + v.y * v.y + v.z * v.z;
    if (lenSq < VEC3_EPSILON) {
        return 0.0f;
    }
    return sqrtf(lenSq);
}

void normalize(vec3 &v) {
    f32 lenSq = v.x * v.x + v.y * v.y + v.z * v.z;
    if (lenSq < VEC3_EPSILON) {
        return;
    }
    f32 invLen = 1.0f / sqrtf(lenSq);

    v.x *= invLen;
    v.y *= invLen;
    v.z *= invLen;
}

vec3 normalized(const vec3 &v) {
    f32 lenSq = v.x * v.x + v.y * v.y + v.z * v.z;
    if (lenSq < VEC3_EPSILON) {
        return v;
    }
    f32 invLen = 1.0f / sqrtf(lenSq);

    return {
            v.x * invLen,
            v.y * invLen,
            v.z * invLen
    };
}

f32 angle(const vec3 &l, const vec3 &r) {
    f32 sqMagL = l.x * l.x + l.y * l.y + l.z * l.z;
    f32 sqMagR = r.x * r.x + r.y * r.y + r.z * r.z;

    if (sqMagL < VEC3_EPSILON || sqMagR < VEC3_EPSILON) {
        return 0.0f;
    }

    f32 dot = l.x * r.x + l.y * r.y + l.z * r.z;
    f32 len = sqrtf(sqMagL) * sqrtf(sqMagR);
    return acosf(dot / len);
}

vec3 project(const vec3 &a, const vec3 &b) {
    f32 magBSq = len(b);
    if (magBSq < VEC3_EPSILON) {
        return {};
    }
    f32 scale = dot(a, b) / magBSq;
    return b * scale;
}

vec3 reject(const vec3 &a, const vec3 &b) {
    vec3 projection = project(a, b);
    return a - projection;
}

vec3 reflect(const vec3 &a, const vec3 &b) {
    f32 magBSq = len(b);
    if (magBSq < VEC3_EPSILON) {
        return {};
    }
    f32 scale = dot(a, b) / magBSq;
    vec3 proj2 = b * (scale * 2);
    return a - proj2;
}

vec3 cross(const vec3 &l, const vec3 &r) {
    return {
            l.y * r.z - l.z * r.y,
            l.z * r.x - l.x * r.z,
            l.x * r.y - l.y * r.x
    };
}

vec3 lerp(const vec3 &start, const vec3 &target, f32 t) {
    if (t > 1.0f){
        t = 1.0f;
    }
    return {
            start.x + (target.x - start.x) * t,
            start.y + (target.y - start.y) * t,
            start.z + (target.z - start.z) * t
    };
}

vec3 slerp(const vec3 &s, const vec3 &e, f32 t) {
    if (t < 0.01f) {
        return lerp(s, e, t);
    }

    vec3 from = normalized(s);
    vec3 to = normalized(e);

    f32 theta = angle(from, to);
    f32 sin_theta = sinf(theta);

    f32 a = sinf((1.0f - t) * theta) / sin_theta;
    f32 b = sinf(t * theta) / sin_theta;

    return from * a + to * b;
}

vec3 nlerp(const vec3 &s, const vec3 &e, f32 t) {
    vec3 linear(
            s.x + (e.x - s.x) * t,
            s.y + (e.y - s.y) * t,
            s.z + (e.z - s.z) * t
    );
    return normalized(linear);
}

bool operator==(const vec3 &l, const vec3 &r) {
    vec3 diff(l - r);
    return lenSq(diff) < VEC3_EPSILON;
}

bool operator!=(const vec3 &l, const vec3 &r) {
    return !(l == r);
}

void print_vec3(const vec3 &v) {
    printf("x: %f, y: %f, z: %f\n", v.x, v.y, v.z);
}

vec3 to_vec3(const vec4 &v) {
    return {v.x, v.y, v.z};
}

vec4 to_vec4(const vec3 &v, f32 w) {
    return {v.x, v.y, v.z, w};
}