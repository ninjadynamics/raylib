/*
 * rlgl_dc_batch.h - Dreamcast-only rlgl immediate-mode batcher
 *
 * Patches A + D combined: Captures immediate-mode vertex traffic from raylib
 * helpers (rlBegin/rlVertex/rlEnd) and coalesces it into large batched
 * glDrawArrays calls that hit GLdc's array fast path.
 *
 * Also implements deferred texture unbinding (Patch D) so that the common
 * rlSetTexture(id) -> draw -> rlSetTexture(0) -> rlSetTexture(id) -> draw
 * pattern does not cause redundant state churn.
 *
 * Drop this file next to rlgl.h in raylib/src/.
 * Compile with -DPLATFORM_DREAMCAST (already set by the DC build).
 *
 * All code is guarded behind PLATFORM_DREAMCAST so PC/web builds are
 * completely unaffected.
 */

#ifndef RLGL_DC_BATCH_H
#define RLGL_DC_BATCH_H

#if defined(PLATFORM_DREAMCAST)

#include <GL/gl.h>
#include <string.h>

/* -------------------------------------------------------------------
 * Configuration
 * ---------------------------------------------------------------- */

/* Maximum vertices before the batcher prefers to flush.
 * Must be divisible by 12 (LCM of 3 and 4) so a capacity flush
 * never splits an incomplete quad or triangle.
 * 4080 verts * 24 bytes = ~95 KB. */
#ifndef RLDC_BATCH_CAPACITY
#define RLDC_BATCH_CAPACITY  4080
#endif

/* Safety margin: if a single rlBegin/rlEnd block exceeds CAPACITY,
 * the per-vertex check can still flush mid-primitive. The overflow
 * margin gives headroom so the next rlDcBegin triggers a clean flush
 * instead. Actual buffer is CAPACITY + OVERFLOW. */
#define RLDC_BATCH_OVERFLOW  120

/* -------------------------------------------------------------------
 * Vertex format — matches GLdc fast-path requirements:
 *   position: 3 x GL_FLOAT
 *   texcoord: 2 x GL_FLOAT
 *   color:    GL_BGRA x GL_UNSIGNED_BYTE (BGRA byte order)
 * Total: 24 bytes per vertex, no padding needed.
 * ---------------------------------------------------------------- */
typedef struct {
    float x, y, z;            /* 12 bytes — position                 */
    float u, v;               /*  8 bytes — texcoord                 */
    unsigned char b, g, r, a; /*  4 bytes — color in BGRA byte order */
} RlDcBatchVertex;

/* -------------------------------------------------------------------
 * Batcher state
 * ---------------------------------------------------------------- */
typedef struct {
    /* Vertex buffer — includes overflow margin for mid-primitive safety */
    RlDcBatchVertex verts[RLDC_BATCH_CAPACITY + RLDC_BATCH_OVERFLOW];
    int count;                     /* vertices accumulated so far     */

    /* Current draw state for compatibility checking */
    int mode;                      /* RL_QUADS or RL_TRIANGLES        */
    unsigned int textureId;        /* currently bound texture         */
    int active;                    /* are we inside a captured begin? */

    /* Deferred unbind (Patch D) */
    int pendingUnbind;             /* rlSetTexture(0) was deferred    */

    /* Current vertex attributes (set by rlTexCoord/rlColor/rlNormal) */
    float curU, curV;
    unsigned char curR, curG, curB, curA;

    /* Last texture actually submitted to GL — avoids redundant binds */
    unsigned int lastFlushedTexId;
    int lastFlushedTexValid;        /* false until first flush */

    /* Stats (optional, compile with -DRLDC_ENABLE_STATS) */
#ifdef RLDC_ENABLE_STATS
    unsigned int statFlushes;
    unsigned int statFlushTexChange;
    unsigned int statFlushModeChange;
    unsigned int statFlushMatrixChange;
    unsigned int statFlushCapacity;
    unsigned int statFlushExplicit;
    unsigned int statFlushStateChange;
    unsigned int statTotalVertices;
    unsigned int statCancelledUnbinds;
#endif
} RlDcBatch;

/* Single global batcher instance */
static RlDcBatch rlDcBatch = {
    .count = 0,
    .mode = -1,
    .textureId = 0,
    .active = 0,
    .pendingUnbind = 0,
    .curU = 0.0f, .curV = 0.0f,
    .curR = 255, .curG = 255, .curB = 255, .curA = 255,
    .lastFlushedTexId = 0,
    .lastFlushedTexValid = 0,
};

/* -------------------------------------------------------------------
 * Stats helpers
 * ---------------------------------------------------------------- */
#ifdef RLDC_ENABLE_STATS
#define RLDC_STAT_INC(f)  (rlDcBatch.f++)
static void rlDcPrintStats(void) {
    printf("[rlDC] flushes=%u tex=%u mode=%u mtx=%u state=%u cap=%u expl=%u verts=%u "
           "unbinds_cancelled=%u\n",
           rlDcBatch.statFlushes,
           rlDcBatch.statFlushTexChange,
           rlDcBatch.statFlushModeChange,
           rlDcBatch.statFlushMatrixChange,
           rlDcBatch.statFlushStateChange,
           rlDcBatch.statFlushCapacity,
           rlDcBatch.statFlushExplicit,
           rlDcBatch.statTotalVertices,
           rlDcBatch.statCancelledUnbinds);
}
static void rlDcResetStats(void) {
    rlDcBatch.statFlushes = 0;
    rlDcBatch.statFlushTexChange = 0;
    rlDcBatch.statFlushModeChange = 0;
    rlDcBatch.statFlushMatrixChange = 0;
    rlDcBatch.statFlushStateChange = 0;
    rlDcBatch.statFlushCapacity = 0;
    rlDcBatch.statFlushExplicit = 0;
    rlDcBatch.statTotalVertices = 0;
    rlDcBatch.statCancelledUnbinds = 0;
}
#else
#define RLDC_STAT_INC(f)  ((void)0)
__attribute__((unused)) static void rlDcPrintStats(void) {}
__attribute__((unused)) static void rlDcResetStats(void) {}
#endif

/* -------------------------------------------------------------------
 * Core: Flush the batcher
 *
 * Submits all accumulated vertices to GLdc as a single glDrawArrays
 * call using client arrays. This feeds directly into GLdc's
 * generateArraysFastPath_QUADS / _TRIS, which is the best-case
 * submission path.
 * ---------------------------------------------------------------- */
static void rlDcFlushBatch(void)
{
    if (rlDcBatch.count == 0) return;

    RLDC_STAT_INC(statFlushes);
#ifdef RLDC_ENABLE_STATS
    rlDcBatch.statTotalVertices += rlDcBatch.count;
#endif

    GLenum glMode;
    switch (rlDcBatch.mode) {
        case RL_QUADS:     glMode = GL_QUADS;     break;
        case RL_TRIANGLES: glMode = GL_TRIANGLES; break;
        default:           glMode = GL_TRIANGLES; break;
    }

    /* Bind the batch texture — skip if unchanged since last flush */
    if (!rlDcBatch.lastFlushedTexValid || rlDcBatch.textureId != rlDcBatch.lastFlushedTexId) {
        if (rlDcBatch.textureId > 0) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, rlDcBatch.textureId);
        } else {
            glDisable(GL_TEXTURE_2D);
        }
        rlDcBatch.lastFlushedTexId = rlDcBatch.textureId;
        rlDcBatch.lastFlushedTexValid = 1;
    }

    /* Set up client arrays pointing into our interleaved buffer.
     * Stride = sizeof(RlDcBatchVertex) = 24 bytes.
     * This layout satisfies GLdc's fast-path requirements:
     *   vertex:  3 x GL_FLOAT
     *   uv:     2 x GL_FLOAT
     *   colour: GL_BGRA x GL_UNSIGNED_BYTE
     */
    const GLsizei stride = sizeof(RlDcBatchVertex);
    const RlDcBatchVertex* buf = rlDcBatch.verts;

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);  /* Skip normals for speed */

    glVertexPointer(3, GL_FLOAT, stride, &buf[0].x);
    glTexCoordPointer(2, GL_FLOAT, stride, &buf[0].u);
    glColorPointer(GL_BGRA, GL_UNSIGNED_BYTE, stride, &buf[0].b);

    /* The big payoff: one draw call for potentially hundreds of
     * raylib helper calls that would otherwise be individual
     * glBegin/glEnd pairs. */
    glDrawArrays(glMode, 0, rlDcBatch.count);

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    rlDcBatch.count = 0;
}

/* -------------------------------------------------------------------
 * Append a single vertex to the batch buffer.
 * If the buffer is full, flush first.
 * ---------------------------------------------------------------- */
static inline void rlDcAppendVertex(float x, float y, float z)
{
    /* Hard safety limit — should rarely trigger if rlDcBegin flushes
     * at the clean capacity boundary. This catches the edge case of
     * a single rlBegin/rlEnd block exceeding RLDC_BATCH_CAPACITY. */
    if (rlDcBatch.count >= (RLDC_BATCH_CAPACITY + RLDC_BATCH_OVERFLOW)) {
        RLDC_STAT_INC(statFlushCapacity);
        rlDcFlushBatch();
    }

    RlDcBatchVertex* v = &rlDcBatch.verts[rlDcBatch.count++];
    v->x = x;
    v->y = y;
    v->z = z;
    v->u = rlDcBatch.curU;
    v->v = rlDcBatch.curV;
    v->r = rlDcBatch.curR;
    v->g = rlDcBatch.curG;
    v->b = rlDcBatch.curB;
    v->a = rlDcBatch.curA;
}

/* -------------------------------------------------------------------
 * Texture state management with deferred unbinding (Patch D)
 *
 * When rlSetTexture(0) is called, we don't immediately unbind.
 * If the next rlSetTexture(id) matches the current batch texture,
 * we cancel the unbind entirely — saving a state dirty + header
 * rebuild in GLdc.
 * ---------------------------------------------------------------- */
static void rlDcSetTexture(unsigned int id)
{
    if (id == 0) {
        /* Defer the unbind — don't flush or dirty state yet */
        rlDcBatch.pendingUnbind = 1;
        return;
    }

    /* Non-zero texture */
    if (rlDcBatch.pendingUnbind) {
        if (id == rlDcBatch.textureId) {
            /* Same texture as current batch — cancel the unbind */
            rlDcBatch.pendingUnbind = 0;
            RLDC_STAT_INC(statCancelledUnbinds);
            return;
        }
        /* Different texture — need to flush with old texture first */
        rlDcBatch.pendingUnbind = 0;
    }

    if (rlDcBatch.count > 0 && id != rlDcBatch.textureId) {
        RLDC_STAT_INC(statFlushTexChange);
        rlDcFlushBatch();
    }

    rlDcBatch.textureId = id;
}

/* -------------------------------------------------------------------
 * Begin/End capture
 *
 * rlDcBegin: If the mode is RL_QUADS or RL_TRIANGLES, capture it.
 *            Flush if the mode changed from the current batch.
 *
 * rlDcEnd:   Do nothing — keep the batch open for the next draw.
 *            The batch only flushes on correctness boundaries.
 * ---------------------------------------------------------------- */
static int rlDcBegin(int mode)
{
    /* Only capture quads and triangles — let lines/points fall through
     * to the original GL path */
    if (mode != RL_QUADS && mode != RL_TRIANGLES) {
        /* Flush anything pending before non-captured mode */
        rlDcFlushBatch();
        /* Resolve any pending unbind before falling through */
        if (rlDcBatch.pendingUnbind) {
            rlDcBatch.pendingUnbind = 0;
            glDisable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, 0);
            rlDcBatch.textureId = 0;
            rlDcBatch.lastFlushedTexValid = 0;  /* GL state changed outside batcher */
        }
        return 0; /* Not captured — caller should do the real glBegin */
    }

    /* Flush if mode changed */
    if (rlDcBatch.count > 0 && rlDcBatch.mode != mode) {
        RLDC_STAT_INC(statFlushModeChange);
        rlDcFlushBatch();
    }

    /* Clean-boundary capacity check: flush at CAPACITY (multiple of 12)
     * so we never split a primitive. This fires between rlBegin calls,
     * guaranteeing the flush point is a primitive boundary. */
    if (rlDcBatch.count >= RLDC_BATCH_CAPACITY) {
        RLDC_STAT_INC(statFlushCapacity);
        rlDcFlushBatch();
    }

    /* Resolve pending unbind if texture context genuinely needs 0 */
    if (rlDcBatch.pendingUnbind && rlDcBatch.count == 0) {
        /* If batch is empty and we have a pending unbind, the next
         * draw might set a new texture anyway, so just clear the
         * flag and let rlDcSetTexture handle it */
        rlDcBatch.pendingUnbind = 0;
        rlDcBatch.textureId = 0;
    }

    rlDcBatch.mode = mode;
    rlDcBatch.active = 1;
    return 1; /* Captured */
}

static void rlDcEnd(void)
{
    rlDcBatch.active = 0;
    /* Do NOT flush here — keep the batch open */
}

/* -------------------------------------------------------------------
 * Matrix change flush trigger
 *
 * Any matrix operation (push, pop, translate, rotate, scale, load,
 * mult) must flush the batch because GLdc loads the transform state
 * once per submitVertices() call.
 * ---------------------------------------------------------------- */
static inline void rlDcFlushOnMatrixChange(void)
{
    if (rlDcBatch.count > 0) {
        RLDC_STAT_INC(statFlushMatrixChange);
        rlDcFlushBatch();
    }
}

/* -------------------------------------------------------------------
 * GL state change flush trigger
 *
 * Any GL state operation (blend, depth, cull, scissor, etc.) must
 * flush the batch because GLdc builds poly-headers from current state.
 * Also invalidates lastFlushedTex since GL state is now different.
 * ---------------------------------------------------------------- */
static inline void rlDcFlushOnStateChange(void)
{
    if (rlDcBatch.count > 0) {
        RLDC_STAT_INC(statFlushStateChange);
        rlDcFlushBatch();
    }
    rlDcBatch.lastFlushedTexValid = 0;
}

/* -------------------------------------------------------------------
 * Explicit flush — call at frame boundaries, before SwapBuffers,
 * or before any operation that must see all submitted geometry.
 * ---------------------------------------------------------------- */
static void rlDcFlushAll(void)
{
    if (rlDcBatch.count > 0) {
        RLDC_STAT_INC(statFlushExplicit);
        rlDcFlushBatch();
    }

    /* Resolve any pending unbind */
    if (rlDcBatch.pendingUnbind) {
        rlDcBatch.pendingUnbind = 0;
        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
        rlDcBatch.textureId = 0;
        rlDcBatch.lastFlushedTexValid = 0;  /* GL state changed outside batcher */
    }
}

#endif /* PLATFORM_DREAMCAST */
#endif /* RLGL_DC_BATCH_H */
