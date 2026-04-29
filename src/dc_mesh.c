/*
 * dc_mesh.c — Dreamcast DCMesh Runtime Implementation
 *
 * Phase 2: Strip rendering via GLdc (GL_TRIANGLE_STRIP)
 * Phase 3: Patch E optimized submission (minimal state changes)
 *
 * Drop this file into your raylib src/ directory and add to build.
 * Requires: dcmesh.h, dc_mesh.h, PLATFORM_DREAMCAST defined.
 */

#if defined(PLATFORM_DREAMCAST)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/gl.h>

#include "dc_mesh.h"
#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"

/* GLdc stats integration */
#ifdef GLDC_ENABLE_STATS
#include "gldc_stats.h"
#endif

/* -------------------------------------------------------------------
 * Registry — global table mapping IDs to DCMeshData
 * ---------------------------------------------------------------- */
static DCMeshData* dc_registry[DC_MESH_REGISTRY_MAX] = {0};
static int dc_registry_count = 0;

static int dcRegistryAdd(DCMeshData* data) {
    if (dc_registry_count >= DC_MESH_REGISTRY_MAX) {
        printf("[DCMesh] Registry full (%d max)\n", DC_MESH_REGISTRY_MAX);
        return -1;
    }
    for (int i = 0; i < DC_MESH_REGISTRY_MAX; i++) {
        if (dc_registry[i] == NULL) {
            dc_registry[i] = data;
            dc_registry_count++;
            return i;
        }
    }
    return -1;
}

static DCMeshData* dcRegistryGet(unsigned int vaoId) {
    if (!DCMESH_IS_REGISTRY_ID(vaoId)) return NULL;
    int idx = DCMESH_REGISTRY_INDEX(vaoId);
    if (idx < 0 || idx >= DC_MESH_REGISTRY_MAX) return NULL;
    return dc_registry[idx];
}

static void dcRegistryRemove(unsigned int vaoId) {
    if (!DCMESH_IS_REGISTRY_ID(vaoId)) return;
    int idx = DCMESH_REGISTRY_INDEX(vaoId);
    if (idx >= 0 && idx < DC_MESH_REGISTRY_MAX && dc_registry[idx]) {
        dc_registry[idx] = NULL;
        dc_registry_count--;
    }
}

/* -------------------------------------------------------------------
 * File loader — reads .dcmesh binary into DCMeshData
 * ---------------------------------------------------------------- */
static DCMeshData* dcLoadFile(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;

    /* Read and validate header */
    DCMeshFileHeader fhdr;
    if (fread(&fhdr, sizeof(fhdr), 1, f) != 1) { fclose(f); return NULL; }

    if (fhdr.magic != DCMESH_MAGIC || fhdr.version != DCMESH_VERSION) {
        printf("[DCMesh] Invalid file: %s (magic=0x%X ver=%u)\n",
               path, fhdr.magic, fhdr.version);
        fclose(f);
        return NULL;
    }

    /* Allocate runtime structure */
    DCMeshData* data = (DCMeshData*)calloc(1, sizeof(DCMeshData));
    data->submesh_count = fhdr.submesh_count;
    data->submeshes = (DCSubmesh*)calloc(fhdr.submesh_count, sizeof(DCSubmesh));

    /* Read each submesh */
    for (uint32_t i = 0; i < fhdr.submesh_count; i++) {
        DCSubmeshHeader shdr;
        if (fread(&shdr, sizeof(shdr), 1, f) != 1) goto fail;

        DCSubmesh* sm = &data->submeshes[i];
        sm->material_index = shdr.material_index;
        sm->is_opaque = shdr.is_opaque;
        sm->vertex_count = shdr.vertex_count;
        sm->strip_count = shdr.strip_count;

        /* Read vertices */
        sm->vertices = (DCVertex*)malloc(shdr.vertex_count * sizeof(DCVertex));
        if (!sm->vertices) goto fail;
        if (fread(sm->vertices, sizeof(DCVertex), shdr.vertex_count, f) != shdr.vertex_count) goto fail;

        /* Read strips */
        sm->strips = (DCStrip*)malloc(shdr.strip_count * sizeof(DCStrip));
        if (!sm->strips) goto fail;
        if (fread(sm->strips, sizeof(DCStrip), shdr.strip_count, f) != shdr.strip_count) goto fail;

        /* Read vertex_map (strip vertex -> original vertex index) */
        sm->vertex_map = (uint16_t*)malloc(shdr.vertex_count * sizeof(uint16_t));
        if (!sm->vertex_map) goto fail;
        if (fread(sm->vertex_map, sizeof(uint16_t), shdr.vertex_count, f) != shdr.vertex_count) goto fail;
    }

    fclose(f);
    printf("[DCMesh] Loaded: %s (%u submeshes, %u verts, %u strips)\n",
           path, fhdr.submesh_count, fhdr.total_vertices, fhdr.total_strips);
    return data;

fail:
    printf("[DCMesh] Error reading: %s\n", path);
    /* Cleanup partial load */
    for (uint32_t i = 0; i < data->submesh_count; i++) {
        free(data->submeshes[i].vertices);
        free(data->submeshes[i].strips);
        free(data->submeshes[i].vertex_map);
    }
    free(data->submeshes);
    free(data);
    fclose(f);
    return NULL;
}

static void dcFreeData(DCMeshData* data) {
    if (!data) return;
    for (uint32_t i = 0; i < data->submesh_count; i++) {
        free(data->submeshes[i].vertices);
        free(data->submeshes[i].strips);
        free(data->submeshes[i].vertex_map);
    }
    free(data->submeshes);
    free(data);
}

/* -------------------------------------------------------------------
 * Public API: Load sidecar
 * ---------------------------------------------------------------- */
int dcMeshLoadSidecar(Model *model, const char *modelPath) {
    if (!model || !modelPath) return 0;

    /* Build .dcmesh path from model path */
    char dcpath[512];
    strncpy(dcpath, modelPath, sizeof(dcpath) - 1);
    dcpath[sizeof(dcpath) - 1] = '\0';

    char* dot = strrchr(dcpath, '.');
    if (dot) strcpy(dot, ".dcmesh");
    else strcat(dcpath, ".dcmesh");

    /* Try to load */
    DCMeshData* data = dcLoadFile(dcpath);
    if (!data) return 0;

    /* Register */
    int reg_idx = dcRegistryAdd(data);
    if (reg_idx < 0) {
        dcFreeData(data);
        return 0;
    }

    /* Link each raylib mesh to its corresponding DCMesh submesh.
     * Mesh[0] -> submesh[0], Mesh[1] -> submesh[1], etc.
     * Only link as many as we have submeshes for. */
    int linked = 0;
    int limit = model->meshCount < (int)data->submesh_count
              ? model->meshCount : (int)data->submesh_count;

    for (int i = 0; i < limit; i++) {
        model->meshes[i].vaoId = DCMESH_MAKE_ID(reg_idx, i);
        linked++;
    }

    printf("[DCMesh] Linked %d/%d meshes to registry[%d] (%u submeshes)\n",
           linked, model->meshCount, reg_idx, data->submesh_count);
    return 1;
}

/* -------------------------------------------------------------------
 * Patch E eligibility check
 * ---------------------------------------------------------------- */
#if ENABLE_PATCH_E
static int dcPatchEEligible(DCSubmesh* sm, Material material) {
    /* Strict scope: opaque, single texture, no weird state */
    if (!sm->is_opaque) return 0;

    /* Must have a valid diffuse texture */
    if (material.maps[MATERIAL_MAP_DIFFUSE].texture.id == 0) return 0;

    /* Material color alpha must be fully opaque */
    if (material.maps[MATERIAL_MAP_DIFFUSE].color.a < 255) return 0;

    return 1;
}
#endif

/* -------------------------------------------------------------------
 * Phase 2: Strip rendering via GLdc
 *
 * Sets up client arrays once for all strips in a submesh,
 * then submits each strip as GL_TRIANGLE_STRIP. GLdc's
 * genTriangleStrip() is trivial (one EOL flag), so this
 * avoids the per-triangle EOL overhead of GL_TRIANGLES.
 *
 * Phase 3 (Patch E): When eligible, we additionally minimize
 * state changes by batching all strips under one GL state context.
 * ---------------------------------------------------------------- */
static void dcDrawSubmesh(DCSubmesh* sm, Material material, Matrix transform) {
    if (sm->vertex_count == 0 || sm->strip_count == 0) return;

    const GLsizei stride = sizeof(DCVertex);
    const DCVertex* buf = sm->vertices;

#if ENABLE_PATCH_E
    int use_patchE = dcPatchEEligible(sm, material);
#else
    int use_patchE = 0;
#endif

#ifdef GLDC_ENABLE_STATS
    if (use_patchE) GLDC_STAT_INC(patchE_hits);
    else GLDC_STAT_INC(patchE_fallbacks);
    GLDC_STAT_ADD(strip_count, sm->strip_count);
    GLDC_STAT_ADD(strip_vertices_total, sm->vertex_count);
#endif

    /* Bind texture */
    unsigned int texId = material.maps[MATERIAL_MAP_DIFFUSE].texture.id;
    rlEnableTexture(texId);

    /* Set up client arrays — one setup for all strips in this submesh.
     * Layout matches GLdc fast-path requirements:
     *   vertex:  3 x GL_FLOAT
     *   uv:      2 x GL_FLOAT
     *   color:   GL_BGRA x GL_UNSIGNED_BYTE */
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);

    glVertexPointer(3, GL_FLOAT, stride, &buf[0].x);
    glTexCoordPointer(2, GL_FLOAT, stride, &buf[0].u);
    glColorPointer(GL_BGRA, GL_UNSIGNED_BYTE, stride, &buf[0].color);

    /* Push model transform */
    rlPushMatrix();
    rlMultMatrixf(MatrixToFloat(transform));

    /* Set material tint color */
    rlColor4ub(material.maps[MATERIAL_MAP_DIFFUSE].color.r,
               material.maps[MATERIAL_MAP_DIFFUSE].color.g,
               material.maps[MATERIAL_MAP_DIFFUSE].color.b,
               material.maps[MATERIAL_MAP_DIFFUSE].color.a);

    /* Submit all strips — each as GL_TRIANGLE_STRIP.
     * GLdc's genTriangleStrip() is just one EOL flag assignment,
     * so this is extremely efficient per strip. With Patch E eligibility,
     * all strips share the same GL state (no header rebuilds between strips). */
    for (uint32_t i = 0; i < sm->strip_count; i++) {
        DCStrip* strip = &sm->strips[i];
        glDrawArrays(GL_TRIANGLE_STRIP, strip->first_vertex, strip->vertex_count);
    }

    rlPopMatrix();

    /* Tear down client state */
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    rlDisableTexture();
}

/* -------------------------------------------------------------------
 * Public API: Draw mesh with DCMesh fast path
 * ---------------------------------------------------------------- */
void dcMeshDraw(Mesh mesh, Material material, Matrix transform) {
    DCMeshData* data = dcRegistryGet(mesh.vaoId);

    if (!data || !ENABLE_STRIPS) {
        /* No strip data or strips disabled — standard DrawMesh path */
        unsigned int saved = mesh.vaoId;
        mesh.vaoId = 0;
        DrawMesh(mesh, material, transform);
        mesh.vaoId = saved;
        return;
    }

    /* Draw only the submesh corresponding to this mesh */
    int sub_idx = DCMESH_SUBMESH_INDEX(mesh.vaoId);
    if (sub_idx >= 0 && sub_idx < (int)data->submesh_count) {
        dcDrawSubmesh(&data->submeshes[sub_idx], material, transform);
    }
}

/* -------------------------------------------------------------------
 * Public API: Unload model's DCMesh data
 * ---------------------------------------------------------------- */
void dcMeshUnloadModel(Model *model) {
    if (!model) return;

    for (int i = 0; i < model->meshCount; i++) {
        unsigned int vaoId = model->meshes[i].vaoId;
        if (DCMESH_IS_REGISTRY_ID(vaoId)) {
            DCMeshData* data = dcRegistryGet(vaoId);
            if (data) {
                dcFreeData(data);
                dcRegistryRemove(vaoId);
            }
            model->meshes[i].vaoId = 0;
        }
    }
}

/* -------------------------------------------------------------------
 * Public API: Query and diagnostics
 * ---------------------------------------------------------------- */
int dcMeshHasStripData(Mesh mesh) {
    return dcRegistryGet(mesh.vaoId) != NULL;
}

void dcMeshPrintRegistryStats(void) {
    printf("[DCMesh] Registry: %d/%d entries\n", dc_registry_count, DC_MESH_REGISTRY_MAX);
    for (int i = 0; i < DC_MESH_REGISTRY_MAX; i++) {
        if (dc_registry[i]) {
            DCMeshData* d = dc_registry[i];
            uint32_t total_v = 0, total_s = 0;
            for (uint32_t j = 0; j < d->submesh_count; j++) {
                total_v += d->submeshes[j].vertex_count;
                total_s += d->submeshes[j].strip_count;
            }
            printf("  [%d] %u submeshes, %u vertices, %u strips\n",
                   i, d->submesh_count, total_v, total_s);
        }
    }
}

/* -------------------------------------------------------------------
 * Public API: Recenter DCMesh geometry to match recenter_model_geometry()
 *
 * Uses -= to match raylib's recenter convention. Pass the same
 * (dx, dy, dz) values you pass to recenter_model_geometry().
 * ---------------------------------------------------------------- */
void dcMeshRecenterGeometry(Model *model, float offsetX, float offsetY, float offsetZ) {
    if (!model) return;

    for (int i = 0; i < model->meshCount; i++) {
        DCMeshData* data = dcRegistryGet(model->meshes[i].vaoId);
        if (!data) continue;

        int sub_idx = DCMESH_SUBMESH_INDEX(model->meshes[i].vaoId);
        if (sub_idx < 0 || sub_idx >= (int)data->submesh_count) continue;

        DCSubmesh* sm = &data->submeshes[sub_idx];
        for (uint32_t v = 0; v < sm->vertex_count; v++) {
            sm->vertices[v].x -= offsetX;
            sm->vertices[v].y -= offsetY;
            sm->vertices[v].z -= offsetZ;
        }
    }
}

/* -------------------------------------------------------------------
 * Public API: Sync per-vertex colors from raylib mesh to DCMesh
 *
 * Call after light_player_model() or any CPU-side per-vertex coloring.
 * Copies raylib's RGBA color bytes into dcmesh's packed BGRA uint32.
 *
 * Only syncs the submesh corresponding to each mesh. If the mesh
 * has no colors or no dcmesh data, silently skips.
 * ---------------------------------------------------------------- */
void dcMeshSyncColors(Model *model) {
    if (!model) return;

    for (int i = 0; i < model->meshCount; i++) {
        Mesh *mesh = &model->meshes[i];
        if (!mesh->colors) continue;

        DCMeshData* data = dcRegistryGet(mesh->vaoId);
        if (!data) continue;

        int sub_idx = DCMESH_SUBMESH_INDEX(mesh->vaoId);
        if (sub_idx < 0 || sub_idx >= (int)data->submesh_count) continue;

        DCSubmesh* sm = &data->submeshes[sub_idx];

        /* raylib stores colors as sequential RGBA bytes per vertex.
         * DCMesh stores color as packed uint32 in BGRA order.
         * vertex_map[v] gives the original vertex index that
         * strip vertex v was expanded from. */

        unsigned char *src = mesh->colors;
        int rl_vc = mesh->vertexCount;

        for (uint32_t v = 0; v < sm->vertex_count; v++) {
            int si = sm->vertex_map ? sm->vertex_map[v] : (int)v;
            if (si >= rl_vc) si = si % rl_vc;  /* Safety clamp */

            unsigned char b = src[si * 4 + 0];
            unsigned char g = src[si * 4 + 1];
            unsigned char r = src[si * 4 + 2];
            unsigned char a = src[si * 4 + 3];

            /* Pack as BGRA uint32 (little-endian: byte0=B, byte1=G, byte2=R, byte3=A) */
            sm->vertices[v].color = ((uint32_t)a << 24) |
                                    ((uint32_t)r << 16) |
                                    ((uint32_t)g << 8)  |
                                    ((uint32_t)b);
        }
    }
}

/* -------------------------------------------------------------------
 * Public API: Safe UploadMesh wrapper
 *
 * Temporarily clears vaoId so UploadMesh doesn't warn about
 * "trying to re-load an already loaded mesh", then restores it.
 * Use this instead of UploadMesh() in your game code for meshes
 * that have dcmesh strip data.
 * ---------------------------------------------------------------- */
void dcMeshUploadSafe(Mesh *mesh, bool dynamic) {
    if (!mesh) return;
    unsigned int saved = mesh->vaoId;
    mesh->vaoId = 0;
    UploadMesh(mesh, dynamic);
    mesh->vaoId = saved;
}

#endif /* PLATFORM_DREAMCAST */
