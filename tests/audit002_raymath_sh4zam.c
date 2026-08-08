/*
 * Focused regression coverage for the Dreamcast SH4ZAM raymath paths.
 *
 * This intentionally includes raymath.h standalone: it catches SH4ZAM helper
 * declarations being placed before raymath's fallback public types. On a host
 * build SH4ZAM selects its software XMTRX backend, which has the same matrix
 * layout contract as the SH4 FTRV backend and can execute the comparisons.
 */
#define USE_SH4ZAM
#define RAYMATH_STATIC_INLINE
#include "../src/raymath.h"

#include <math.h>
#include <stdio.h>

/* The SH4 owns XMTRX in hardware. SH4ZAM's software backend declares this
 * thread-local stand-in so host-side differential tests can supply it. */
thread_local struct shz_xmtrx_ shz_xmtrx_state_;

static Vector3 scalar_transform(Vector3 v, Matrix m)
{
    return (Vector3){
        m.m0*v.x + m.m4*v.y + m.m8*v.z + m.m12,
        m.m1*v.x + m.m5*v.y + m.m9*v.z + m.m13,
        m.m2*v.x + m.m6*v.y + m.m10*v.z + m.m14
    };
}

static int close_float(float a, float b)
{
    return fabsf(a - b) <= 0.00001f;
}

static int check_transform(const char *name, Vector3 v, Matrix m)
{
    Vector3 expected = scalar_transform(v, m);
    Vector3 actual = Vector3Transform(v, m);

    if (!close_float(actual.x, expected.x) ||
        !close_float(actual.y, expected.y) ||
        !close_float(actual.z, expected.z))
    {
        fprintf(stderr, "%s: got {%g,%g,%g}, expected {%g,%g,%g}\n",
                name, actual.x, actual.y, actual.z,
                expected.x, expected.y, expected.z);
        return 0;
    }

    return 1;
}

int main(void)
{
    int ok = 1;
    Vector3 v = { 1.25f, -2.0f, 3.5f };
    Matrix arbitrary = {
         1.0f,  2.0f,  3.0f,  4.0f,
         5.0f,  6.0f,  7.0f,  8.0f,
         9.0f, 10.0f, 11.0f, 12.0f,
        13.0f, 14.0f, 15.0f, 16.0f
    };

    ok &= check_transform("identity", v, MatrixIdentity());
    ok &= check_transform("translation", v, MatrixTranslate(10.0f, 20.0f, 30.0f));
    ok &= check_transform("scale", v, MatrixScale(2.0f, 3.0f, 4.0f));
    ok &= check_transform("rotation-x", v, MatrixRotateX(0.37f));
    ok &= check_transform("rotation-y", v, MatrixRotateY(-0.81f));
    ok &= check_transform("rotation-z", v, MatrixRotateZ(1.19f));
    ok &= check_transform("arbitrary", v, arbitrary);

    Quaternion zero = QuaternionNormalize((Quaternion){ 0.0f, 0.0f, 0.0f, 0.0f });
    if ((zero.x != 0.0f) || (zero.y != 0.0f) ||
        (zero.z != 0.0f) || (zero.w != 0.0f))
    {
        fprintf(stderr, "zero quaternion normalization produced {%g,%g,%g,%g}\n",
                zero.x, zero.y, zero.z, zero.w);
        ok = 0;
    }

    return ok? 0 : 1;
}
