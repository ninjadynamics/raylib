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
#include <GL/glkos.h>
#include <stddef.h>
#include <string.h>

/* Parent builds can turn off only the new handoff while retaining the same
 * GLdc/raylib sources, which gives hardware A/B runs an attribution-safe
 * fallback binary. */
#ifndef RLDC_USE_INTERLEAVED_FAST_PATH
#define RLDC_USE_INTERLEAVED_FAST_PATH 1
#endif

/* N3 is independently switchable so a production-game hardware A/B can keep
 * the permanent N2 consumers and synchronous F1 fallback identical on both
 * sides. Disabling the older interleaved switch still disables both lanes. */
#ifndef RLDC_USE_FINAL_PACKET_FAST_PATH
#define RLDC_USE_FINAL_PACKET_FAST_PATH 0
#endif
#if RLDC_USE_FINAL_PACKET_FAST_PATH != 0 && RLDC_USE_FINAL_PACKET_FAST_PATH != 1
#error "RLDC_USE_FINAL_PACKET_FAST_PATH must be 0 or 1"
#endif

/* Final-packet construction has a fixed transactional cost which only pays
 * back once a batch is large enough. The generic fork leaves the threshold at
 * zero; HyperSolar's parent build selects the hardware-informed cutoff. */
#ifndef RLDC_FINAL_PACKET_MIN_VERTICES
#define RLDC_FINAL_PACKET_MIN_VERTICES 0
#endif
#if RLDC_FINAL_PACKET_MIN_VERTICES < 0
#error "RLDC_FINAL_PACKET_MIN_VERTICES must not be negative"
#endif

/* The trusted N3 writer is not a general rlgl property: public rlVertex,
 * rlTexCoord and matrix calls accept arbitrary floats. HyperSolar's parent
 * build may assert its game-owned finite source/matrix/depth contract; every
 * other consumer defaults to checked N3. This is a contract selector, not a
 * hardware A/B switch. */
#ifndef RLDC_HYPERSOLAR_TRUSTED_N3
#define RLDC_HYPERSOLAR_TRUSTED_N3 0
#endif
#if RLDC_HYPERSOLAR_TRUSTED_N3 != 0 && RLDC_HYPERSOLAR_TRUSTED_N3 != 1
#error "RLDC_HYPERSOLAR_TRUSTED_N3 must be 0 or 1"
#endif

#if RLDC_USE_INTERLEAVED_FAST_PATH && \
    defined(GL_KOS_HAS_INTERLEAVED_P3T2BGRA) && \
    defined(GL_KOS_FAST_PATH_ABI_VERSION) && \
    (GL_KOS_HAS_INTERLEAVED_P3T2BGRA != 0) && \
    (GL_KOS_FAST_PATH_ABI_VERSION == 3u)
#define RLDC_HAS_INTERLEAVED_P3T2BGRA 1
#else
#define RLDC_HAS_INTERLEAVED_P3T2BGRA 0
#endif

#if RLDC_USE_INTERLEAVED_FAST_PATH && \
    RLDC_USE_FINAL_PACKET_FAST_PATH && \
    defined(GL_KOS_HAS_FINAL_INTERLEAVED_P3T2BGRA) && \
    defined(GL_KOS_HAS_TRUSTED_FINAL_INTERLEAVED_P3T2BGRA) && \
    defined(GL_KOS_FAST_PATH_ABI_VERSION) && \
    (GL_KOS_HAS_FINAL_INTERLEAVED_P3T2BGRA != 0) && \
    (GL_KOS_HAS_TRUSTED_FINAL_INTERLEAVED_P3T2BGRA != 0) && \
    (GL_KOS_FAST_PATH_ABI_VERSION == 3u)
#define RLDC_HAS_FINAL_INTERLEAVED_P3T2BGRA 1
#else
#define RLDC_HAS_FINAL_INTERLEAVED_P3T2BGRA 0
#endif

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
#if RLDC_HAS_INTERLEAVED_P3T2BGRA || RLDC_HAS_FINAL_INTERLEAVED_P3T2BGRA
typedef GLKosVertexP3T2BGRA RlDcBatchVertex;
#else
typedef struct {
    float x, y, z;            /* 12 bytes — position                 */
    float u, v;               /*  8 bytes — texcoord                 */
    unsigned int bgra;        /*  4 bytes — color word, BGRA byte order
                                 (little-endian: b|g<<8|r<<16|a<<24) */
} RlDcBatchVertex;
#endif

typedef char RlDcBatchVertexSizeMustBe24[
    sizeof(RlDcBatchVertex) == 24 ? 1 : -1];
typedef char RlDcBatchVertexPositionMustStartAt0[
    offsetof(RlDcBatchVertex, x) == 0 ? 1 : -1];
typedef char RlDcBatchVertexUvMustStartAt12[
    offsetof(RlDcBatchVertex, u) == 12 ? 1 : -1];
typedef char RlDcBatchVertexColorMustStartAt20[
    offsetof(RlDcBatchVertex, bgra) == 20 ? 1 : -1];

/* Pack semantic RGBA into the physical BGRA color word once, at rlColor time,
 * so the per-vertex append is a single word load + store instead of four byte
 * loads re-swizzled per vertex (~25% of the append's instruction count). */
static inline unsigned int rlDcPackBGRA(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    return (unsigned int)b | ((unsigned int)g << 8) | ((unsigned int)r << 16) | ((unsigned int)a << 24);
}

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
    int active;                    /* 1 = captured area, -1 = discard */

    /* Deferred unbind (Patch D) */
    int pendingUnbind;             /* rlSetTexture(0) was deferred    */

    /* Current vertex attributes (set by rlTexCoord/rlColor/rlNormal) */
    float curU, curV;
    unsigned int curBGRA;          /* pre-packed color word (rlDcPackBGRA) */

    /* Last texture actually submitted to GL — avoids redundant binds */
    unsigned int lastFlushedTexId;
    int lastFlushedTexValid;        /* false until first flush */

    /* rlgl framebuffer bookkeeping remains meaningful even though GLdc has
     * no FBO objects: it describes the current viewport-sized draw target. */
    int framebufferWidth, framebufferHeight;
    float lineWidth;

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
    unsigned int statInterleavedHits;
    unsigned int statInterleavedFallbacks;
    unsigned int statFinalPacketHits;
    unsigned int statFinalPacketFallbacks;
#endif
} RlDcBatch;

/* Single global batcher instance. Keep the 100 KB vertex arena in .bss;
 * rlDcResetState() is the authoritative initializer for non-zero defaults. */
static RlDcBatch rlDcBatch;

#if defined(GLDC_NATIVE_BENCH) && GLDC_NATIVE_BENCH
/* One benchmark-only route word preserves the checked/F1/trusted selector
 * without letting extra control globals perturb the adjacent hot arena. */
static int rlDcNativeBenchN3Route;
#endif

static inline int rlDcFinalPacketEnabled(void)
{
#if defined(GLDC_NATIVE_BENCH) && GLDC_NATIVE_BENCH
    return rlDcNativeBenchN3Route != 1;
#else
    return 1;
#endif
}

#if RLDC_HAS_FINAL_INTERLEAVED_P3T2BGRA
static inline GLboolean rlDcTryFinalPacket(
    GLenum mode, const RlDcBatchVertex *vertices, GLsizei count)
{
#if defined(GLDC_NATIVE_BENCH) && GLDC_NATIVE_BENCH
    /* Route 1 is filtered by rlDcFinalPacketEnabled(). Keep the checked and
     * trusted hardware controls independent of the parent game contract. */
    if (rlDcNativeBenchN3Route == 2)
        return glKosTryQueueTrustedFinalInterleavedP3T2BGRA(
            mode, vertices, count);
    return glKosTryQueueFinalInterleavedP3T2BGRA(
        mode, vertices, count);
#elif RLDC_HYPERSOLAR_TRUSTED_N3
    /* This seam is private to HyperSolar's captured P3/T2/BGRA batch. Its source
     * positions/UVs and the current matrices must be finite; every accepted
     * transformed record must have finite X/Y/UV and positive finite inverse
     * depth. GLdc still classifies every transformed vertex and commits only
     * an all-visible batch. Near/ambiguous input returns false and therefore
     * takes the synchronous F1/exact-client-array fallback below. */
    return glKosTryQueueTrustedFinalInterleavedP3T2BGRA(
        mode, vertices, count);
#else
    return glKosTryQueueFinalInterleavedP3T2BGRA(
        mode, vertices, count);
#endif
}
#endif

static inline void rlDcResetState(void)
{
    rlDcBatch.count = 0;
    rlDcBatch.mode = -1;
    rlDcBatch.textureId = 0;
    rlDcBatch.active = 0;
    rlDcBatch.pendingUnbind = 0;
    rlDcBatch.curU = 0.0f;
    rlDcBatch.curV = 0.0f;
    rlDcBatch.curBGRA = 0xFFFFFFFFu;
    rlDcBatch.lastFlushedTexId = 0;
    rlDcBatch.lastFlushedTexValid = 0;
    rlDcBatch.framebufferWidth = 0;
    rlDcBatch.framebufferHeight = 0;
    rlDcBatch.lineWidth = 1.0f;
#if defined(GLDC_NATIVE_BENCH) && GLDC_NATIVE_BENCH
    rlDcNativeBenchN3Route = 0;
#endif
#ifdef RLDC_ENABLE_STATS
    rlDcBatch.statFlushes = 0;
    rlDcBatch.statFlushTexChange = 0;
    rlDcBatch.statFlushModeChange = 0;
    rlDcBatch.statFlushMatrixChange = 0;
    rlDcBatch.statFlushCapacity = 0;
    rlDcBatch.statFlushExplicit = 0;
    rlDcBatch.statFlushStateChange = 0;
    rlDcBatch.statTotalVertices = 0;
    rlDcBatch.statCancelledUnbinds = 0;
    rlDcBatch.statInterleavedHits = 0;
    rlDcBatch.statInterleavedFallbacks = 0;
    rlDcBatch.statFinalPacketHits = 0;
    rlDcBatch.statFinalPacketFallbacks = 0;
#endif
}

/* -------------------------------------------------------------------
 * Stats helpers
 * ---------------------------------------------------------------- */
#ifdef RLDC_ENABLE_STATS
#define RLDC_STAT_INC(f)  (rlDcBatch.f++)
__attribute__((unused)) static void rlDcResetStats(void) {
    rlDcBatch.statFlushes = 0;
    rlDcBatch.statFlushTexChange = 0;
    rlDcBatch.statFlushModeChange = 0;
    rlDcBatch.statFlushMatrixChange = 0;
    rlDcBatch.statFlushStateChange = 0;
    rlDcBatch.statFlushCapacity = 0;
    rlDcBatch.statFlushExplicit = 0;
    rlDcBatch.statTotalVertices = 0;
    rlDcBatch.statCancelledUnbinds = 0;
    rlDcBatch.statInterleavedHits = 0;
    rlDcBatch.statInterleavedFallbacks = 0;
    rlDcBatch.statFinalPacketHits = 0;
    rlDcBatch.statFinalPacketFallbacks = 0;
}
#else
#define RLDC_STAT_INC(f)  ((void)0)
__attribute__((unused)) static void rlDcResetStats(void) {}
#endif

/* -------------------------------------------------------------------
 * Core: Flush the batcher
 *
 * Submits all accumulated vertices to GLdc in one call. A paired GLdc first
 * snapshots a final N3 packet, then tries synchronous F1; an ineligible case
 * takes the exact client-array fallback below.
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

#if RLDC_HAS_FINAL_INTERLEAVED_P3T2BGRA
    /* Trusted N3 copies final TA records into GLdc-owned RAM before returning.
     * GLdc accepts only a proven all-visible batch. That makes this transient
     * arena safe to reuse while preserving active-list chronology at the later
     * GLdc drain; any decline retains the exact synchronous fallbacks. */
    if (rlDcBatch.count >= RLDC_FINAL_PACKET_MIN_VERTICES &&
        rlDcFinalPacketEnabled()) {
        if (rlDcTryFinalPacket(glMode, rlDcBatch.verts, rlDcBatch.count)) {
            RLDC_STAT_INC(statFinalPacketHits);
            rlDcBatch.count = 0;
            return;
        }
        RLDC_STAT_INC(statFinalPacketFallbacks);
    }
#endif

#if RLDC_HAS_INTERLEAVED_P3T2BGRA
    /* The paired GLdc lane consumes this borrowed stream before returning and
     * does not disturb client-array state. GLdc alone decides eligibility; a
     * false return leaves render/list/header/capture state untouched. */
    if (glKosTryDrawInterleavedP3T2BGRA(glMode, rlDcBatch.verts,
                                        rlDcBatch.count)) {
        RLDC_STAT_INC(statInterleavedHits);
        rlDcBatch.count = 0;
        return;
    }
    RLDC_STAT_INC(statInterleavedFallbacks);
#endif

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
    glDisableClientState(GL_NORMAL_ARRAY);  /* Hot P+UV+BGRA GLdc lane */

    glVertexPointer(3, GL_FLOAT, stride, &buf[0].x);
    glTexCoordPointer(2, GL_FLOAT, stride, &buf[0].u);
    glColorPointer(GL_BGRA, GL_UNSIGNED_BYTE, stride, &buf[0].bgra);

    /* The big payoff: one draw call for potentially hundreds of
     * raylib helper calls that would otherwise be individual
     * glBegin/glEnd pairs. */
#if RLDC_HAS_INTERLEAVED_P3T2BGRA
    /* A declined try may mean computed radial fog or a general TnL effect.
     * Plain glDrawArrays is the exact fallback for both primitive types; the
     * older triangle fused lane deliberately omits those computed effects. */
    glDrawArrays(glMode, 0, rlDcBatch.count);
#else
    if (glMode == GL_TRIANGLES) glKosDrawTrianglesArrays(0, rlDcBatch.count);
    else glDrawArrays(glMode, 0, rlDcBatch.count);
#endif

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
    v->bgra = rlDcBatch.curBGRA;
}

/* The common font quad is four adjacent appends with one color and no
 * intervening state change. Check room once; an unsupported mode or capacity
 * edge returns untouched so the caller retains the exact per-vertex fallback
 * and its original flush boundary. This changes no GLdc submission contract. */
bool rlDcTryTexturedQuad2D(float x0, float y0, float x1, float y1,
                          float u0, float v0, float u1, float v1)
{
    if (rlDcBatch.active <= 0 || rlDcBatch.mode != RL_QUADS ||
        (unsigned)rlDcBatch.count >
            (RLDC_BATCH_CAPACITY + RLDC_BATCH_OVERFLOW - 4u)) return false;

    RlDcBatchVertex *v = &rlDcBatch.verts[rlDcBatch.count];
    const unsigned int color = rlDcBatch.curBGRA;
    v[0].x = x0; v[0].y = y0; v[0].z = 0.0f;
    v[0].u = u0; v[0].v = v0; v[0].bgra = color;
    v[1].x = x0; v[1].y = y1; v[1].z = 0.0f;
    v[1].u = u0; v[1].v = v1; v[1].bgra = color;
    v[2].x = x1; v[2].y = y1; v[2].z = 0.0f;
    v[2].u = u1; v[2].v = v1; v[2].bgra = color;
    v[3].x = x1; v[3].y = y0; v[3].z = 0.0f;
    v[3].u = u1; v[3].v = v0; v[3].bgra = color;
    rlDcBatch.count += 4;
    /* rlTexCoord2f is persistent even across rlEnd: retain the last corner. */
    rlDcBatch.curU = u1;
    rlDcBatch.curV = v0;
    return true;
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
/* Keep unsupported primitive handling out of the area-geometry hot path.
 * RL_LINES records no vertices, so arbitrarily large wire helpers stay safe. */
#if defined(__GNUC__) || defined(__clang__)
__attribute__((cold, noinline))
#endif
static int rlDcBeginUnsupported(int mode)
{
    if (mode == RL_LINES) {
        rlDcBatch.active = -1;
        return 1;
    }

    if (rlDcBatch.count > 0)
        RLDC_STAT_INC(statFlushModeChange);
    rlDcFlushBatch();
    if (rlDcBatch.pendingUnbind) {
        rlDcBatch.pendingUnbind = 0;
        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
        rlDcBatch.textureId = 0;
        rlDcBatch.lastFlushedTexValid = 0;
    }
    return 0;
}

#if defined(__GNUC__) || defined(__clang__)
__attribute__((cold, noinline))
#endif
static void rlDcCommitPendingUnbind(void)
{
    if (rlDcBatch.count > 0) {
        RLDC_STAT_INC(statFlushTexChange);
        rlDcFlushBatch();
    }
    rlDcBatch.pendingUnbind = 0;
    rlDcBatch.textureId = 0;
}

static int rlDcBegin(int mode)
{
    if (__builtin_expect((mode != RL_QUADS) && (mode != RL_TRIANGLES), 0))
        return rlDcBeginUnsupported(mode);

    /* A still-pending zero texture means this is genuinely untextured; the
     * common same-texture case cancels it in rlDcSetTexture() before Begin. */
    if (__builtin_expect(rlDcBatch.pendingUnbind != 0, 0))
        rlDcCommitPendingUnbind();

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
 * Any non-texture GL state operation (blend, depth, cull, scissor, etc.)
 * must flush because GLdc builds poly-headers from current state.  Texture
 * binding is unchanged, so keeping that cache valid avoids a redundant bind.
 * ---------------------------------------------------------------- */
static inline void rlDcFlushOnStateChange(void)
{
    if (rlDcBatch.count > 0) {
        RLDC_STAT_INC(statFlushStateChange);
        rlDcFlushBatch();
    }
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

#if defined(GLDC_NATIVE_BENCH) && GLDC_NATIVE_BENCH
/* Flush under the old route before changing the benchmark seam. Invalid
 * values deliberately normalize to the production checked-N3 route. */
static inline void rlDcNativeBenchSelectN3RouteInternal(int route)
{
    rlDcFlushAll();
    rlDcNativeBenchN3Route = (route == 1 || route == 2) ? route : 0;
}
#endif

#if defined(__GNUC__) || defined(__clang__)
__attribute__((cold, noinline, unused))
#endif
static void rlDcResolvePendingUnbind(void)
{
    rlDcFlushAll();
}

/* A direct texture bind supersedes a deferred unbind. Flush queued geometry,
 * then let the caller perform exactly one final bind; never emit the losing
 * disable+bind(0) state immediately before the winning texture state. */
static inline void rlDcExternalTextureBarrier(void)
{
    if (rlDcBatch.count > 0) {
        RLDC_STAT_INC(statFlushExplicit);
        rlDcFlushBatch();
    }
    rlDcBatch.pendingUnbind = 0;
}

/* A queued immediate batch installs its own client arrays, so submit it before
 * accepting caller pointers. Deferred texture state is independent and stays
 * coalescible until the direct draw or texture call. */
static inline void rlDcClientArrayBarrier(void)
{
    if (rlDcBatch.count > 0) {
        RLDC_STAT_INC(statFlushExplicit);
        rlDcFlushBatch();
    }
}

/* Cold resource/state APIs may mutate texture state without a replacement
 * bind. Resolve everything and invalidate the cache conservatively. */
static inline void rlDcExternalStateBarrierInternal(void)
{
    rlDcFlushAll();
    rlDcBatch.lastFlushedTexValid = 0;
}


#endif /* PLATFORM_DREAMCAST */
#endif /* RLGL_DC_BATCH_H */
