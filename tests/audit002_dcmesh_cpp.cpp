#define DREAMCAST
#include "../src/dc_mesh.h"

// Compile as C++ and inspect/link the undefined reference: extern "C" must
// keep the public DCMesh symbol unmangled for Dreamcast application objects.
void audit002_dcmesh_cpp_linkage(Mesh *mesh)
{
    dcMeshSyncColors(mesh);
}
