/*
 * dc_mesh.h — Dreamcast DCMesh Runtime API
 *
 * Provides loading, drawing, and transparent routing for .dcmesh files.
 * Drop this file into your raylib src/ directory.
 *
 * Compile with -DPLATFORM_DREAMCAST (already set by DC build).
 * Optional: -DENABLE_PATCH_E=1 to enable direct PVR submission.
 * Optional: -DENABLE_STRIPS=1 to enable strip rendering via GLdc.
 *
 * Both default to 1 when PLATFORM_DREAMCAST is defined.
 */

#ifndef DC_MESH_H
#define DC_MESH_H

#if defined(PLATFORM_DREAMCAST) || defined(DREAMCAST)

#include "dcmesh.h"
#include "raylib.h"

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
 * Call this after LoadModel(). If "model.glb" was loaded, this
 * checks for "model.dcmesh" and loads strip data if found.
 * Returns 1 if strip data was loaded, 0 if not found or failed.
 *
 * The function modifies model->meshes[].vaoId to link each mesh
 * to its strip data in the registry. */
int dcMeshLoadSidecar(Model *model, const char *modelPath);

/* Draw a mesh using strip/Patch E path if available, or fall back
 * to standard GLdc path. This is the replacement for DrawMesh()
 * on Dreamcast — called from the transparent routing hook. */
void dcMeshDraw(Mesh mesh, Material material, Matrix transform);

/* Unload all DCMesh data associated with a model.
 * Call this before UnloadModel(). */
void dcMeshUnloadModel(Model *model);

/* Check if a mesh has DCMesh strip data loaded. */
int dcMeshHasStripData(Mesh mesh);

/* Print DCMesh registry stats (for debugging). */
void dcMeshPrintRegistryStats(void);

/* Recenter DCMesh geometry — call right after recenter_model_geometry()
 * so strip vertices get the same offset as raylib mesh vertices.
 * Pass the SAME (dx, dy, dz) values — uses -= internally. */
void dcMeshRecenterGeometry(Model *model, float offsetX, float offsetY, float offsetZ);

/* Sync per-vertex colors from raylib mesh to DCMesh strip vertices.
 * Call after light_player_model() or any CPU per-vertex coloring.
 * Converts raylib RGBA to dcmesh BGRA automatically. */
void dcMeshSyncColors(Model *model);

/* Safe UploadMesh wrapper — temporarily clears the dcmesh vaoId tag
 * to suppress "already loaded" warnings, then restores it.
 * Use instead of UploadMesh() for meshes with dcmesh data. */
void dcMeshUploadSafe(Mesh *mesh, bool dynamic);

#endif /* PLATFORM_DREAMCAST */
#endif /* DC_MESH_H */
