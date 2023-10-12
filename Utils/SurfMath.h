#pragma once

#include <assert.h>
#include <memory>

typedef uint32_t u32;
typedef int32_t i32;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint8_t u8;
typedef int8_t i8;

#define ISNAN(x)  ((*(const u32*)&(x) & 0x7F800000) == 0x7F800000 && (*(const u32*)&(x) & 0x7FFFFF) != 0)
#define ISINF(x)  ((*(const uint32_t*)&(x) & 0x7FFFFFFF) == 0x7F800000)

constexpr float K_PI = 3.141592654f;

template <typename T>
struct Vector2Component
{
    union
    {
        struct
        {
            T x, y;
        };
        T v[2];
    };

    constexpr Vector2Component() : x((T)0), y((T)0) {}
    constexpr Vector2Component(T _xy) : x(_xy), y(_xy) {}
    constexpr Vector2Component(T _x, T _y) : x(_x), y(_y) {}

    Vector2Component& operator*=(T rhs)
    {
        this->x *= rhs;
        this->y *= rhs;
        return *this;
    }

    Vector2Component& operator/=(T rhs)
    {
        this->x /= rhs;
        this->y /= rhs;
        return *this;
    }
};

template<typename T>
inline constexpr Vector2Component<T> operator+(Vector2Component<T> lhs, Vector2Component<T> rhs) noexcept
{
    return Vector2Component<T>(lhs.x + rhs.x, lhs.y + rhs.y);
}

template<typename T>
inline constexpr Vector2Component<T> operator-(Vector2Component<T> lhs, Vector2Component<T> rhs) noexcept
{
    return Vector2Component<T>(lhs.x - rhs.x, lhs.y - rhs.y);
}

template<typename T>
inline constexpr Vector2Component<T> operator-(Vector2Component<T> lhs, T rhs) noexcept
{
    return Vector2Component<T>(lhs.x - rhs, lhs.y - rhs);
}

template<typename T>
inline constexpr Vector2Component<T> operator*(Vector2Component<T> lhs, Vector2Component<T> rhs) noexcept
{
    return Vector2Component<T>(lhs.x * rhs.x, lhs.y * rhs.y);
}

template<typename T>
inline constexpr Vector2Component<T> operator*(Vector2Component<T> lhs, T rhs) noexcept
{
    return Vector2Component<T>(lhs.x * rhs, lhs.y * rhs);
}

template<typename T>
inline constexpr Vector2Component<T> operator/(Vector2Component<T> lhs, Vector2Component<T> rhs) noexcept
{
    return Vector2Component<T>(lhs.x / rhs.x, lhs.y / rhs.y);
}

template<typename T>
inline constexpr Vector2Component<T> operator/(Vector2Component<T> lhs, T rhs) noexcept
{
    return Vector2Component<T>(lhs.x / rhs, lhs.y / rhs);
}

template<typename T>
inline constexpr Vector2Component<T> operator/(T lhs, Vector2Component<T> rhs) noexcept
{
    return Vector2Component<T>(lhs / rhs.x, lhs / rhs.y);
}

template<typename T>
inline constexpr bool operator<(Vector2Component<T> lhs, Vector2Component<T> rhs) noexcept
{
    return lhs.x < rhs.x || lhs.y < rhs.y;
}

template<typename T>
inline constexpr bool operator==(Vector2Component<T> lhs, Vector2Component<T> rhs) noexcept
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

using float2 = Vector2Component<float>;
using uint2 = Vector2Component<u32>;
using int2 = Vector2Component<i32>;

constexpr u32 SwzX = 0;
constexpr u32 SwzY = 1;
constexpr u32 SwzZ = 2;
constexpr u32 SwzW = 3;

template <typename T>
struct Vector3Component
{
    union
    {
        struct
        {
            T x, y, z;
        };
        T v[3];
    };

    constexpr Vector3Component() : x((T)0), y((T)0), z((T)0) {}
    constexpr Vector3Component(T _xyz) : x(_xyz), y(_xyz), z(_xyz) {}
    constexpr Vector3Component(T _x, T _y, T _z) : x(_x), y(_y), z(_z) {}

    template<typename V>
    constexpr Vector3Component(V _x, V _y, V _z) : x(static_cast<T>(_x)), y(static_cast<T>(_y)), z(static_cast<T>(_z)) {}

    template<typename V>
    constexpr Vector3Component(V _xyz) : x(static_cast<T>(_xyz)), y(static_cast<T>(_xyz)), z(static_cast<T>(_xyz)) {}

    template<typename V>
    constexpr Vector3Component(const Vector3Component<V>& _other) : x(static_cast<T>(_other.x)), y(static_cast<T>(_other.y)), z(static_cast<T>(_other.z)) {}

    Vector3Component Swizzle(u32 x, u32 y, u32 z) const noexcept { return Vector3Component{ v[x], v[y], v[z] }; }

    Vector3Component& operator+=(const Vector3Component& rhs)
    {
        this->x += rhs.x;
        this->y += rhs.y;
        this->z += rhs.z;
        return *this;
    }

    Vector3Component& operator-=(const Vector3Component& rhs)
    {
        this->x -= rhs.x;
        this->y -= rhs.y;
        this->z -= rhs.z;
        return *this;
    }

    Vector3Component& operator*=(T rhs)
    {
        this->x *= rhs;
        this->y *= rhs;
        this->z *= rhs;
        return *this;
    }

    Vector3Component& operator/=(const Vector3Component& rhs)
    {
        this->x /= rhs.x;
        this->y /= rhs.y;
        this->z /= rhs.z;
        return *this;
    }

    Vector3Component& operator/=(T rhs)
    {
        this->x /= rhs;
        this->y /= rhs;
        this->z /= rhs;
        return *this;
    }
};

using float3 = Vector3Component<float>;

template<typename T>
struct Vector4Component
{
    union
    {
        struct
        {
            T x, y, z, w;
        };    
        struct
        {
            Vector3Component<T> xyz;
            T w;
        };
        T v[4];
    };

    constexpr Vector4Component() : x((T)0), y((T)0), z((T)0), w((T)0) {}
    constexpr Vector4Component(T _xyzw) : x(_xyzw), y(_xyzw), z(_xyzw), w(_xyzw) {}
    constexpr Vector4Component(Vector3Component<T> _f3, T _w = (T)0) : x(_f3.x), y(_f3.y), z(_f3.z), w(_w) {}
    constexpr Vector4Component(T _x, T _y, T _z, T _w) : x(_x), y(_y), z(_z), w(_w) {}

    Vector4Component& operator+=(const Vector4Component& rhs)
    {
        this->x += rhs.x;
        this->y += rhs.y;
        this->z += rhs.z;
        this->w += rhs.w;
        return *this;
    }

    Vector4Component& operator-=(const Vector4Component& rhs)
    {
        this->x -= rhs.x;
        this->y -= rhs.y;
        this->z -= rhs.z;
        this->w -= rhs.w;
        return *this;
    }

    Vector4Component operator*=(const Vector4Component& rhs)
    {
        this->x *= rhs.x;
        this->y *= rhs.y;
        this->z *= rhs.z;
        this->w *= rhs.w;
        return *this;
    }

    Vector4Component& operator/=(const T rhs)
    {
        x /= rhs;
        y /= rhs;
        z /= rhs;
        w /= rhs;
        return *this;
    }

    Vector4Component& operator/=(const Vector4Component& rhs)
    {
        x /= rhs.x;
        y /= rhs.y;
        z /= rhs.z;
        w /= rhs.w;
        return *this;
    }
};

using float4 = Vector4Component<float>;
using u32_4 = Vector4Component<u32>;

struct matrix2
{
    union
    {
        float2 r[2];
        struct
        {
            float _11, _12;
            float _21, _22;
        };
        float m[2][2];
    };

    constexpr matrix2()
        : _11(0), _12(0)
        , _21(0), _22(0)
    {}

    constexpr matrix2(float r0c0, float r0c1, float r1c0, float r1c1)
        : _11(r0c0), _12(r0c1)
        , _21(r1c0), _22(r1c1)
    {}

    constexpr matrix2(float2 r0, float2 r1)
        : _11(r0.x), _12(r0.y)
        , _21(r1.x), _22(r1.y)
    {}
};

struct matrix2x3
{
    union
    {
        float3 r[2];
        struct
        {
            float _11, _12, _13;
            float _21, _22, _23;
        };
        float m[2][3];
    };

    constexpr matrix2x3()
        : _11(0), _12(0), _13(0)
        , _21(0), _22(0), _23(0)
    {}

    constexpr matrix2x3(float3 r0, float3 r1)
        : _11(r0.x), _12(r0.y), _13(r0.z)
        , _21(r1.x), _22(r1.y), _23(r1.z)
    {}
};

struct matrix3x4
{
    union
    {
        float4 r[3];
        struct
        {
            float _11, _12, _13, _14;
            float _21, _22, _23, _24;
            float _31, _32, _33, _34;
        };
        float m[3][4];
    };

    constexpr matrix3x4()
        : _11(0), _12(0), _13(0), _14(0)
        , _21(0), _22(0), _23(0), _24(0)
        , _31(0), _32(0), _33(0), _34(0)
    {}

    constexpr matrix3x4(float4 r0, float4 r1, float4 r2)
        : _11(r0.x), _12(r0.y), _13(r0.z), _14(r0.w)
        , _21(r1.x), _22(r1.y), _23(r1.z), _24(r1.w)
        , _31(r2.x), _32(r2.y), _33(r2.z), _34(r2.w)
    {}
};

struct matrix
{
    union
    {
        float4 r[4];
        struct
        {
            float _11, _12, _13, _14;
            float _21, _22, _23, _24;
            float _31, _32, _33, _34;
            float _41, _42, _43, _44;
        };
        float m[4][4];
    };

    constexpr matrix()
        : _11(0), _12(0), _13(0), _14(0)
        , _21(0), _22(0), _23(0), _24(0)
        , _31(0), _32(0), _33(0), _34(0)
        , _41(0), _42(0), _43(0), _44(0)
    {}

    constexpr matrix(float4 r0, float4 r1, float4 r2, float4 r3)
        : _11(r0.x), _12(r0.y), _13(r0.z), _14(r0.w)
        , _21(r1.x), _22(r1.y), _23(r1.z), _24(r1.w)
        , _31(r2.x), _32(r2.y), _33(r2.z), _34(r2.w)
        , _41(r3.x), _42(r3.y), _43(r3.z), _44(r3.w)
    {}

    constexpr matrix(   float __11, float __12, float __13, float __14,
                        float __21, float __22, float __23, float __24,
                        float __31, float __32, float __33, float __34,
                        float __41, float __42, float __43, float __44 )
        : _11(__11), _12(__12), _13(__13), _14(__14)
        , _21(__21), _22(__22), _23(__23), _24(__24)
        , _31(__31), _32(__32), _33(__33), _34(__34)
        , _41(__41), _42(__42), _43(__43), _44(__44)
    {}
};

constexpr float3 k_Vec3Zero = float3(0);
constexpr float3 k_HalfF3 = float3(0.5f);
constexpr float3 k_FLTMAXF3 = float3(FLT_MAX);
constexpr float3 k_FLTMINF3 = float3(-FLT_MAX);

constexpr float4 k_Vec4Zero = float4(0, 0, 0, 0);
constexpr float4 K_IdentityR0 = float4(1, 0, 0, 0);
constexpr float4 K_IdentityR1 = float4(0, 1, 0, 0);
constexpr float4 K_IdentityR2 = float4(0, 0, 1, 0);
constexpr float4 K_IdentityR3 = float4(0, 0, 0, 1);
constexpr float4 k_FLTMAXF4 = float4(FLT_MAX);
constexpr float4 k_FLTMINF4 = float4(-FLT_MAX);
constexpr float4 k_HalfF4 = float4(0.5f);

inline constexpr bool operator==(float3 lhs, float3 rhs) noexcept
{
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

inline constexpr float3 operator*(float3 lhs, float rhs) noexcept
{
    return float3(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs);
}

inline constexpr float3 operator*(float3 lhs, float3 rhs) noexcept
{
    return float3(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z);
}

inline constexpr float4 operator*(float4 lhs, float rhs) noexcept
{
    return float4(lhs.x * rhs, lhs.y * rhs, lhs.z * rhs, lhs.w * rhs);
}

inline constexpr float4 operator*(float4 lhs, float4 rhs) noexcept
{
    return float4(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w);
}

inline constexpr float3 operator+(float3 lhs, float3 rhs) noexcept
{
    return float3(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z);
}

inline constexpr float4 operator+(float4 lhs, float4 rhs) noexcept
{
    return float4(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w);
}

inline constexpr float2 operator-(float2 lhs, float2 rhs) noexcept
{
    return float2(lhs.x - rhs.x, lhs.y - rhs.y);
}

inline constexpr float3 operator-(float3 lhs, float3 rhs) noexcept
{
    return float3(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z);
}

inline constexpr float4 operator-(float4 lhs, float4 rhs) noexcept
{
    return float4(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w);
}

inline constexpr bool operator!=(float4 lhs, float4 rhs) noexcept
{
    return lhs.x != rhs.x || lhs.y != rhs.y || lhs.z != rhs.z || lhs.w != rhs.w;
}

inline constexpr bool operator!=(float3 lhs, float3 rhs) noexcept
{
    return lhs.x != rhs.x || lhs.y != rhs.y || lhs.z != rhs.z;
}

inline constexpr bool operator<(float2 lhs, float2 rhs) noexcept
{
    return lhs.x < rhs.x || lhs.y < rhs.y;
}

inline constexpr bool operator<(float3 lhs, float3 rhs) noexcept
{
    return lhs.x < rhs.x || lhs.y < rhs.y || lhs.z < rhs.z;
}

inline constexpr bool operator>(float2 lhs, float2 rhs) noexcept
{
    return lhs.x > rhs.x || lhs.y > rhs.y;
}

inline constexpr bool operator>(float3 lhs, float3 rhs) noexcept
{
    return lhs.x > rhs.x || lhs.y > rhs.y || lhs.z > rhs.z;
}

inline constexpr matrix2 operator*(matrix2 lhs, float rhs) noexcept
{
    matrix2 m;
    m._11 = lhs._11 * rhs;
    m._12 = lhs._12 * rhs;
    m._21 = lhs._21 * rhs;
    m._22 = lhs._22 * rhs;

    return m;
}

inline constexpr matrix2 operator*(matrix2 lhs, matrix2 rhs) noexcept
{
    return matrix2(
        lhs._11 * rhs._11 + lhs._12 * rhs._21,
        lhs._11 * rhs._12 + lhs._12 * rhs._22,
        lhs._21 * rhs._11 + lhs._22 * rhs._21,
        lhs._21 * rhs._12 + lhs._22 * rhs._22
    );
}

inline constexpr matrix2x3 operator*(matrix2 lhs, matrix2x3 rhs) noexcept
{
    matrix2x3 m;

    m._11 = lhs._11 * rhs._11 + lhs._12 * rhs._21;
    m._12 = lhs._11 * rhs._12 + lhs._12 * rhs._22;
    m._13 = lhs._11 * rhs._13 + lhs._12 * rhs._23;
    m._21 = lhs._21 * rhs._11 + lhs._22 * rhs._21;
    m._22 = lhs._21 * rhs._12 + lhs._22 * rhs._22;
    m._23 = lhs._21 * rhs._13 + lhs._22 * rhs._23;

    return m;
}

inline constexpr matrix operator*(matrix lhs, matrix rhs) noexcept
{
    matrix m;
    // Cache the invariants in registers
    float x = lhs.m[0][0];
    float y = lhs.m[0][1];
    float z = lhs.m[0][2];
    float w = lhs.m[0][3];
    // Perform the operation on the first row
    m.m[0][0] = (rhs.m[0][0] * x) + (rhs.m[1][0] * y) + (rhs.m[2][0] * z) + (rhs.m[3][0] * w);
    m.m[0][1] = (rhs.m[0][1] * x) + (rhs.m[1][1] * y) + (rhs.m[2][1] * z) + (rhs.m[3][1] * w);
    m.m[0][2] = (rhs.m[0][2] * x) + (rhs.m[1][2] * y) + (rhs.m[2][2] * z) + (rhs.m[3][2] * w);
    m.m[0][3] = (rhs.m[0][3] * x) + (rhs.m[1][3] * y) + (rhs.m[2][3] * z) + (rhs.m[3][3] * w);
    // Repeat for all the other rows
    x = lhs.m[1][0];
    y = lhs.m[1][1];
    z = lhs.m[1][2];
    w = lhs.m[1][3];
    m.m[1][0] = (rhs.m[0][0] * x) + (rhs.m[1][0] * y) + (rhs.m[2][0] * z) + (rhs.m[3][0] * w);
    m.m[1][1] = (rhs.m[0][1] * x) + (rhs.m[1][1] * y) + (rhs.m[2][1] * z) + (rhs.m[3][1] * w);
    m.m[1][2] = (rhs.m[0][2] * x) + (rhs.m[1][2] * y) + (rhs.m[2][2] * z) + (rhs.m[3][2] * w);
    m.m[1][3] = (rhs.m[0][3] * x) + (rhs.m[1][3] * y) + (rhs.m[2][3] * z) + (rhs.m[3][3] * w);
    x = lhs.m[2][0];
    y = lhs.m[2][1];
    z = lhs.m[2][2];
    w = lhs.m[2][3];
    m.m[2][0] = (rhs.m[0][0] * x) + (rhs.m[1][0] * y) + (rhs.m[2][0] * z) + (rhs.m[3][0] * w);
    m.m[2][1] = (rhs.m[0][1] * x) + (rhs.m[1][1] * y) + (rhs.m[2][1] * z) + (rhs.m[3][1] * w);
    m.m[2][2] = (rhs.m[0][2] * x) + (rhs.m[1][2] * y) + (rhs.m[2][2] * z) + (rhs.m[3][2] * w);
    m.m[2][3] = (rhs.m[0][3] * x) + (rhs.m[1][3] * y) + (rhs.m[2][3] * z) + (rhs.m[3][3] * w);
    x = lhs.m[3][0];
    y = lhs.m[3][1];
    z = lhs.m[3][2];
    w = lhs.m[3][3];
    m.m[3][0] = (rhs.m[0][0] * x) + (rhs.m[1][0] * y) + (rhs.m[2][0] * z) + (rhs.m[3][0] * w);
    m.m[3][1] = (rhs.m[0][1] * x) + (rhs.m[1][1] * y) + (rhs.m[2][1] * z) + (rhs.m[3][1] * w);
    m.m[3][2] = (rhs.m[0][2] * x) + (rhs.m[1][2] * y) + (rhs.m[2][2] * z) + (rhs.m[3][2] * w);
    m.m[3][3] = (rhs.m[0][3] * x) + (rhs.m[1][3] * y) + (rhs.m[2][3] * z) + (rhs.m[3][3] * w);
    return m;
}

// Util
template<typename T>
inline constexpr T DivideRoundUp(T a, T b) noexcept { return (a + (b - T(1))) / b; }

template<typename T>
inline constexpr T Sqr(T t) noexcept { return t * t; }

inline constexpr float ConvertToRadians(float angle) noexcept { return angle * (K_PI / 180.0f); }
inline constexpr bool IsAnyInf(float3 f3) noexcept { return ISINF(f3.x) || ISINF(f3.y) || ISINF(f3.z); }
inline bool ScalarNearEqual(float s1, float s2, float e) noexcept { return fabsf(s1 - s2) <= e; }

template<typename T> inline constexpr T Max(T a, T b) noexcept { return a > b ? a : b; }
template<typename T> inline constexpr T Min(T a, T b) noexcept { return a < b ? a : b; }
template<typename T> inline constexpr T Clamp(T v, T min, T max) noexcept { return Max(Min(v, max), min); }

inline constexpr float2 MaxF2(float2 a, float2 b) noexcept { return float2(Max(a.x, b.x), Max(a.y, b.y)); }
inline constexpr float2 MinF2(float2 a, float2 b) noexcept { return float2(Min(a.x, b.x), Min(a.y, b.y)); }
inline constexpr float2 ClampF2(float2 a, float2 l, float2 u) { return MinF2(MaxF2(a, l), u); }

inline constexpr float3 MaxF3(float3 a, float3 b) noexcept { return float3(Max(a.x, b.x), Max(a.y, b.y), Max(a.z, b.z)); }
inline constexpr float3 MinF3(float3 a, float3 b) noexcept { return float3(Min(a.x, b.x), Min(a.y, b.y), Min(a.z, b.z)); }

inline constexpr float4 MaxF4(float4 a, float4 b) noexcept { return float4(Max(a.x, b.x), Max(a.y, b.y), Max(a.z, b.z), Max(a.w, b.w)); }
inline constexpr float4 MinF4(float4 a, float4 b) noexcept { return float4(Min(a.x, b.x), Min(a.y, b.y), Min(a.z, b.z), Min(a.w, b.w)); }

inline constexpr float MinElemF4(float4 v) noexcept { return Min(v.x, Min(v.y, Min(v.z, v.w))); }

inline float2 FloorF2(float2 v) noexcept { return float2(floorf(v.x), floorf(v.y)); }
inline float4 FloorF4(float4 v) noexcept { return float4(floorf(v.x), floorf(v.y), floorf(v.z), floorf(v.w)); }

inline u32 FloorToUint(float a) noexcept { return static_cast<u32>(floorf(a)); }
inline u32 CeilToUint(float a) noexcept { return static_cast<u32>(ceilf(a)); }
inline u32 FloorLog2(u32 n) noexcept
{
    u32 ret = 0;
    while (n >>= 1) ++ret;
    return ret;
}

// Vector
inline constexpr float3 MultiplyAddF3(float3 a, float3 b, float3 c) noexcept
{
    return float3(
        a.x * b.x + c.x,
        a.y * b.y + c.y,
        a.z * b.z + c.z
    );
}

inline constexpr float4 MultiplyAddF4(float4 a, float4 b, float4 c) noexcept
{
    return float4(
        a.x * b.x + c.x,
        a.y * b.y + c.y,
        a.z * b.z + c.z,
        a.w * b.w + c.w
    );
}

inline constexpr float3 NegativeMultiplaySubtractF3(float3 a, float3 b, float3 c) noexcept
{
    return float3(
        c.x - (a.x * b.x),
        c.y - (a.y * b.y),
        c.z - (a.z * b.z)
    );
}

inline constexpr float4 NegativeMultiplaySubtractF4(float4 a, float4 b, float4 c) noexcept
{
    return float4(
        c.x - (a.x * b.x),
        c.y - (a.y * b.y),
        c.z - (a.z * b.z),
        c.w - (a.w * b.w)
    );
}

inline constexpr float3 NegateF3(float3 f3) noexcept
{
    return float3(-f3.x, -f3.y, -f3.z);
}

inline constexpr float4 MergeXYF4(float4 a, float4 b) noexcept
{
    return float4(a.x, b.x, a.y, b.y);
}

inline constexpr float4 MergeZWF4(float4 a, float4 b) noexcept
{
    return float4(a.z, b.z, a.w, b.w);
}

inline constexpr float3 CrossF3(float3 a, float3 b) noexcept
{
    return float3(
        (a.y * b.z) - (a.z * b.y),
        (a.z * b.x) - (a.x * b.z),
        (a.x * b.y) - (a.y * b.x)
    );
}

inline constexpr float DotF3(float3 a, float3 b) noexcept
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline constexpr float DotF4(float4 a, float4 b) noexcept
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

inline constexpr float LengthSqrF3(float3 f3) noexcept
{
    return DotF3(f3, f3);
}

inline float LengthF3(float3 f3) noexcept
{
    return sqrtf(LengthSqrF3(f3));
}

inline float DistSqrF3(float3 a, float3 b)
{
    return LengthSqrF3(b - a);
}

inline float DistF3(float3 a, float3 b)
{
    return LengthF3(b - a);
}

inline float3 NormalizeF3(float3 f3) noexcept
{
    float length = LengthF3(f3);
    if (length > 0) length = 1.0f / length;
    return float3(f3.x * length, f3.y * length, f3.z * length);
}

inline float3 TransformF3(float3 v, matrix m) noexcept
{
    float4 z(v.z);
    float4 y(v.y);
    float4 x(v.x);

    float4 result;
    result = MultiplyAddF4(z, m.r[2], m.r[3]);
    result = MultiplyAddF4(y, m.r[1], result);
    result = MultiplyAddF4(x, m.r[0], result);

    return result.xyz;
}

inline float4 TransformF4(float4 v, matrix m) noexcept
{
    float fX = (m.m[0][0] * v.v[0]) + (m.m[1][0] * v.v[1]) + (m.m[2][0] * v.v[2]) + (m.m[3][0] * v.v[3]);
    float fY = (m.m[0][1] * v.v[0]) + (m.m[1][1] * v.v[1]) + (m.m[2][1] * v.v[2]) + (m.m[3][1] * v.v[3]);
    float fZ = (m.m[0][2] * v.v[0]) + (m.m[1][2] * v.v[1]) + (m.m[2][2] * v.v[2]) + (m.m[3][2] * v.v[3]);
    float fW = (m.m[0][3] * v.v[0]) + (m.m[1][3] * v.v[1]) + (m.m[2][3] * v.v[2]) + (m.m[3][3] * v.v[3]);
    return float4(fX, fY, fZ, fW);
}

inline constexpr matrix3x4 MakeMatrix3x4(matrix m) noexcept
{
    return matrix3x4(m.r[0], m.r[1], m.r[2]);
}

// Matrix
inline constexpr matrix MakeMatrixIdentity() noexcept
{
    return matrix(K_IdentityR0, K_IdentityR1, K_IdentityR2, K_IdentityR3);
}

inline constexpr matrix MakeMatrixTranslation(float x, float y, float z) noexcept
{
    return matrix(
        K_IdentityR0 + float4(0, 0, 0, x),
        K_IdentityR1 + float4(0, 0, 0, y),
        K_IdentityR2 + float4(0, 0, 0, z),
        K_IdentityR3
    );
}

inline constexpr matrix MakeMatrixTranslation(float3 v) noexcept
{
    return MakeMatrixTranslation(v.x, v.y, v.z);
}

inline constexpr matrix MakeMatrixScaling(float x, float y, float z) noexcept
{
    return matrix(K_IdentityR0 * x, K_IdentityR1 * y, K_IdentityR2 * z, K_IdentityR3);
}

inline matrix MakeMatrixRotationNormal(float3 normal, float angleRadians) noexcept
{
    float sinAngle = sinf(angleRadians);
    float cosAngle = cosf(angleRadians);

    float3 A(sinAngle, cosAngle, 1.0f - cosAngle);

    float3 C2(A.z);
    float3 C1(A.y);
    float3 C0(A.x);

    float3 N0(normal.y, normal.z, normal.x);
    float3 N1(normal.z, normal.x, normal.y);

    float3 V0 = (C2 * N0) * N1;
    float3 R0 = MultiplyAddF3(C2 * normal, normal, C1);

    float3 R1 = MultiplyAddF3(C0, normal, V0);
    float3 R2 = NegativeMultiplaySubtractF3(C0, normal, V0);

    V0 = R0;
    float4 V1(R1.z, R2.y, R2.z, R1.x);
    float4 V2(R1.y, R2.x, R1.y, R2.x);

    matrix m;
    m.r[0] = float4(V0.x, V1.x, V1.y, 0);
    m.r[1] = float4(V1.z, V0.y, 0, 0);
    m.r[2] = float4(V2.x, V2.y, V0.z, 0);
    m.r[3] = K_IdentityR3;

    return m;
}

inline matrix MakeMatrixRotationAxis(float3 axis, float angleRadians) noexcept
{
    assert(axis != k_Vec3Zero);
    assert(!IsAnyInf(axis));

    float3 normal = NormalizeF3(axis);
    return MakeMatrixRotationNormal(normal, angleRadians);
}

inline matrix MakeMatrixRotationFromVector(float3 v) noexcept
{
    float cp = cosf(v.x);
    float sp = sinf(v.x);
    float cy = cosf(v.y);
    float sy = sinf(v.y);
    float cr = cosf(v.z);
    float sr = sinf(v.z);

    matrix m;
    m.m[0][0] = cr * cy + sr * sp * sy;
    m.m[0][1] = sr * cp;
    m.m[0][2] = sr * sp * cy - cr * sy;
    m.m[0][3] = 0.0f;

    m.m[1][0] = cr * sp * sy - sr * cy;
    m.m[1][1] = cr * cp;
    m.m[1][2] = sr * sy + cr * sp * cy;
    m.m[1][3] = 0.0f;

    m.m[2][0] = cp * sy;
    m.m[2][1] = -sp;
    m.m[2][2] = cp * cy;
    m.m[2][3] = 0.0f;

    m.m[3][0] = 0.0f;
    m.m[3][1] = 0.0f;
    m.m[3][2] = 0.0f;
    m.m[3][3] = 1.0f;

    return m;
}

inline matrix MakeMatrixRotationFromQuaternion(float4 quaternion)
{
    float4 q0 = quaternion + quaternion;
    float4 q1 = quaternion * q0;

    float4 v0 = float4{ q1.y, q1.x, q1.x, 0.0f };
    float4 v1 = float4{ q1.z, q1.z, q1.y, 0.0f };
    float4 r0 = float4{ 1.0f, 1.0f, 1.0f, 0.0f } - v0;
    r0 -= v1;

    v0 = float4{ quaternion.x, quaternion.x, quaternion.y, quaternion.w };
    v1 = float4{ q0.z, q0.y, q0.z, q0.w };
    v0 *= v1;

    v1 = float4{ quaternion.w };
    float4 v2 = float4{ q0.y, q0.z, q0.x, q0.w };
    v1 *= v2;

    float4 r1 = v0 + v1;
    float4 r2 = v0 - v1;

    v0 = float4{ r1.y, r2.x, r2.y, r1.z };
    v1 = float4{ r1.x, r2.z, r1.x, r2.z };

    return matrix{
        float4{r0.x, v0.x, v0.y, r0.w},
        float4{v0.z, r0.y, v0.w, r0.w},
        float4{v1.x, v1.y, r0.z, r0.w},
        float4{0.0f, 0.0f, 0.0f, 1.0f}
    };
}

inline constexpr matrix TransposeMatrix(matrix m) noexcept
{
    matrix p;
    p.r[0] = MergeXYF4(m.r[0], m.r[2]);
    p.r[1] = MergeXYF4(m.r[1], m.r[3]);
    p.r[2] = MergeZWF4(m.r[0], m.r[2]);
    p.r[3] = MergeZWF4(m.r[1], m.r[3]);

    matrix mt;
    mt.r[0] = MergeXYF4(p.r[0], p.r[1]);
    mt.r[1] = MergeZWF4(p.r[0], p.r[1]);
    mt.r[2] = MergeXYF4(p.r[2], p.r[3]);
    mt.r[3] = MergeZWF4(p.r[2], p.r[3]);

    return mt;
}

inline constexpr float Matrix2Determinant(matrix2 m) noexcept
{
    return 1.0f / (m._11 * m._22 - m._12 * m._21);
}

inline constexpr matrix2 Matrix2Adjugate(matrix2 m) noexcept
{
    return matrix2(m._22, -m._12, -m._21, m._11);
}

inline matrix2 InverseMatrix2(matrix2 m, float* outDeterminant = nullptr) noexcept
{
    const float det = Matrix2Determinant(m);
    if (outDeterminant != nullptr)
        *outDeterminant = det;
    return Matrix2Adjugate(m) * det;
}

#define SWIZZLEF4(f, c0, c1, c2, c3) float4(f.c0, f.c1, f.c2, f.c3)

inline matrix InverseMatrix(matrix m, float* outDeterminant = nullptr) noexcept
{
    matrix mt = TransposeMatrix(m);

    float4 v0[4], v1[4];
    v0[0] = SWIZZLEF4(mt.r[2], x, x, y, y);
    v1[0] = SWIZZLEF4(mt.r[3], z, w, z, w);
    v0[1] = SWIZZLEF4(mt.r[0], x, x, y, y);
    v1[1] = SWIZZLEF4(mt.r[1], z, w, z, w);
    v0[2] = float4(mt.r[2].x, mt.r[2].z, mt.r[0].x, mt.r[0].z);
    v1[2] = float4(mt.r[3].y, mt.r[3].w, mt.r[1].y, mt.r[1].w);

    float4 d0 = v0[0] * v1[0];
    float4 d1 = v0[1] * v1[1];
    float4 d2 = v0[2] * v1[2];

    v0[0] = SWIZZLEF4(mt.r[2], z, w, z, w);
    v1[0] = SWIZZLEF4(mt.r[3], x, x, y, y);
    v0[1] = SWIZZLEF4(mt.r[0], z, w, z, w);
    v1[1] = SWIZZLEF4(mt.r[1], x, x, y, y);
    v0[2] = float4(mt.r[2].y, mt.r[2].w, mt.r[0].y, mt.r[0].w);
    v1[2] = float4(mt.r[3].x, mt.r[3].z, mt.r[1].x, mt.r[1].z);

    d0 = NegativeMultiplaySubtractF4(v0[0], v1[0], d0);
    d1 = NegativeMultiplaySubtractF4(v0[1], v1[1], d1);
    d2 = NegativeMultiplaySubtractF4(v0[2], v1[2], d2);

    v0[0] = SWIZZLEF4(mt.r[1], y, z, x, y);
    v1[0] = float4(d2.y, d0.y, d0.w, d0.x);
    v0[1] = SWIZZLEF4(mt.r[0], z, x, y, x);
    v1[1] = float4(d0.w, d2.y, d0.y, d0.z);
    v0[2] = SWIZZLEF4(mt.r[3], y, z, x, y);
    v1[2] = float4(d2.w, d1.y, d1.w, d1.x);
    v0[3] = SWIZZLEF4(mt.r[2], z, x, y, x);
    v1[3] = float4(d1.w, d2.w, d1.y, d1.z);

    float4 c0 = v0[0] * v1[0];
    float4 c2 = v0[1] * v1[1];
    float4 c4 = v0[2] * v1[2];
    float4 c6 = v0[3] * v1[3];

    v0[0] = SWIZZLEF4(mt.r[1], z, w, y, z);
    v1[0] = float4(d0.w, d0.x, d0.y, d2.x);
    v0[1] = SWIZZLEF4(mt.r[0], w, z, w, y);
    v1[1] = float4(d0.z, d0.y, d2.x, d0.x);
    v0[2] = SWIZZLEF4(mt.r[3], z, w, y, z);
    v1[2] = float4(d1.w, d1.x, d1.y, d2.z);
    v0[3] = SWIZZLEF4(mt.r[2], w, z, w, y);
    v1[3] = float4(d1.z, d1.y, d2.z, d1.x);

    c0 = NegativeMultiplaySubtractF4(v0[0], v1[0], c0);
    c2 = NegativeMultiplaySubtractF4(v0[1], v1[1], c2);
    c4 = NegativeMultiplaySubtractF4(v0[2], v1[2], c4);
    c6 = NegativeMultiplaySubtractF4(v0[3], v1[3], c6);

    v0[0] = SWIZZLEF4(mt.r[1], w, x, w, x);
    v1[0] = float4(d0.z, d2.y, d2.x, d0.z);
    v0[1] = SWIZZLEF4(mt.r[0], y, w, x, z);
    v1[1] = float4(d2.y, d0.x, d0.w, d2.x);
    v0[2] = SWIZZLEF4(mt.r[3], w, x, w, x);
    v1[2] = float4(d1.z, d2.w, d2.z, d1.z);
    v0[3] = SWIZZLEF4(mt.r[2], y, w, x, z);
    v1[3] = float4(d2.w, d1.x, d1.w, d2.z);

    float4 c1 = NegativeMultiplaySubtractF4(v0[0], v1[0], c0);
    float4 c3 = MultiplyAddF4(v0[1], v1[1], c2);
    float4 c5 = NegativeMultiplaySubtractF4(v0[2], v1[2], c4);
    float4 c7 = MultiplyAddF4(v0[3], v1[3], c6);

    c0 = MultiplyAddF4(v0[0], v1[0], c0);
    c2 = NegativeMultiplaySubtractF4(v0[1], v1[1], c2);
    c4 = MultiplyAddF4(v0[2], v1[2], c4);
    c6 = NegativeMultiplaySubtractF4(v0[3], v1[3], c6);

    matrix r;
    r.r[0] = float4(c0.x, c1.y, c0.z, c1.w);
    r.r[1] = float4(c2.x, c3.y, c2.z, c3.w);
    r.r[2] = float4(c4.x, c5.y, c4.z, c5.w);
    r.r[3] = float4(c6.x, c7.y, c6.z, c7.w);

    float determinant = DotF4(r.r[0], mt.r[0]);

    if (outDeterminant)
        *outDeterminant = determinant;

    float reciprocal = 1.0f / determinant;

    matrix result;
    result.r[0] = r.r[0] * reciprocal;
    result.r[1] = r.r[1] * reciprocal;
    result.r[2] = r.r[2] * reciprocal;
    result.r[3] = r.r[3] * reciprocal;

    return result;
}

inline matrix MakeMatrixLookToLH(float3 eyePos, float3 eyeDir, float3 up) noexcept
{
    assert(eyeDir != k_Vec3Zero);
    assert(!IsAnyInf(eyeDir));
    assert(up != k_Vec3Zero);
    assert(!IsAnyInf(up));

    float3 R2 = NormalizeF3(eyeDir);
    float3 R0 = CrossF3(up, R2);
    R0 = NormalizeF3(R0);

    float3 R1 = CrossF3(R2, R0);
    float3 NegEyePos = NegateF3(eyePos);

    float D0 = DotF3(R0, NegEyePos);
    float D1 = DotF3(R1, NegEyePos);
    float D2 = DotF3(R2, NegEyePos);

    matrix m;
    m.r[0] = float4(R0, D0);
    m.r[1] = float4(R1, D1);
    m.r[2] = float4(R2, D2);
    m.r[3] = K_IdentityR3;

    return TransposeMatrix(m);
}

inline matrix MakeMatrixLookAtLH(float3 eye, float3 at, float3 up) noexcept
{
    float3 eyeDir = at - eye;
    return MakeMatrixLookToLH(eye, eyeDir, up);
}

inline matrix MakeMatrixPerspectiveFovLH(float fovRadians, float aspectRatio, float nearZ, float farZ) noexcept
{
    assert(nearZ > 0 && farZ > 0);
    assert(!ScalarNearEqual(fovRadians, 0.0f, 0.00001f * 2.0f));
    assert(!ScalarNearEqual(aspectRatio, 0.0f, 0.00001f));
    assert(!ScalarNearEqual(farZ, nearZ, 0.00001f));

    float sinFov = sinf(fovRadians);
    float cosFov = cosf(fovRadians);

    float height = cosFov / sinFov;
    float width = height / aspectRatio;
    float range = farZ / (farZ - nearZ);

    matrix m;
    m.m[0][0] = width;
    m.m[0][1] = 0.0f;
    m.m[0][2] = 0.0f;
    m.m[0][3] = 0.0f;

    m.m[1][0] = 0.0f;
    m.m[1][1] = height;
    m.m[1][2] = 0.0f;
    m.m[1][3] = 0.0f;

    m.m[2][0] = 0.0f;
    m.m[2][1] = 0.0f;
    m.m[2][2] = range;
    m.m[2][3] = 1.0f;

    m.m[3][0] = 0.0f;
    m.m[3][1] = 0.0f;
    m.m[3][2] = -range * nearZ;
    m.m[3][3] = 0.0f;

    return m;
}

inline matrix makeMatrixOrthographicOffCentreLH(float left, float right, float bottom, float top, float nearZ, float farZ) noexcept
{
    assert(!ScalarNearEqual(right, left, 0.00001f));
    assert(!ScalarNearEqual(top, bottom, 0.00001f));
    assert(!ScalarNearEqual(nearZ, farZ, 0.00001f));

    float rcpWidth = 1.0f / (right - left);
    float rcpHeight = 1.0f / (top - bottom);
    float range = 1.0f / (farZ - nearZ);

    matrix m = matrix();
    m.m[0][0] = rcpWidth + rcpWidth;
    m.m[1][1] = rcpHeight + rcpHeight;
    m.m[2][2] = range;

    m.m[3][0] = -(left + right) * rcpWidth;
    m.m[3][1] = -(top + bottom) * rcpHeight;
    m.m[3][2] = -range * nearZ;
    m.m[3][3] = 1.0f;

    return m;
}

constexpr float3 k_BoxOffsets[8] =
{
    float3( -1.0f, -1.0f,  1.0f ),
    float3(  1.0f, -1.0f,  1.0f ),
    float3(  1.0f,  1.0f,  1.0f ),
    float3( -1.0f,  1.0f,  1.0f ),
    float3( -1.0f, -1.0f, -1.0f ),
    float3(  1.0f, -1.0f, -1.0f ),
    float3(  1.0f,  1.0f, -1.0f ),
    float3( -1.0f,  1.0f, -1.0f ),
};

struct AABB
{
    float3 mins;
    float3 maxs;

    constexpr AABB() : mins(FLT_MAX), maxs(-FLT_MAX) {}
    constexpr AABB(float3 _mins, float3 _maxs) : mins(_mins), maxs(_maxs) {}

    constexpr void Grow(float3 p) noexcept
    {
        mins = MinF3(p, mins);
        maxs = MaxF3(p, maxs);
    }

    constexpr void Grow(AABB a) noexcept
    {
        mins = MinF3(mins, a.mins);
        maxs = MaxF3(maxs, a.maxs);
    }

    constexpr bool Invalid() noexcept { return mins > maxs; }

    float3 Origin() const noexcept
    {
        return (mins + maxs) * 0.5f;
    }

    float3 Extents() const noexcept
    {
        return (maxs - mins) * 0.5f;
    }

    void Transform(const matrix& mat)
    {
        float3 centre = Origin();
        float3 extents = Extents();

        maxs = -FLT_MAX;
        mins = FLT_MAX;

        for (size_t i = 0; i < 8; i++)
            Grow(TransformF3(MultiplyAddF3(extents, k_BoxOffsets[i], centre), mat));
    }
};

struct BoundingBox
{
    float3 centre;
    float3 extents;

    constexpr BoundingBox() : centre(0), extents(0) {}
    constexpr BoundingBox(float3 _centre, float3 _extents) : centre(_centre), extents(_extents) {}
    BoundingBox(const AABB& aabb)
    {
        centre = aabb.Origin();
        extents = aabb.Extents();
    }

    void GetCorners(float3 corners[8]) const noexcept
    {
        for (size_t i = 0; i < 8; i++)
            corners[i] = MultiplyAddF3(extents, k_BoxOffsets[i], centre);
    }
};

struct BoundingFrustum
{
    static constexpr size_t CornerCount = 8;

    float3 centre;
    float4 orientation;

    // Slopes
    float right, left, top, bottom;

    float nearZ, farZ;

    BoundingFrustum(matrix projection)
    {
        CreateFromMatrix(projection);
    }

    inline void CreateFromMatrix(matrix projection) noexcept
    {
        static float4 HomogenousPoints[6] =
        {
            float4(1,  0, 1, 1), // right
            float4(-1,  0, 1, 1), // left
            float4(0,  1, 1, 1), // top
            float4(0, -1, 1, 1), // bottom

            float4(0, 0, 0, 1), // near
            float4(0, 0, 1, 1), // far
        };

        float determinant;
        matrix inverse = InverseMatrix(projection, &determinant);

        float4 points[6];
        for (size_t i = 0; i < 6; i++)
            points[i] = TransformF4(HomogenousPoints[i], inverse);

        centre = k_Vec3Zero;
        orientation = K_IdentityR3;

        points[0] = points[0] * (1.0f / points[0].z);
        points[1] = points[1] * (1.0f / points[1].z);
        points[2] = points[2] * (1.0f / points[2].z);
        points[3] = points[3] * (1.0f / points[3].z);

        right = points[0].x;
        left = points[1].x;
        top = points[2].y;
        bottom = points[3].y;

        points[4] = points[4] * (1.0f / points[4].w);
        points[5] = points[5] * (1.0f / points[5].w);

        nearZ = points[4].z;
        farZ = points[5].z;
    }
};

