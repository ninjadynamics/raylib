#include <assert.h>
#include <stddef.h>

#include "../src/rmodels_iqm_validate.h"

int main(void)
{
    const unsigned char indices[4] = { 0, 2, 255, 1 };
    const unsigned char weights[4] = { 255, 1, 0, 0 };
    const unsigned char zeroWeights[4] = { 0, 0, 0, 0 };

    assert(!rmodelsIQMWeightedBonesValid(indices, weights, 1, 1));
    assert(rmodelsIQMWeightedBonesValid(indices, zeroWeights, 1, 1));
    assert(rmodelsIQMWeightedBonesValid(NULL, weights, 1, 1));
    assert(rmodelsIQMWeightedBonesValid(indices, NULL, 1, 0));
    assert(!rmodelsIQMWeightedBonesValid(indices, weights, (SIZE_MAX/4u) + 1u, 1));
    return 0;
}
