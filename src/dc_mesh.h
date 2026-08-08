/*
 * dc_mesh.h — Dreamcast DCMesh Runtime API
 *
 * Provides loading, drawing, and transparent routing for .dcmesh files.
 * Drop this file into your raylib src/ directory.
 *
 * Compile with -DPLATFORM_DREAMCAST (raylib) or -DDREAMCAST (applications).
 * Optional: -DENABLE_PATCH_E=1 to enable opaque-list routing.
 * Optional: -DENABLE_STRIPS=1 to enable strip rendering via GLdc.
 *
 * Both default to 1 for either Dreamcast target macro.
 */

#ifndef DC_MESH_H
#define DC_MESH_H

#if defined(PLATFORM_DREAMCAST) || defined(DREAMCAST)

#include "dcmesh.h"
#include "raylib.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Feature toggles — default ON for Dreamcast */
#ifndef ENABLE_STRIPS
#define ENABLE_STRIPS   1
#endif
#ifndef ENABLE_PATCH_E
#define ENABLE_PATCH_E  1
#endif

/* -------------------------------------------------------------------
 * Registry — maps raylib Mesh.vaoId to DCMeshData
 *
 * On Dreamcast GL 1.1, Mesh.vaoId is unused (VAOs are GL 3.3+).
 * We store DCMESH_MAKE_ID(index) in vaoId to link a Mesh to its
 * strip data. DrawMesh checks this to route to the fast path.
 * ---------------------------------------------------------------- */
#define DC_MESH_REGISTRY_MAX  64

/* -------------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------- */

/* Try to load a .dcmesh sidecar for the given model file.
 * LoadModel() calls this automatically when strip support is enabled;
 * direct callers may use it for models assembled by other loaders. A
 * second call on an already fully-linked model is idempotent.
 * Returns 1 if strip data was loaded, 0 if not found or failed.
 *
 * The function modifies model->meshes[].vaoId to link each mesh
 * to its strip data in the registry. */
RLAPI int dcMeshLoadSidecar(Model *model, const char *modelPath);

/* Draw a mesh using strip/Patch E path if available, or fall back
 * to standard GLdc path. This is the replacement for DrawMesh()
 * on Dreamcast — called from the transparent routing hook. */
RLAPI void dcMeshDraw(Mesh mesh, Material material, Matrix transform);

/* Explicit batch path for many instances of one dcmesh-backed material.
 * Begin binds texture/client arrays once, each Draw submits one triangle array
 * with its own transform/tint, and End tears state down. Returns 1 if the
 * model/material has dcmesh strip data and the batch was opened. */
RLAPI int dcMeshBatchBegin(Model model, int materialIndex);
RLAPI void dcMeshBatchDraw(Matrix transform, Color tint);
RLAPI void dcMeshBatchEnd(void);

/* Unload all DCMesh data associated with a model. UnloadModel() and
 * UnloadMesh() call the corresponding hooks automatically. */
RLAPI void dcMeshUnloadModel(Model *model);
RLAPI void dcMeshUnloadMesh(Mesh *mesh);

/* Check if a mesh has DCMesh strip data loaded. */
RLAPI int dcMeshHasStripData(Mesh mesh);

/* UploadMesh hook — if mesh has dcmesh data, syncs positions and colors
 * from raylib arrays to strip vertices and returns 1.
 * Returns 0 if no dcmesh data (caller runs normal UploadMesh).
 * Game code should NOT call this directly — it's called from the
 * UploadMesh hook in rmodels.c automatically. */
RLAPI int dcMeshHandleUpload(Mesh *mesh, bool dynamic);

/* Native color contract: Dreamcast mesh color producers store bytes in
 * GLdc's native BGRA order. DCMesh sidecars and synchronization deliberately
 * copy that packed uint32_t without a runtime channel conversion. */
RLAPI void dcMeshSyncColors(Mesh *mesh);   /* colors-only per-frame sync: no cache rebuild */

/* Print DCMesh registry stats (for debugging). */
RLAPI void dcMeshPrintRegistryStats(void);

/* Recenter DCMesh geometry — call right after recenter_model_geometry()
 * so strip vertices get the same offset as raylib mesh vertices.
 * Pass the SAME (dx, dy, dz) values — uses -= internally. */
RLAPI void dcMeshRecenterGeometry(Model *model, float offsetX, float offsetY, float offsetZ);

/* Compatibility wrapper for callers that explicitly upload a mesh. Tagged
 * meshes take the DCMesh synchronization hook; ordinary meshes take the
 * normal UploadMesh path. */
RLAPI void dcMeshUploadSafe(Mesh *mesh, bool dynamic);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_DREAMCAST || DREAMCAST */
#endif /* DC_MESH_H */
