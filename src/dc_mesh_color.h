#ifndef DC_MESH_COLOR_H
#define DC_MESH_COLOR_H

#include <stdint.h>

/* Preserve the native little-endian BGRA word without requiring aligned input. */
static inline uint32_t dcMeshUnalignedColorWord(const unsigned char *src)
{
    return (uint32_t)src[0] |
           ((uint32_t)src[1] << 8) |
           ((uint32_t)src[2] << 16) |
           ((uint32_t)src[3] << 24);
}

#endif
