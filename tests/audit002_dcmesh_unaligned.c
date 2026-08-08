#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "../src/dc_mesh_color.h"

int main(void)
{
    unsigned char storage[9] = { 0 };
    unsigned char *unaligned = storage + 1;
    const unsigned char source[8] = { 0x11, 0x22, 0x33, 0x44, 0xaa, 0xbb, 0xcc, 0xdd };
    memcpy(unaligned, source, sizeof(source));

    assert(dcMeshUnalignedColorWord(unaligned) == UINT32_C(0x44332211));
    assert(dcMeshUnalignedColorWord(unaligned + 4) == UINT32_C(0xddccbbaa));
    return 0;
}
