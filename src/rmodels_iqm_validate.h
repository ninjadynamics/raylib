#ifndef RMODELS_IQM_VALIDATE_H
#define RMODELS_IQM_VALIDATE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* IQM stores four byte-sized influences per vertex. Zero-weight lanes are
 * semantically unused; absent index arrays retain raylib's joint-zero default. */
static inline bool rmodelsIQMWeightedBonesValid(const unsigned char *indices,
                                                const unsigned char *weights,
                                                size_t vertexCount,
                                                unsigned int jointCount)
{
    if (weights == NULL) return true;
    if (vertexCount > SIZE_MAX/4u) return false;

    const size_t valueCount = vertexCount*4u;
    for (size_t i = 0; i < valueCount; i++)
    {
        const unsigned int boneId = (indices != NULL)? indices[i] : 0u;
        if ((weights[i] != 0u) && (boneId >= jointCount)) return false;
    }

    return true;
}

#endif
