#include "../src/dcmesh.h"

#include <string.h>

typedef char audit002_dcvertex_size[(sizeof(DCVertex) == 24)? 1 : -1];
typedef char audit002_dcstrip_size[(sizeof(DCStrip) == 8)? 1 : -1];
typedef char audit002_submesh_header_size[(sizeof(DCSubmeshHeader) == 16)? 1 : -1];
typedef char audit002_file_header_size[(sizeof(DCMeshFileHeader) == 32)? 1 : -1];

int main(void)
{
    const uint32_t magic = DCMESH_MAGIC;
    const unsigned char expected[4] = { '1', 'D', 'C', 'M' };
    return memcmp(&magic, expected, sizeof(expected)) == 0? 0 : 1;
}
