#pragma once

#define ROW_MAJOR 0
#define DIRECTX_COORDINATE_SYSTEM 1


#include <cmath>
#include <cfloat>
#include <iostream>

#include "common.h"
#include "simd.h"

#define M_PI 3.14159265359f
#define M_PI_OVER_2 (M_PI * 0.5f)
#define M_PI_OVER_180 (M_PI / 180.f)
#define M_180_OVER_PI (180.f / M_PI)
#define SQRT_PI	1.77245385090f
#define INV_PI 0.31830988618379f
#define M_TAU 6.28318530718f
#define INV_TAU 0.159154943091895335f

#define EPSILON 1e-6f

#define deg2rad(deg) ((deg) * M_PI_OVER_180)
#define rad2deg(rad) ((rad) * M_180_OVER_PI)

static constexpr float lerp(float l, float u, float t) { return l + t * (u - l); }
static constexpr float inverse_lerp(float l, float u, float v) { return (v - l) / (u - l); }
static constexpr float remap(float v, float oldL, float oldU, float newL, float newU) { return lerp(newL, newU, inverse_lerp(oldL, oldU, v)); }
static constexpr float clamp(float v, float l, float u) { float r = max(l, v); r = min(u, r); return r; }
static constexpr u32 clamp(u32 v, u32 l, u32 u) { u32 r = max(l, v); r = min(u, r); return r; }
static constexpr i32 clamp(i32 v, i32 l, i32 u) { i32 r = max(l, v); r = min(u, r); return r; }
static constexpr float clamp01(float v) { return clamp(v, 0.f, 1.f); }
static constexpr float saturate(float v) { return clamp01(v); }
static constexpr float smoothstep(float t) { return t * t * (3.f - 2.f * t); }
static constexpr float smoothstep(float l, float u, float v) { return smoothstep(clamp01(inverse_lerp(l, u, v))); }
static constexpr u32 bucketize(u32 problem_size, u32 bucket_size) { return (problem_size + bucket_size - 1) / bucket_size; }
static constexpr u64 bucketize(u64 problem_size, u64 bucket_size) { return (problem_size + bucket_size - 1) / bucket_size; }

static float frac(float v) { return fmodf(v, 1.f); }
static void copy_sign(float from, float& to) { to = copysign(to, from); }

// Constexpr-version of _BitScanForward. Returns -1 if mask is zero.
static constexpr u32 index_of_least_significant_set_bit(u32 i)
{
	if (i == 0)
	{
		return (u32)-1;
	}

	u32 count = 0;

	while ((i & 1) == 0)
	{
		i >>= 1;
		++count;
	}
	return count;
}

// Constexpr-version of _BitScanReverse. Returns -1 if mask is zero.
static constexpr u32 index_of_most_significant_set_bit(u32 i)
{
	if (i == 0)
	{
		return (u32)-1;
	}

	u32 count = 0;

	while (i != 1)
	{
		i >>= 1;
		++count;
	}
	return count;
}

static constexpr i32 log2(i32 i)
{
	ASSERT(i >= 0);

	u32 mssb = index_of_most_significant_set_bit((u32)i);
	u32 lssb = index_of_least_significant_set_bit((u32)i);

	if (mssb == (u32)-1 || lssb == (u32)-1)
	{
		return 0;
	}

	// If perfect power of two (only one set bit), return index of bit.  Otherwise round up
	// fractional log by adding 1 to most signicant set bit's index.
	return (i32)mssb + (mssb == lssb ? 0 : 1);
}

static constexpr u32 log2(u32 i) 
{ 
	u32 mssb = index_of_most_significant_set_bit(i);
	u32 lssb = index_of_least_significant_set_bit(i);

	if (mssb == (u32)-1 || lssb == (u32)-1)
	{
		return 0;
	}

	// If perfect power of two (only one set bit), return index of bit.  Otherwise round up
	// fractional log by adding 1 to most signicant set bit's index.
	return mssb + (mssb == lssb ? 0 : 1);
}

static constexpr bool is_power_of_two(u32 i)
{
	return (i & (i - 1)) == 0;
}

static constexpr u32 align_to_power_of_two(u32 i)
{
	return i == 0u ? 0u : 1u << log2(i);
}

union vec2
{
	struct
	{
		float x, y;
	};
	float data[2];

	vec2() {}
	vec2(float v) : vec2(v, v) {}
	vec2(float x, float y) : x(x), y(y) {}

	static const vec2 zero;
};

union vec3
{
	struct
	{
		float x, y, z;
	};
	struct
	{
		float r, g, b;
	};
	struct
	{
		vec2 xy;
		float z;
	};
	struct
	{
		float x;
		vec2 yz;
	};
	float data[3];

	vec3() {}
	vec3(float v) : vec3(v, v, v) {}
	vec3(float x, float y, float z) : x(x), y(y), z(z) {}
	vec3(vec2 xy, float z) : x(xy.x), y(xy.y), z(z) {}

	static const vec3 zero;
};

union vec4
{
	struct
	{
		float x, y, z, w;
	};
	struct
	{
		float r, g, b, a;
	};
	struct
	{
		vec3 xyz;
		float w;
	};
	struct
	{
		vec2 xy;
		vec2 zw;
	};
	struct
	{
		float x;
		vec3 yzw;
	};
	w4_float f4;
	float data[4];

	vec4() {}
	vec4(float v) : vec4(v, v, v, v) {}
	vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
	vec4(vec3 xyz, float w) : x(xyz.x), y(xyz.y), z(xyz.z), w(w) {}
	vec4(w4_float f4) : f4(f4) {}

	static const vec4 zero;
};

union quat
{
	struct
	{
		float x, y, z, w;
	};
	struct
	{
		vec3 v;
		float cos_half_angle;
	};
	vec4 v4;
	w4_float f4;

	quat() {}
	quat(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
	quat(vec3 axis, float angle);
	quat(vec4 v4) : v4(v4) {}
	quat(w4_float f4) : f4(f4) {}

	static const quat identity;
	static const quat zero;
};

struct dual_quat
{
	quat real;
	quat dual;

	dual_quat() {}
	dual_quat(quat real, quat dual) : real(real), dual(dual) {}
	dual_quat(quat rotation, vec3 translation);

	vec3 get_translation();
	quat get_roation();

	static const dual_quat identity;
	static const dual_quat zero;
};

union mat2
{
#if ROW_MAJOR
	struct
	{
		float
			m00, m01,
			m10, m11;
	};
	struct
	{
		vec2 row0;
		vec2 row1;
	};
	vec2 rows[2];
#else
	struct
	{
		float
			m00, m10,
			m01, m11;
	};
	struct
	{
		vec2 col0;
		vec2 col1;
	};
	vec2 cols[2];
#endif
	float m[4];

	mat2() {}
	mat2(
		float m00, float m01,
		float m10, float m11);

	static const mat2 identity;
	static const mat2 zero;
};

union mat3
{
#if ROW_MAJOR
	struct
	{
		float
			m00, m01, m02,
			m10, m11, m12,
			m20, m21, m22;
	};
	struct
	{
		vec3 row0;
		vec3 row1;
		vec3 row2;
	};
	vec3 rows[3];
#else
	struct
	{
		float
			m00, m10, m20,
			m01, m11, m21,
			m02, m12, m22;
	};
	struct
	{
		vec3 col0;
		vec3 col1;
		vec3 col2;
	};
	vec3 cols[3];
#endif
	float m[9];

	mat3() {}
	mat3(
		float m00, float m01, float m02,
		float m10, float m11, float m12,
		float m20, float m21, float m22);

	static const mat3 identity;
	static const mat3 zero;
};

union mat4
{
#if ROW_MAJOR
	struct
	{
		float
			m00, m01, m02, m03,
			m10, m11, m12, m13,
			m20, m21, m22, m23,
			m30, m31, m32, m33;
	};
	struct
	{
		vec4 row0;
		vec4 row1;
		vec4 row2;
		vec4 row3;
	};
	vec4 rows[4];
#else
	struct
	{
		float
			m00, m10, m20, m30,
			m01, m11, m21, m31,
			m02, m12, m22, m32,
			m03, m13, m23, m33;
	};
	struct
	{
		vec4 col0;
		vec4 col1;
		vec4 col2;
		vec4 col3;
	};
	vec4 cols[4];
#endif
	struct
	{
		w4_float f40;
		w4_float f41;
		w4_float f42;
		w4_float f43;
	};
	float m[16];

	mat4() {}
	mat4(
		float m00, float m01, float m02, float m03,
		float m10, float m11, float m12, float m13,
		float m20, float m21, float m22, float m23,
		float m30, float m31, float m32, float m33);

	static const mat4 identity;
	static const mat4 zero;
};

static_assert(sizeof(mat4) == 16 * sizeof(float), "");

#if ROW_MAJOR
static vec2 row(const mat2& a, u32 r) { return a.rows[r]; }
static vec3 row(const mat3& a, u32 r) { return a.rows[r]; }
static vec4 row(const mat4& a, u32 r) { return a.rows[r]; }

static vec2 col(const mat2& a, u32 c) { return { a.m[c], a.m[c + 2] }; }
static vec3 col(const mat3& a, u32 c) { return { a.m[c], a.m[c + 3], a.m[c + 6] }; }
static vec4 col(const mat4& a, u32 c) { return { a.m[c], a.m[c + 4], a.m[c + 8], a.m[c + 12] }; }
#else
static vec2 row(const mat2& a, u32 r) { return { a.m[r], a.m[r + 2] }; }
static vec3 row(const mat3& a, u32 r) { return { a.m[r], a.m[r + 3], a.m[r + 6] }; }
static vec4 row(const mat4& a, u32 r) { return { a.m[r], a.m[r + 4], a.m[r + 8], a.m[r + 12] }; }
	   
static vec2 col(const mat2& a, u32 c) { return a.cols[c]; }
static vec3 col(const mat3& a, u32 c) { return a.cols[c]; }
static vec4 col(const mat4& a, u32 c) { return a.cols[c]; }
#endif

struct Transform
{
	quat rotation;
	vec3 position;
	vec3 scale;

	Transform() {}
	Transform(vec3 position, quat rotation = quat::identity, vec3 scale = { 1.f, 1.f, 1.f }) : position(position), rotation(rotation), scale(scale) {}

	static const Transform identity;
};




// Vec2 operators.
static vec2 operator+(vec2 a, vec2 b) { vec2 result = { a.x + b.x, a.y + b.y }; return result; }
static vec2& operator+=(vec2& a, vec2 b) { a = a + b; return a; }
static vec2 operator-(vec2 a, vec2 b) { vec2 result = { a.x - b.x, a.y - b.y }; return result; }
static vec2& operator-=(vec2& a, vec2 b) { a = a - b; return a; }
static vec2 operator*(vec2 a, vec2 b) { vec2 result = { a.x * b.x, a.y * b.y }; return result; }
static vec2& operator*=(vec2& a, vec2 b) { a = a * b; return a; }
static vec2 operator/(vec2 a, vec2 b) { vec2 result = { a.x / b.x, a.y / b.y }; return result; }
static vec2& operator/=(vec2& a, vec2 b) { a = a / b; return a; }

static vec2 operator*(vec2 a, float b) { vec2 result = { a.x * b, a.y * b }; return result; }
static vec2 operator*(float a, vec2 b) { return b * a; }
static vec2& operator*=(vec2& a, float b) { a = a * b; return a; }
static vec2 operator/(vec2 a, float b) { vec2 result = { a.x / b, a.y / b }; return result; }
static vec2& operator/=(vec2& a, float b) { a = a / b; return a; }

static vec2 operator-(vec2 a) { return vec2(-a.x, -a.y); }

static bool operator==(vec2 a, vec2 b) { return a.x == b.x && a.y == b.y; }
static bool operator!=(vec2 a, vec2 b) { return !(a == b); }


// Vec3 operators.
static vec3 operator+(vec3 a, vec3 b) { vec3 result = { a.x + b.x, a.y + b.y, a.z + b.z }; return result; }
static vec3& operator+=(vec3& a, vec3 b) { a = a + b; return a; }
static vec3 operator-(vec3 a, vec3 b) { vec3 result = { a.x - b.x, a.y - b.y, a.z - b.z }; return result; }
static vec3& operator-=(vec3& a, vec3 b) { a = a - b; return a; }
static vec3 operator*(vec3 a, vec3 b) { vec3 result = { a.x * b.x, a.y * b.y, a.z * b.z }; return result; }
static vec3& operator*=(vec3& a, vec3 b) { a = a * b; return a; }
static vec3 operator/(vec3 a, vec3 b) { vec3 result = { a.x / b.x, a.y / b.y, a.z / b.z }; return result; }
static vec3& operator/=(vec3& a, vec3 b) { a = a / b; return a; }

static vec3 operator*(vec3 a, float b) { vec3 result = { a.x * b, a.y * b, a.z * b }; return result; }
static vec3 operator*(float a, vec3 b) { return b * a; }
static vec3& operator*=(vec3& a, float b) { a = a * b; return a; }
static vec3 operator/(vec3 a, float b) { vec3 result = { a.x / b, a.y / b, a.z / b }; return result; }
static vec3& operator/=(vec3& a, float b) { a = a / b; return a; }

static vec3 operator-(vec3 a) { return vec3(-a.x, -a.y, -a.z); }

static bool operator==(vec3 a, vec3 b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
static bool operator!=(vec3 a, vec3 b) { return !(a == b); }


// Vec4 operators.
static vec4 operator+(vec4 a, vec4 b) { vec4 result = { a.f4 + b.f4 }; return result; }
static vec4& operator+=(vec4& a, vec4 b) { a = a + b; return a; }
static vec4 operator-(vec4 a, vec4 b) { vec4 result = { a.f4 - b.f4 }; return result; }
static vec4& operator-=(vec4& a, vec4 b) { a = a - b; return a; }
static vec4 operator*(vec4 a, vec4 b) { vec4 result = { a.f4 * b.f4 }; return result; }
static vec4& operator*=(vec4& a, vec4 b) { a = a * b; return a; }
static vec4 operator/(vec4 a, vec4 b) { vec4 result = { a.f4 / b.f4 }; return result; }
static vec4& operator/=(vec4& a, vec4 b) { a = a / b; return a; }

static vec4 operator*(vec4 a, float b) { vec4 result = { a.f4 * w4_float(b) }; return result; }
static vec4 operator*(float a, vec4 b) { return b * a; }
static vec4& operator*=(vec4& a, float b) { a = a * b; return a; }
static vec4 operator/(vec4 a, float b) { vec4 result = { a.f4 / w4_float(b) }; return result; }
static vec4& operator/=(vec4& a, float b) { a = a / b; return a; }

static vec4 operator-(vec4 a) { return vec4(-a.f4); }

static bool operator==(vec4 a, vec4 b) { return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w; }
static bool operator!=(vec4 a, vec4 b) { return !(a == b); }


static vec2 diagonal(const mat2& m) { return vec2(m.m00, m.m11); }
static vec3 diagonal(const mat3& m) { return vec3(m.m00, m.m11, m.m22); }
static vec4 diagonal(const mat4& m) { return vec4(m.m00, m.m11, m.m22, m.m33); }

static float dot(vec2 a, vec2 b) { float result = a.x * b.x + a.y * b.y; return result; }
static float dot(vec3 a, vec3 b) { float result = a.x * b.x + a.y * b.y + a.z * b.z; return result; }
static float dot(vec4 a, vec4 b) { w4_float m = a.f4 * b.f4; return add_elements(m); }

static float cross(vec2 a, vec2 b) { return a.x * b.y - a.y * b.x; }
static vec3 cross(vec3 a, vec3 b) { vec3 result = { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; return result; }

static float squared_length(vec2 a) { return dot(a, a); }
static float squared_length(vec3 a) { return dot(a, a); }
static float squared_length(vec4 a) { return dot(a, a); }

static float length(vec2 a) { return sqrt(squared_length(a)); }
static float length(vec3 a) { return sqrt(squared_length(a)); }
static float length(vec4 a) { return sqrt(squared_length(a)); }

static vec2 noz(vec2 a) { float sl = squared_length(a); return (sl < 1e-8f) ? vec2(0.f, 0.f) : (a * (1.f / sqrt(sl))); }
static vec3 noz(vec3 a) { float sl = squared_length(a); return (sl < 1e-8f) ? vec3(0.f, 0.f, 0.f) : (a * (1.f / sqrt(sl))); }
static vec4 noz(vec4 a) { float sl = squared_length(a); return (sl < 1e-8f) ? vec4(0.f, 0.f, 0.f, 0.f) : (a * (1.f / sqrt(sl))); }

static vec2 normalize(vec2 a) { float l = length(a); return a * (1.f / l); }
static vec3 normalize(vec3 a) { float l = length(a); return a * (1.f / l); }
static vec4 normalize(vec4 a) { float l = length(a); return a * (1.f / l); }

static void copy_sign(vec2 from, vec2& to) { copy_sign(from.x, to.x); copy_sign(from.y, to.y); }
static void copy_sign(vec3 from, vec3& to) { copy_sign(from.x, to.x); copy_sign(from.y, to.y); copy_sign(from.z, to.z); }
static void copy_sign(vec4 from, vec4& to) { copy_sign(from.x, to.x); copy_sign(from.y, to.y); copy_sign(from.z, to.z); copy_sign(from.w, to.w); }

static vec2 abs(vec2 a) { return vec2(abs(a.x), abs(a.y)); }
static vec3 abs(vec3 a) { return vec3(abs(a.x), abs(a.y), abs(a.z)); }
static vec4 abs(vec4 a) { return vec4(abs(a.f4)); }

static vec2 floor(vec2 a) { return vec2(floor(a.x), floor(a.y)); }
static vec3 floor(vec3 a) { return vec3(floor(a.x), floor(a.y), floor(a.z)); }
static vec4 floor(vec4 a) { return vec4(floor(a.f4)); }

static vec2 round(vec2 a) { return vec2(round(a.x), round(a.y)); }
static vec3 round(vec3 a) { return vec3(round(a.x), round(a.y), round(a.z)); }
static vec4 round(vec4 a) { return vec4(round(a.f4)); }

static vec2 frac(vec2 a) { return vec2(frac(a.x), frac(a.y)); }
static vec3 frac(vec3 a) { return vec3(frac(a.x), frac(a.y), frac(a.z)); }
static vec4 frac(vec4 a) { return vec4(frac(a.x), frac(a.y), frac(a.z), frac(a.w)); }

static quat normalize(quat a) { return { normalize(a.v4).f4 }; }
static quat conjugate(quat a) { return { -a.x, -a.y, -a.z, a.w }; }

static quat operator+(quat a, quat b) { quat result = { a.f4 + b.f4 }; return result; }

static quat operator*(quat a, quat b)
{
	quat result;
	result.w = a.w * b.w - dot(a.v, b.v);
	result.v = a.v * b.w + b.v * a.w + cross(a.v, b.v);
	return result;
}

static quat operator*(quat q, float s)
{
	quat result;
	result.v4 = q.v4 * s;
	return result;
}

static vec3 operator*(quat q, vec3 v)
{
	quat p(v.x, v.y, v.z, 0.f);
	return (q * p * conjugate(q)).v;
}

static dual_quat operator+(const dual_quat& a, const dual_quat& b) { return { a.real + b.real, a.dual + b.dual }; }
static dual_quat& operator+=(dual_quat& a, const dual_quat& b) { a = a + b; return a; }
static dual_quat operator*(const dual_quat& a, const dual_quat& b) { return { a.real * b.real, a.dual * b.real + b.dual * a.real }; }
static dual_quat operator*(const dual_quat& a, float b) { return { a.real * b, a.dual * b }; }
static dual_quat operator*(float b, const dual_quat& a) { return a * b; }
static dual_quat& operator*=(dual_quat& q, float v) { q = q * v; return q; }
static dual_quat normalize(const dual_quat& q) { float n = 1.f / length(q.real.v4); return q * n; }

static bool operator==(quat a, quat b) { return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w; }
static bool operator!=(quat a, quat b) { return !(a == b); }

static vec2 operator*(mat2 a, vec2 b) { vec2 result = { dot(row(a, 0), b), dot(row(a, 1), b) }; return result; }
static vec3 operator*(mat3 a, vec3 b) { vec3 result = { dot(row(a, 0), b), dot(row(a, 1), b), dot(row(a, 2), b) }; return result; }

#if ROW_MAJOR
static vec4 operator*(mat4 a, vec4 b) { vec4 result = { dot(row(a, 0), b), dot(row(a, 1), b), dot(row(a, 2), b), dot(row(a, 3), b) }; return result; }
#else
static vec4 operator*(mat4 a, vec4 b) { vec4 result = col(a, 0) * b.x + col(a, 1) * b.y + col(a, 2) * b.z + col(a, 3) * b.w; return result; }
#endif



static vec2 lerp(vec2 l, vec2 u, float t) { return l + t * (u - l); }
static vec3 lerp(vec3 l, vec3 u, float t) { return l + t * (u - l); }
static vec4 lerp(vec4 l, vec4 u, float t) { return l + t * (u - l); }
static quat lerp(quat l, quat u, float t) { quat result; result.v4 = lerp(l.v4, u.v4, t); return normalize(result); }
static dual_quat lerp(const dual_quat& l, const dual_quat& u, float t) { return { quat(lerp(l.real.v4, u.real.v4, t)), quat(lerp(l.dual.v4, u.dual.v4, t)) }; }
static Transform lerp(const Transform& l, const Transform& u, float t)
{
	Transform result;
	result.position = lerp(l.position, u.position, t);
	result.rotation = lerp(l.rotation, u.rotation, t);
	result.scale = lerp(l.scale, u.scale, t);
	return result;
}

static vec2 exp(vec2 v) { return vec2(exp(v.x), exp(v.y)); }
static vec3 exp(vec3 v) { return vec3(exp(v.x), exp(v.y), exp(v.z)); }
static vec4 exp(vec4 v) { return vec4(exp(v.f4)); }

static vec2 pow(vec2 v, float e) { return vec2(pow(v.x, e), pow(v.y, e)); }
static vec3 pow(vec3 v, float e) { return vec3(pow(v.x, e), pow(v.y, e), pow(v.z, e)); }
static vec4 pow(vec4 v, float e) { return vec4(pow(v.f4, w4_float(e))); }

static vec2 min(vec2 a, vec2 b) { return vec2(min(a.x, b.x), min(a.y, b.y)); }
static vec3 min(vec3 a, vec3 b) { return vec3(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z)); }
static vec4 min(vec4 a, vec4 b) { return vec4(minimum(a.f4, b.f4)); }

static vec2 max(vec2 a, vec2 b) { return vec2(max(a.x, b.x), max(a.y, b.y)); }
static vec3 max(vec3 a, vec3 b) { return vec3(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z)); }
static vec4 max(vec4 a, vec4 b) { return vec4(maximum(a.f4, b.f4)); }

static float max_element(vec2 a) { return max(a.x, a.y); }
static float max_element(vec3 a) { return max(a.x, max(a.y, a.z)); }
static float max_element(vec4 a) { return max(a.x, max(a.y, max(a.z, a.w))); }

static u32 max_element_index(vec2 a) { return (a.x > a.y) ? 0 : 1; }
static u32 max_element_index(vec3 a) { return (a.x > a.y) ? ((a.x > a.z) ? 0 : 2) : ((a.y > a.z) ? 1 : 2); }
static u32 max_element_index(vec4 a) { return (a.x > a.y)	? ((a.x > a.z)	? ((a.x > a.w) ? 0 : 3) 
																			: ((a.z > a.w) ? 2 : 3)) 
															: ((a.y > a.z)	? ((a.y > a.w) ? 1 : 3) 
																			: ((a.z > a.w) ? 2 : 3)); }

static vec2 remap(vec2 v, vec2 oldL, vec2 oldU, vec2 newL, vec2 newU)
{
	return
	{
		remap(v.x, oldL.x, oldU.x, newL.x, newU.x),
		remap(v.y, oldL.y, oldU.y, newL.y, newU.y),
	};
}
static vec3 remap(vec3 v, vec3 oldL, vec3 oldU, vec3 newL, vec3 newU)
{
	return
	{
		remap(v.x, oldL.x, oldU.x, newL.x, newU.x),
		remap(v.y, oldL.y, oldU.y, newL.y, newU.y),
		remap(v.z, oldL.z, oldU.z, newL.z, newU.z),
	};
}
static vec4 remap(vec4 v, vec4 oldL, vec4 oldU, vec4 newL, vec4 newU)
{
	return
	{
		remap(v.x, oldL.x, oldU.x, newL.x, newU.x),
		remap(v.y, oldL.y, oldU.y, newL.y, newU.y),
		remap(v.z, oldL.z, oldU.z, newL.z, newU.z),
		remap(v.w, oldL.w, oldU.w, newL.w, newU.w),
	};
}

mat2 operator*(const mat2& a, const mat2& b);
mat3 operator*(const mat3& a, const mat3& b);
mat3 operator+(const mat3& a, const mat3& b);
mat3& operator+=(mat3& a, const mat3& b);
mat3 operator-(const mat3& a, const mat3& b);
mat4 operator*(const mat4& a, const mat4& b);
mat2 operator*(const mat2& a, float b);
mat3 operator*(const mat3& a, float b);
mat4 operator*(const mat4& a, float b);
mat2 operator*(float a, const mat2& b);
mat3 operator*(float a, const mat3& b);
mat4 operator*(float a, const mat4& b);
mat2& operator*=(mat2& a, float b);
mat3& operator*=(mat3& a, float b);
mat4& operator*=(mat4& a, float b);
Transform operator*(const Transform& a, const Transform& b);

mat2 transpose(const mat2& a);
mat3 transpose(const mat3& a);
mat4 transpose(const mat4& a);

mat3 invert(const mat3& m);
mat4 invert(const mat4& m);
Transform invert(const Transform& t);

float determinant(const mat2& m);
float determinant(const mat3& m);
float determinant(const mat4& m);

float trace(const mat3& m);
float trace(const mat4& m);

vec3 transform_position(const mat4& m, vec3 pos);
vec3 transform_direction(const mat4& m, vec3 dir);
vec3 transform_position(const Transform& m, vec3 pos);
vec3 transform_direction(const Transform& m, vec3 dir);
vec3 inverse_transform_position(const Transform& m, vec3 pos);
vec3 inverse_transform_direction(const Transform& m, vec3 dir);

quat rotate_from_to(vec3 from, vec3 to);
quat look_at_quaternion(vec3 forward, vec3 up);
void get_axis_rotation(quat q, vec3& axis, float& angle);
void decompose_quaternion_into_twist_and_swing(quat q, vec3 normalizedTwistAxis, quat& twist, quat& swing);

quat slerp(quat from, quat to, float t);
quat nlerp(quat* qs, float* weights, u32 count);

mat3 quaternion_to_mat3(quat q);
quat mat3_to_quaternion(const mat3& m);

vec3 quaternion_to_euler(quat q);
quat euler_to_quaternion(vec3 euler);

mat3 outer_product(vec3 a, vec3 b);
mat3 get_skew_matrix(vec3 r);

mat4 transform_to_mat4(const Transform& transform);
Transform mat4_to_transform(const mat4& m);

mat4 create_perspective_projection_matrix(float fov, float aspect, float nearPlane, float farPlane);
mat4 create_perspective_projection_matrix(float width, float height, float fx, float fy, float cx, float cy, float nearPlane, float farPlane);
mat4 create_perspective_projection_matrix(float r, float l, float t, float b, float nearPlane, float farPlane);
mat4 create_orthographic_projection_matrix(float r, float l, float t, float b, float nearPlane, float farPlane);
mat4 invert_perspective_projection_matrix(const mat4& m);
mat4 invert_orthographic_projection_matrix(const mat4& m);
mat4 create_translation_matrix(vec3 position);
mat4 create_model_matrix(vec3 position, quat rotation, vec3 scale = vec3(1.f, 1.f, 1.f));
mat4 create_billboard_model_matrix(vec3 position, vec3 eye, vec3 scale);
mat4 create_view_matrix(vec3 eye, float pitch, float yaw);
mat4 create_sky_view_matrix(const mat4& v);
mat4 look_at_matrix(vec3 eye, vec3 target, vec3 up);
mat4 create_view_matrix(vec3 position, quat rotation);
mat4 invert_affine_matrix(const mat4& m);

bool is_point_in_triangle(vec3 point, vec3 triA, vec3 triB, vec3& triC);
bool is_point_in_rectangle(vec2 p, vec2 topLeft, vec2 bottomRight);
bool is_point_in_box(vec3 p, vec3 minCorner, vec3 maxCorner);

vec2 direction_to_panorama_uv(vec3 dir);

float angle_to_zero_to_two_pi(float angle);
float angle_to_net_pi_to_pi(float angle);

vec2 solve_linear_system(const mat2& A, vec2 b);
vec3 solve_linear_system(const mat3& A, vec3 b);

vec3 get_barycentric_coordinates(vec2 a, vec2 b, vec2 c, vec2 p);
vec3 get_barycentric_coordinates(vec3 a, vec3 b, vec3 c, vec3 p);
bool is_barycentric_in_triangle(vec3 barycentrics);

void get_tangents(vec3 normal, vec3& out_tangent, vec3& out_bitangent);
vec3 get_tangent(vec3 normal);

// W = PDF = 1 / 4pi
vec4 uniform_sample_sphere(vec2 E);





struct SVD
{
	mat3 U;
	mat3 V;
	vec3 singularValues;
};

struct QRDecomposition
{
	mat3 Q, R;
};

SVD compute_svd(const mat3& A);
QRDecomposition qr_decomposition(const mat3& mat);
bool get_eigen_values_and_vectors(const mat3& A, vec3& out_eigen_values, mat3& out_eigen_vectors);


template <typename T>
void exclusive_prefix_sum(const T* input, T* output, u32 count)
{
	if (count > 0)
	{
		output[0] = T(0);
		for (u32 i = 1; i < count; ++i)
		{
			output[i] = output[i - 1] + input[i - 1];
		}
	}
}

template <typename T>
T sum(const T* input, u32 count)
{
	T result = 0;
	for (u32 i = 0; i < count; ++i)
	{
		result += input[i];
	}
	return result;
}



static bool fuzzy_equals(float a, float b, float threshold = 1e-4f) { return abs(a - b) < threshold; }
static bool fuzzy_equals(vec2 a, vec2 b, float threshold = 1e-4f) { return fuzzy_equals(a.x, b.x, threshold) && fuzzy_equals(a.y, b.y, threshold); }
static bool fuzzy_equals(vec3 a, vec3 b, float threshold = 1e-4f) { return fuzzy_equals(a.x, b.x, threshold) && fuzzy_equals(a.y, b.y, threshold) && fuzzy_equals(a.z, b.z, threshold); }
static bool fuzzy_equals(vec4 a, vec4 b, float threshold = 1e-4f) { return fuzzy_equals(a.x, b.x, threshold) && fuzzy_equals(a.y, b.y, threshold) && fuzzy_equals(a.z, b.z, threshold) && fuzzy_equals(a.w, b.w, threshold); }
static bool fuzzy_equals(quat a, quat b, float threshold = 1e-4f) { if (dot(a.v4, b.v4) < 0.f) { a.v4 *= -1.f; } return fuzzy_equals(a.x, b.x, threshold) && fuzzy_equals(a.y, b.y, threshold) && fuzzy_equals(a.z, b.z, threshold) && fuzzy_equals(a.w, b.w, threshold); }

static bool fuzzy_equals(const mat2& a, const mat2& b, float threshold = 1e-4f)
{
	bool result = true;
	for (u32 i = 0; i < 4; ++i)
	{
		result &= fuzzy_equals(a.m[i], b.m[i], threshold);
	}
	return result;
}

static bool fuzzy_equals(const mat3& a, const mat3& b, float threshold = 1e-4f)
{
	bool result = true;
	for (u32 i = 0; i < 9; ++i)
	{
		result &= fuzzy_equals(a.m[i], b.m[i], threshold);
	}
	return result;
}

static bool fuzzy_equals(const mat4& a, const mat4& b, float threshold = 1e-4f)
{
	bool result = true;
	for (u32 i = 0; i < 16; ++i)
	{
		result &= fuzzy_equals(a.m[i], b.m[i], threshold);
	}
	return result;
}

static bool fuzzy_equals(const Transform& a, const Transform& b, float threshold = 1e-4f)
{
	bool result = true;
	result &= fuzzy_equals(a.position, b.position, threshold);
	result &= fuzzy_equals(a.rotation, b.rotation, threshold);
	result &= fuzzy_equals(a.scale, b.scale, threshold);
	return result;
}

inline quat::quat(vec3 axis, float angle)
{
	w = cos(angle * 0.5f);
	v = axis * sin(angle * 0.5f);
}

inline dual_quat::dual_quat(quat rotation, vec3 translation)
{
	float w = -0.5f * (translation.x * rotation.x + translation.y * rotation.y + translation.z * rotation.z);
	float x = 0.5f * (translation.x * rotation.w + translation.y * rotation.z - translation.z * rotation.y);
	float y = 0.5f * (-translation.x * rotation.z + translation.y * rotation.w + translation.z * rotation.x);
	float z = 0.5f * (translation.x * rotation.y - translation.y * rotation.x + translation.z * rotation.w);

	real = rotation;
	dual = quat(x, y, z, w);
}

inline vec3 dual_quat::get_translation()
{
	quat tq = dual * conjugate(real);
	return 2.f * tq.v4.xyz;
}

inline quat dual_quat::get_roation()
{
	return real;
}

inline mat2::mat2(
	float m00, float m01,
	float m10, float m11)
	:
	m00(m00), m01(m01),
	m10(m10), m11(m11) {}

inline mat3::mat3(
	float m00, float m01, float m02,
	float m10, float m11, float m12,
	float m20, float m21, float m22)
	:
	m00(m00), m01(m01), m02(m02),
	m10(m10), m11(m11), m12(m12),
	m20(m20), m21(m21), m22(m22) {}

inline mat4::mat4(
	float m00, float m01, float m02, float m03,
	float m10, float m11, float m12, float m13,
	float m20, float m21, float m22, float m23,
	float m30, float m31, float m32, float m33)
	:
	m00(m00), m01(m01), m02(m02), m03(m03),
	m10(m10), m11(m11), m12(m12), m13(m13),
	m20(m20), m21(m21), m22(m22), m23(m23),
	m30(m30), m31(m31), m32(m32), m33(m33) {}

inline std::ostream& operator<<(std::ostream& s, vec2 v)
{
	s << "[" << v.x << ", " << v.y << "]";
	return s;
}

inline std::ostream& operator<<(std::ostream& s, vec3 v)
{
	s << "[" << v.x << ", " << v.y << ", " << v.z << "]";
	return s;
}

inline std::ostream& operator<<(std::ostream& s, vec4 v)
{
	s << "[" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << "]";
	return s;
}

inline std::ostream& operator<<(std::ostream& s, quat v)
{
	s << "[" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << "]";
	return s;
}

inline std::ostream& operator<<(std::ostream& s, const mat2& m)
{
	s << "[" << m.m00 << ", " << m.m01 << "]\n";
	s << "[" << m.m10 << ", " << m.m11 << "]";
	return s;
}

inline std::ostream& operator<<(std::ostream& s, const mat3& m)
{
	s << "[" << m.m00 << ", " << m.m01 << ", " << m.m02 << "]\n";
	s << "[" << m.m10 << ", " << m.m11 << ", " << m.m12 << "]\n";
	s << "[" << m.m20 << ", " << m.m21 << ", " << m.m22 << "]";
	return s;
}

inline std::ostream& operator<<(std::ostream& s, const mat4& m)
{
	s << "[" << m.m00 << ", " << m.m01 << ", " << m.m02 << ", " << m.m03 << "]\n";
	s << "[" << m.m10 << ", " << m.m11 << ", " << m.m12 << ", " << m.m13 << "]\n";
	s << "[" << m.m20 << ", " << m.m21 << ", " << m.m22 << ", " << m.m23 << "]\n";
	s << "[" << m.m30 << ", " << m.m31 << ", " << m.m32 << ", " << m.m33 << "]";
	return s;
}

inline std::ostream& operator<<(std::ostream& s, const Transform& m)
{
	s << "Position: " << m.position << '\n';
	s << "Rotation: " << m.rotation << '\n';
	s << "Scale: " << m.scale << '\n';
	return s;
}



static float ease_in_quadratic(float t) { return t * t; }
static float ease_out_quadratic(float t) { return t * (2.f - t); }
static float ease_inout_quadratic(float t) { return (t < 0.5f) ? (2.f * t * t) : (-1.f + (4.f - 2.f * t) * t); }

static float ease_in_cubic(float t) { return t * t * t; }
static float ease_out_cubic(float t) { float tmin1 = t - 1.f; return tmin1 * tmin1 * tmin1 + 1.f; }
static float ease_inout_cubic(float t) { return (t < 0.5f) ? (4.f * t * t * t) : ((t - 1.f) * (2.f * t - 2.f) * (2.f * t - 2.f) + 1.f); }

static float ease_in_quartic(float t) { return t * t * t * t; }
static float ease_out_quartic(float t) { float tmin1 = t - 1.f; return 1.f - tmin1 * tmin1 * tmin1 * tmin1; }
static float ease_inout_quartic(float t) { float tmin1 = t - 1.f; return (t < 0.5f) ? (8.f * t * t * t * t) : (1.f - 8.f * tmin1 * tmin1 * tmin1 * tmin1); }

static float ease_in_quintic(float t) { return t * t * t * t * t; }
static float ease_out_quintic(float t) { float tmin1 = t - 1.f; return 1.f + tmin1 * tmin1 * tmin1 * tmin1 * tmin1; }
static float ease_inout_quintic(float t) { float tmin1 = t - 1.f; return t < 0.5 ? 16.f * t * t * t * t * t : 1.f + 16.f * tmin1 * tmin1 * tmin1 * tmin1 * tmin1; }

static float ease_in_sine(float t) { return sin((t - 1.f) * M_PI_OVER_2) + 1.f; }
static float ease_out_sine(float t) { return sin(t * M_PI_OVER_2); }
static float ease_inout_sine(float t) { return 0.5f * (1 - cos(t * M_PI)); }

static float ease_in_circular(float t) { return 1.f - sqrt(1.f - (t * t)); }
static float ease_out_circular(float t) { return sqrt((2.f - t) * t); }
static float ease_inout_circular(float t) { return (t < 0.5f) ? (0.5f * (1.f - sqrt(1.f - 4.f * (t * t)))) : (0.5f * (sqrt(-((2.f * t) - 3.f) * ((2.f * t) - 1.f)) + 1.f)); }

static float ease_in_exponential(float t) { return (t == 0.f) ? t : powf(2.f, 10.f * (t - 1.f)); }
static float ease_out_exponential(float t) { return (t == 1.f) ? t : 1.f - powf(2.f, -10.f * t); }
static float ease_inout_exponential(float t) { if (t == 0.f || t == 1.f) { return t; } return (t < 0.5f) ? (0.5f * powf(2.f, (20.f * t) - 10.f)) : (-0.5f * powf(2.f, (-20.f * t) + 10.f) + 1.f); }


static float get_framerate_independent_t(float speed, float dt)
{
	return 1.f - expf(-speed * dt);
}
