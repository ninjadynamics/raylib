#ifndef AUDIT_DC_RLGL_GLKOS_H
#define AUDIT_DC_RLGL_GLKOS_H

#include "gl.h"

#if !defined(AUDIT_GLKOS_LEGACY)
#if defined(AUDIT_GLKOS_ABI_V1)
#define GL_KOS_FAST_PATH_ABI_VERSION 1u
#else
#define GL_KOS_FAST_PATH_ABI_VERSION 3u
#endif

#if defined(AUDIT_GLKOS_INTERLEAVED_FEATURE_ZERO)
#define GL_KOS_HAS_INTERLEAVED_P3T2BGRA 0
#else
#define GL_KOS_HAS_INTERLEAVED_P3T2BGRA 1
#endif

#if defined(AUDIT_GLKOS_FINAL_FEATURE_ZERO)
#define GL_KOS_HAS_FINAL_INTERLEAVED_P3T2BGRA 0
#else
#define GL_KOS_HAS_FINAL_INTERLEAVED_P3T2BGRA 1
#endif

#if defined(AUDIT_GLKOS_TRUSTED_FINAL_FEATURE_ZERO)
#define GL_KOS_HAS_TRUSTED_FINAL_INTERLEAVED_P3T2BGRA 0
#else
#define GL_KOS_HAS_TRUSTED_FINAL_INTERLEAVED_P3T2BGRA 1
#endif

#define GL_KOS_FAST_PATH_INTERLEAVED_P3T2BGRA (1u << 0)
#define GL_KOS_FAST_PATH_FINAL_INTERLEAVED_P3T2BGRA (1u << 6)
#define GL_KOS_FAST_PATH_TRUSTED_FINAL_INTERLEAVED_P3T2BGRA (1u << 8)
#define GL_KOS_FAST_PATH_CAPABILITIES \
    ((GL_KOS_HAS_INTERLEAVED_P3T2BGRA \
        ? GL_KOS_FAST_PATH_INTERLEAVED_P3T2BGRA : 0u) | \
     (GL_KOS_HAS_FINAL_INTERLEAVED_P3T2BGRA \
        ? GL_KOS_FAST_PATH_FINAL_INTERLEAVED_P3T2BGRA : 0u) | \
     (GL_KOS_HAS_TRUSTED_FINAL_INTERLEAVED_P3T2BGRA \
        ? GL_KOS_FAST_PATH_TRUSTED_FINAL_INTERLEAVED_P3T2BGRA : 0u))

typedef struct GLKosVertexP3T2BGRA {
    float x, y, z;
    float u, v;
    unsigned int bgra;
} GLKosVertexP3T2BGRA;

unsigned int glKosGetFastPathCapabilities(void);
GLboolean glKosTryDrawInterleavedP3T2BGRA(
    GLenum mode, const GLKosVertexP3T2BGRA *vertices, GLsizei count);
GLboolean glKosTryQueueFinalInterleavedP3T2BGRA(
    GLenum mode, const GLKosVertexP3T2BGRA *vertices, GLsizei count);
GLboolean glKosTryQueueTrustedFinalInterleavedP3T2BGRA(
    GLenum mode, const GLKosVertexP3T2BGRA *vertices, GLsizei count);
#if defined(GLDC_NATIVE_BENCH) && GLDC_NATIVE_BENCH
GLboolean glKosNativeBenchTryQueueTrustedFinalP3T2BGRA(
    GLenum mode, const GLKosVertexP3T2BGRA *vertices, GLsizei count);
#endif
#endif

void glKosDrawTrianglesArrays(int first, GLsizei count);

#endif
