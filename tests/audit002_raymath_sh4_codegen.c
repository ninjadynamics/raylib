/* Cross-compile-only probe used to inspect the SH4 code generated for the
 * corrected path beside the pre-fix raw loader. */
#define USE_SH4ZAM
#define RAYMATH_STATIC_INLINE
#include "../src/raymath.h"

__attribute__((noinline)) Vector3 audit_transform_fixed(Vector3 v, Matrix m)
{
    return Vector3Transform(v, m);
}

__attribute__((noinline)) Vector3 audit_transform_prior_raw(Vector3 v, Matrix m)
{
    shz_xmtrx_load_unaligned_4x4((const float *)&m);
    shz_vec4_t sv = { .x = v.x, .y = v.y, .z = v.z, .w = 1.0f };
    shz_vec4_t sr = shz_xmtrx_transform_vec4(sv);
    return (Vector3){ sr.x, sr.y, sr.z };
}

__attribute__((noinline)) Vector3 audit_transform_direct(Vector3 v, Matrix m)
{
    shz_vec3_t sv = { .x = v.x, .y = v.y, .z = v.z };
    shz_vec3_t sr = shz_mat4x4_transform_point3_transpose(
        (const shz_mat4x4_t *)(const void *)&m, sv);
    return (Vector3){ sr.x, sr.y, sr.z };
}

__attribute__((noinline)) Vector3 audit_transform_rows(Vector3 v, Matrix m)
{
    shz_vec4_t point = { .x = v.x, .y = v.y, .z = v.z, .w = 1.0f };
    shz_vec4_t row0 = { .x = m.m0, .y = m.m4, .z = m.m8, .w = m.m12 };
    shz_vec4_t row1 = { .x = m.m1, .y = m.m5, .z = m.m9, .w = m.m13 };
    shz_vec4_t row2 = { .x = m.m2, .y = m.m6, .z = m.m10, .w = m.m14 };
    shz_vec3_t sr = shz_vec4_dot3(point, row0, row1, row2);
    return (Vector3){ sr.x, sr.y, sr.z };
}
