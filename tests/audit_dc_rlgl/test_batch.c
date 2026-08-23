#include <assert.h>
#include <stddef.h>

#define PLATFORM_DREAMCAST
#define RL_LINES 1
#define RL_TRIANGLES 4
#define RL_QUADS 7
#define RL_BLEND_ALPHA 0
#define RL_SRC_ALPHA 0x0302
#define RL_ONE_MINUS_SRC_ALPHA 0x0303
#define RL_FUNC_ADD 0x8006

#include "../../src/rlgl_dc_batch.h"

#ifndef AUDIT_EXPECT_F1
#error "Each audit target must state whether the paired F1 lane is expected"
#endif
#ifndef AUDIT_EXPECT_N3
#error "Each audit target must state whether the paired N3 lane is expected"
#endif
#if RLDC_HAS_INTERLEAVED_P3T2BGRA != AUDIT_EXPECT_F1
#error "RLDC F1 feature/ABI gate disagrees with this test configuration"
#endif
#if RLDC_HAS_FINAL_INTERLEAVED_P3T2BGRA != AUDIT_EXPECT_N3
#error "RLDC N3 feature/ABI gate disagrees with this test configuration"
#endif

#if (defined(GLDC_NATIVE_BENCH) && GLDC_NATIVE_BENCH) || \
    !RLDC_HYPERSOLAR_TRUSTED_N3
#define AUDIT_DEFAULT_N3_CHECKED 1
#else
#define AUDIT_DEFAULT_N3_CHECKED 0
#endif

enum {
    AUDIT_ROUTE_N3 = 1,
    AUDIT_ROUTE_F1,
    AUDIT_ROUTE_CLIENT,
    AUDIT_ROUTE_N3_TRUSTED
};

enum {
    AUDIT_SELECTOR_N3_CHECKED = 0,
    AUDIT_SELECTOR_F1_ONLY = 1,
    AUDIT_SELECTOR_N3_TRUSTED = 2
};

static int drawCount;
static int drawSizes[8];
static int arrayDrawCount;
static int triangleLaneCount;
static int textureEnabled;
static unsigned int boundTexture;
static int textureEnableCount;
static int textureDisableCount;
static int zeroBindCount;
static int normalArrayEnabled;
static int normalPointerCount;
static int clientStateCalls;
static int pointerCalls;
static int routeCount;
static int routes[8];
static int f1TryCount;
static int f1Accept;
static GLenum f1Mode;
static int n3TryCount;
static int n3Accept;
static GLenum n3Mode;
static int trustedN3TryCount;
static int trustedN3Accept;
static GLenum trustedN3Mode;

static void recordRoute(int route)
{
    assert(routeCount < (int)(sizeof(routes)/sizeof(routes[0])));
    routes[routeCount++] = route;
}

#if !defined(AUDIT_GLKOS_LEGACY)
unsigned int glKosGetFastPathCapabilities(void)
{
    return GL_KOS_FAST_PATH_CAPABILITIES;
}

GLboolean glKosTryDrawInterleavedP3T2BGRA(
    GLenum mode, const GLKosVertexP3T2BGRA *vertices, GLsizei count)
{
    (void)vertices;
    recordRoute(AUDIT_ROUTE_F1);
    f1TryCount++;
    f1Mode = mode;
    if (!f1Accept) return 0;
    drawSizes[drawCount++] = count;
    return 1;
}

GLboolean glKosTryQueueFinalInterleavedP3T2BGRA(
    GLenum mode, const GLKosVertexP3T2BGRA *vertices, GLsizei count)
{
    (void)vertices;
    recordRoute(AUDIT_ROUTE_N3);
    n3TryCount++;
    n3Mode = mode;
    if (!n3Accept) return 0;
    drawSizes[drawCount++] = count;
    return 1;
}

GLboolean glKosTryQueueTrustedFinalInterleavedP3T2BGRA(
    GLenum mode, const GLKosVertexP3T2BGRA *vertices, GLsizei count)
{
    (void)vertices;
    recordRoute(AUDIT_ROUTE_N3_TRUSTED);
    trustedN3TryCount++;
    trustedN3Mode = mode;
    if (!trustedN3Accept) return 0;
    drawSizes[drawCount++] = count;
    return 1;
}
#endif

void glKosDrawTrianglesArrays(int first, GLsizei count)
{
    (void)first;
    recordRoute(AUDIT_ROUTE_CLIENT);
    triangleLaneCount++;
    drawSizes[drawCount++] = count;
}

void glEnable(GLenum cap)
{
    if (cap == GL_TEXTURE_2D) {
        textureEnabled = 1;
        textureEnableCount++;
    }
}

void glDisable(GLenum cap)
{
    if (cap == GL_TEXTURE_2D) {
        textureEnabled = 0;
        textureDisableCount++;
    }
}

void glBindTexture(GLenum target, unsigned int texture)
{
    (void)target;
    boundTexture = texture;
    if (texture == 0) zeroBindCount++;
}

void glEnableClientState(GLenum cap)
{
    clientStateCalls++;
    if (cap == GL_NORMAL_ARRAY) normalArrayEnabled = 1;
}

void glDisableClientState(GLenum cap)
{
    clientStateCalls++;
    if (cap == GL_NORMAL_ARRAY) normalArrayEnabled = 0;
}

void glVertexPointer(int size, GLenum type, GLsizei stride, const void *pointer)
{ (void)size; (void)type; (void)stride; (void)pointer; pointerCalls++; }

void glTexCoordPointer(int size, GLenum type, GLsizei stride, const void *pointer)
{ (void)size; (void)type; (void)stride; (void)pointer; pointerCalls++; }

void glColorPointer(int size, GLenum type, GLsizei stride, const void *pointer)
{ (void)size; (void)type; (void)stride; (void)pointer; pointerCalls++; }

void glNormalPointer(GLenum type, GLsizei stride, const void *pointer)
{
    (void)type;
    (void)stride;
    (void)pointer;
    normalPointerCount++;
}

void glDrawArrays(GLenum mode, int first, GLsizei count)
{
    (void)mode;
    (void)first;
    recordRoute(AUDIT_ROUTE_CLIENT);
    arrayDrawCount++;
    drawSizes[drawCount++] = count;
}

static void resetHarness(void)
{
    rlDcResetState();
    drawCount = 0;
    arrayDrawCount = 0;
    triangleLaneCount = 0;
    textureEnabled = 0;
    boundTexture = 0;
    textureEnableCount = 0;
    textureDisableCount = 0;
    zeroBindCount = 0;
    normalArrayEnabled = 0;
    normalPointerCount = 0;
    clientStateCalls = 0;
    pointerCalls = 0;
    routeCount = 0;
    f1TryCount = 0;
    f1Accept = 1;
    f1Mode = 0;
    n3TryCount = 0;
    n3Accept = 1;
    n3Mode = 0;
    trustedN3TryCount = 0;
    trustedN3Accept = 1;
    trustedN3Mode = 0;
}

static void assertFlushAccounting(void)
{
#ifdef RLDC_ENABLE_STATS
    const unsigned int causes =
        rlDcBatch.statFlushTexChange + rlDcBatch.statFlushModeChange +
        rlDcBatch.statFlushMatrixChange + rlDcBatch.statFlushCapacity +
        rlDcBatch.statFlushExplicit + rlDcBatch.statFlushStateChange;
    assert(rlDcBatch.statFlushes == causes);
#endif
}

static void assertRouteStats(
    unsigned int n3Hits, unsigned int n3Fallbacks,
    unsigned int f1Hits, unsigned int f1Fallbacks)
{
#ifdef RLDC_ENABLE_STATS
    assert(rlDcBatch.statFinalPacketHits == n3Hits);
    assert(rlDcBatch.statFinalPacketFallbacks == n3Fallbacks);
    assert(rlDcBatch.statInterleavedHits == f1Hits);
    assert(rlDcBatch.statInterleavedFallbacks == f1Fallbacks);
#else
    (void)n3Hits;
    (void)n3Fallbacks;
    (void)f1Hits;
    (void)f1Fallbacks;
#endif
}

static void triangle(void)
{
    assert(rlDcBegin(RL_TRIANGLES));
    rlDcAppendVertex(0.0f, 0.0f, 0.0f);
    rlDcAppendVertex(1.0f, 0.0f, 0.0f);
    rlDcAppendVertex(0.0f, 1.0f, 0.0f);
    rlDcEnd();
}

static void quad(void)
{
    assert(rlDcBegin(RL_QUADS));
    rlDcAppendVertex(0.0f, 0.0f, 0.0f);
    rlDcAppendVertex(1.0f, 0.0f, 0.0f);
    rlDcAppendVertex(1.0f, 1.0f, 0.0f);
    rlDcAppendVertex(0.0f, 1.0f, 0.0f);
    rlDcEnd();
}

static void capturedVertex(float x, float y, float z)
{
    if (rlDcBatch.active > 0) rlDcAppendVertex(x, y, z);
}

int main(void)
{
    /* The immediate path must remain exactly P3F/T2F/BGRA: no normal
     * sidecar, allocation, normal pointer, or normal-array submission. */
    resetHarness();
    assert(sizeof(RlDcBatchVertex) == 24);
    assert(sizeof(RlDcBatch) < sizeof(rlDcBatch.verts) + 512);
#if !defined(AUDIT_GLKOS_LEGACY)
    {
        unsigned int expectedCapabilities = 0;
#if GL_KOS_HAS_INTERLEAVED_P3T2BGRA
        expectedCapabilities |= GL_KOS_FAST_PATH_INTERLEAVED_P3T2BGRA;
#endif
#if GL_KOS_HAS_FINAL_INTERLEAVED_P3T2BGRA
        expectedCapabilities |= GL_KOS_FAST_PATH_FINAL_INTERLEAVED_P3T2BGRA;
#endif
#if GL_KOS_HAS_TRUSTED_FINAL_INTERLEAVED_P3T2BGRA
        expectedCapabilities |=
            GL_KOS_FAST_PATH_TRUSTED_FINAL_INTERLEAVED_P3T2BGRA;
#endif
        assert(glKosGetFastPathCapabilities() == expectedCapabilities);
    }
#endif

#ifdef AUDIT_TEST_MIN_THRESHOLD
    /* A below-cutoff batch must skip N3 entirely (it is not an N3 fallback),
     * while the first eligible batch still takes the normal checked route. */
    triangle();
    rlDcFlushAll();
    assert(drawCount == 1 && drawSizes[0] == 3);
    assert(n3TryCount == 0 && trustedN3TryCount == 0);
    assert(f1TryCount == 1 && f1Mode == GL_TRIANGLES);
    assert(routeCount == 1 && routes[0] == AUDIT_ROUTE_F1);
    assertRouteStats(0, 0, 1, 0);
    assertFlushAccounting();

    resetHarness();
    quad();
    rlDcFlushAll();
    assert(drawCount == 1 && drawSizes[0] == 4);
    assert(n3TryCount == 1 && n3Mode == GL_QUADS);
    assert(trustedN3TryCount == 0 && f1TryCount == 0);
    assert(routeCount == 1 && routes[0] == AUDIT_ROUTE_N3);
    assertRouteStats(1, 0, 0, 0);
    assertFlushAccounting();
    return 0;
#endif

    triangle();
    rlDcFlushAll();
    assert(drawCount == 1 && drawSizes[0] == 3);
    assert(!normalArrayEnabled && normalPointerCount == 0);
#if RLDC_HAS_FINAL_INTERLEAVED_P3T2BGRA
#if AUDIT_DEFAULT_N3_CHECKED
    assert(n3TryCount == 1 && n3Mode == GL_TRIANGLES);
    assert(trustedN3TryCount == 0);
    assert(f1TryCount == 0);
    assert(routeCount == 1 && routes[0] == AUDIT_ROUTE_N3);
#else
    assert(trustedN3TryCount == 1 && trustedN3Mode == GL_TRIANGLES);
    assert(n3TryCount == 0 && f1TryCount == 0);
    assert(routeCount == 1 && routes[0] == AUDIT_ROUTE_N3_TRUSTED);
#endif
    assert(clientStateCalls == 0 && pointerCalls == 0);
    assert(arrayDrawCount == 0 && triangleLaneCount == 0);
    assertRouteStats(1, 0, 0, 0);
#elif RLDC_HAS_INTERLEAVED_P3T2BGRA
    assert(n3TryCount == 0 && trustedN3TryCount == 0);
    assert(f1TryCount == 1 && f1Mode == GL_TRIANGLES);
    assert(routeCount == 1 && routes[0] == AUDIT_ROUTE_F1);
    assert(clientStateCalls == 0 && pointerCalls == 0);
    assert(arrayDrawCount == 0 && triangleLaneCount == 0);
    assertRouteStats(0, 0, 1, 0);
#else
    assert(n3TryCount == 0 && trustedN3TryCount == 0 && f1TryCount == 0);
    assert(routeCount == 1 && routes[0] == AUDIT_ROUTE_CLIENT);
    assert(clientStateCalls == 7 && pointerCalls == 3);
    assert(arrayDrawCount == 0 && triangleLaneCount == 1);
    assertRouteStats(0, 0, 0, 0);
#endif
    assertFlushAccounting();

#if defined(GLDC_NATIVE_BENCH) && GLDC_NATIVE_BENCH && \
    RLDC_HAS_FINAL_INTERLEAVED_P3T2BGRA
    /* Switching with queued geometry must flush it through the old checked
     * route. Only later geometry may use the trusted seam. */
    resetHarness();
    triangle();
    rlDcNativeBenchSelectN3RouteInternal(AUDIT_SELECTOR_N3_TRUSTED);
    assert(n3TryCount == 1 && trustedN3TryCount == 0);
    assert(routeCount == 1 && routes[0] == AUDIT_ROUTE_N3);
    triangle();
    rlDcFlushAll();
    assert(trustedN3TryCount == 1 && trustedN3Mode == GL_TRIANGLES);
    assert(n3TryCount == 1 && f1TryCount == 0);
    assert(routeCount == 2 && routes[1] == AUDIT_ROUTE_N3_TRUSTED);
    assert(drawCount == 2 && drawSizes[0] == 3 && drawSizes[1] == 3);
    assertRouteStats(2, 0, 0, 0);
    assertFlushAccounting();

    /* A trusted decline must retain the exact trusted -> F1 fallback order;
     * the ordinary checked constructor remains bypassed. */
    resetHarness();
    rlDcNativeBenchSelectN3RouteInternal(AUDIT_SELECTOR_N3_TRUSTED);
    trustedN3Accept = 0;
    triangle();
    rlDcFlushAll();
    assert(trustedN3TryCount == 1 && n3TryCount == 0 && f1TryCount == 1);
    assert(routeCount == 2 && routes[0] == AUDIT_ROUTE_N3_TRUSTED &&
           routes[1] == AUDIT_ROUTE_F1);
    assert(drawCount == 1 && drawSizes[0] == 3);
    assertRouteStats(0, 1, 1, 0);
    assertFlushAccounting();

    /* The F1 control must bypass both N3 symbols in the same archive. */
    resetHarness();
    rlDcNativeBenchSelectN3RouteInternal(AUDIT_SELECTOR_F1_ONLY);
    triangle();
    rlDcFlushAll();
    assert(trustedN3TryCount == 0 && n3TryCount == 0 && f1TryCount == 1);
    assert(routeCount == 1 && routes[0] == AUDIT_ROUTE_F1);
    assert(drawCount == 1 && drawSizes[0] == 3);
    assertRouteStats(0, 0, 1, 0);
    assertFlushAccounting();
#endif

    resetHarness();
    quad();
    rlDcFlushAll();
    assert(drawCount == 1 && drawSizes[0] == 4);
#if RLDC_HAS_FINAL_INTERLEAVED_P3T2BGRA
#if AUDIT_DEFAULT_N3_CHECKED
    assert(n3TryCount == 1 && n3Mode == GL_QUADS);
    assert(trustedN3TryCount == 0);
    assert(f1TryCount == 0);
    assert(routeCount == 1 && routes[0] == AUDIT_ROUTE_N3);
#else
    assert(trustedN3TryCount == 1 && trustedN3Mode == GL_QUADS);
    assert(n3TryCount == 0 && f1TryCount == 0);
    assert(routeCount == 1 && routes[0] == AUDIT_ROUTE_N3_TRUSTED);
#endif
    assert(clientStateCalls == 0 && pointerCalls == 0);
    assert(arrayDrawCount == 0 && triangleLaneCount == 0);
    assertRouteStats(1, 0, 0, 0);
#elif RLDC_HAS_INTERLEAVED_P3T2BGRA
    assert(n3TryCount == 0 && trustedN3TryCount == 0);
    assert(f1TryCount == 1 && f1Mode == GL_QUADS);
    assert(routeCount == 1 && routes[0] == AUDIT_ROUTE_F1);
    assert(clientStateCalls == 0 && pointerCalls == 0);
    assert(arrayDrawCount == 0 && triangleLaneCount == 0);
    assertRouteStats(0, 0, 1, 0);
#else
    assert(n3TryCount == 0 && trustedN3TryCount == 0 && f1TryCount == 0);
    assert(routeCount == 1 && routes[0] == AUDIT_ROUTE_CLIENT);
    assert(clientStateCalls == 7 && pointerCalls == 3);
    assert(arrayDrawCount == 1 && triangleLaneCount == 0);
    assertRouteStats(0, 0, 0, 0);
#endif
    assertFlushAccounting();

#if RLDC_HAS_FINAL_INTERLEAVED_P3T2BGRA
    /* The native benchmark and standalone raylib default to checked N3;
     * HyperSolar's explicit finite-producer contract selects trusted N3. A
     * decline reaches F1 when compiled, otherwise exact client arrays. */
    resetHarness();
#if AUDIT_DEFAULT_N3_CHECKED
    n3Accept = 0;
#else
    trustedN3Accept = 0;
#endif
    triangle();
    rlDcFlushAll();
#if AUDIT_DEFAULT_N3_CHECKED
    assert(n3TryCount == 1 && n3Mode == GL_TRIANGLES);
    assert(trustedN3TryCount == 0);
#else
    assert(trustedN3TryCount == 1 && trustedN3Mode == GL_TRIANGLES);
    assert(n3TryCount == 0);
#endif
    assert(drawCount == 1 && drawSizes[0] == 3);
#if RLDC_HAS_INTERLEAVED_P3T2BGRA
    assert(f1TryCount == 1 && f1Mode == GL_TRIANGLES);
    assert(routeCount == 2 &&
#if AUDIT_DEFAULT_N3_CHECKED
           routes[0] == AUDIT_ROUTE_N3 &&
#else
           routes[0] == AUDIT_ROUTE_N3_TRUSTED &&
#endif
           routes[1] == AUDIT_ROUTE_F1);
    assert(clientStateCalls == 0 && pointerCalls == 0);
    assert(arrayDrawCount == 0 && triangleLaneCount == 0);
    assertRouteStats(0, 1, 1, 0);
#else
    assert(f1TryCount == 0);
    assert(routeCount == 2 &&
#if AUDIT_DEFAULT_N3_CHECKED
           routes[0] == AUDIT_ROUTE_N3 &&
#else
           routes[0] == AUDIT_ROUTE_N3_TRUSTED &&
#endif
           routes[1] == AUDIT_ROUTE_CLIENT);
    assert(clientStateCalls == 7 && pointerCalls == 3);
    assert(arrayDrawCount == 0 && triangleLaneCount == 1);
    assertRouteStats(0, 1, 0, 0);
#endif
    assertFlushAccounting();

#if RLDC_HAS_INTERLEAVED_P3T2BGRA
    /* When both typed routes decline, client arrays are installed only after
     * the observable trusted/checked-N3 -> F1 attempt order. */
    resetHarness();
#if AUDIT_DEFAULT_N3_CHECKED
    n3Accept = 0;
#else
    trustedN3Accept = 0;
#endif
    f1Accept = 0;
    triangle();
    rlDcFlushAll();
    assert(f1TryCount == 1);
#if AUDIT_DEFAULT_N3_CHECKED
    assert(n3TryCount == 1 && trustedN3TryCount == 0);
#else
    assert(trustedN3TryCount == 1 && n3TryCount == 0);
#endif
    assert(routeCount == 3 &&
#if AUDIT_DEFAULT_N3_CHECKED
           routes[0] == AUDIT_ROUTE_N3 &&
#else
           routes[0] == AUDIT_ROUTE_N3_TRUSTED &&
#endif
           routes[1] == AUDIT_ROUTE_F1 &&
           routes[2] == AUDIT_ROUTE_CLIENT);
    assert(drawCount == 1 && drawSizes[0] == 3);
    assert(clientStateCalls == 7 && pointerCalls == 3);
    assert(arrayDrawCount == 1 && triangleLaneCount == 0);
    assertRouteStats(0, 1, 0, 1);
    assertFlushAccounting();
#endif

#elif RLDC_HAS_INTERLEAVED_P3T2BGRA
    /* With no N3 feature, a declined F1 try falls straight through to the
     * exact client-array path. */
    resetHarness();
    f1Accept = 0;
    triangle();
    rlDcFlushAll();
    assert(n3TryCount == 0 && trustedN3TryCount == 0 && f1TryCount == 1);
    assert(routeCount == 2 && routes[0] == AUDIT_ROUTE_F1 &&
           routes[1] == AUDIT_ROUTE_CLIENT);
    assert(drawCount == 1 && drawSizes[0] == 3);
    assert(clientStateCalls == 7 && pointerCalls == 3);
    assert(arrayDrawCount == 1 && triangleLaneCount == 0);
    assertRouteStats(0, 0, 0, 1);
    assertFlushAccounting();
#endif

    /* Unsupported non-line modes are a real mode boundary. Their nonempty
     * flush must be classified so total flushes equal the cause sum. */
    resetHarness();
    triangle();
    assert(!rlDcBegin(99));
    assert(drawCount == 1 && drawSizes[0] == 3);
#ifdef RLDC_ENABLE_STATS
    assert(rlDcBatch.statFlushModeChange == 1);
#endif
    assertFlushAccounting();

    /* PVR-list-hazardous lines remain callable but submit no vertices and do
     * not flush or contaminate already queued area geometry. */
    resetHarness();
    triangle();
    assert(rlDcBegin(RL_LINES));
    assert(rlDcBatch.active < 0);
    capturedVertex(2.0f, 2.0f, 0.0f);
    capturedVertex(3.0f, 3.0f, 0.0f);
    rlDcEnd();
    assert(rlDcBatch.count == 3 && drawCount == 0);
    rlDcFlushAll();
    assert(drawCount == 1 && drawSizes[0] == 3);
    assertFlushAccounting();

    /* Even an oversized no-op line block cannot flush scratch vertices. */
    resetHarness();
    assert(rlDcBegin(RL_LINES));
    for (int i = 0; i < RLDC_BATCH_CAPACITY + RLDC_BATCH_OVERFLOW + 17; i++)
        capturedVertex((float)i, 0.0f, 0.0f);
    rlDcEnd();
    rlDcFlushAll();
    assert(drawCount == 0 && rlDcBatch.count == 0);
    assertFlushAccounting();

    /* A forced state flush inside an invalid line Begin submits only the area
     * geometry queued before it; the remaining line vertices stay discarded. */
    resetHarness();
    triangle();
    assert(rlDcBegin(RL_LINES));
    capturedVertex(2.0f, 2.0f, 0.0f);
    rlDcFlushOnStateChange();
    assert(drawCount == 1 && drawSizes[0] == 3);
    capturedVertex(3.0f, 3.0f, 0.0f);
    rlDcEnd();
    rlDcFlushAll();
    assert(drawCount == 1);
    assertFlushAccounting();

    /* The optimized id->draw->0->same-id sequence remains one batch. */
    resetHarness();
    rlDcSetTexture(7);
    triangle();
    rlDcSetTexture(0);
    rlDcSetTexture(7);
    assert(drawCount == 0);
    rlDcFlushAll();
    assert(drawCount == 1 && drawSizes[0] == 3);
    assert(textureEnabled && boundTexture == 7);
    assertFlushAccounting();

    /* A real untextured draw is a boundary; it cannot inherit texture 7. */
    resetHarness();
    rlDcSetTexture(7);
    triangle();
    rlDcSetTexture(0);
    triangle();
    assert(drawCount == 1 && drawSizes[0] == 3);
    rlDcFlushAll();
    assert(drawCount == 2 && drawSizes[1] == 3);
    assert(!textureEnabled);
    assertFlushAccounting();

    /* A direct texture bind supersedes a deferred unbind. Its barrier may
     * submit old geometry but must not emit disable+bind(0) first. */
    resetHarness();
    rlDcSetTexture(9);
    triangle();
    rlDcSetTexture(0);
    rlDcExternalTextureBarrier();
    assert(drawCount == 1);
    assert(textureEnableCount == 1 && textureDisableCount == 0);
    assert(boundTexture == 9 && zeroBindCount == 0);
    assert(!rlDcBatch.pendingUnbind);
    assertFlushAccounting();

    /* Semantic rlColor RGBA lands directly in physical native BGRA bytes:
     * r=0x11 g=0x22 b=0x33 a=0x44 packs to the little-endian word 0x44112233
     * (byte order in memory: b, g, r, a). */
    resetHarness();
    rlDcBatch.curBGRA = rlDcPackBGRA(0x11, 0x22, 0x33, 0x44);
    assert(rlDcBegin(RL_TRIANGLES));
    rlDcAppendVertex(0.0f, 0.0f, 0.0f);
    assert(rlDcBatch.verts[0].bgra == 0x44112233u);
    assert(((const unsigned char *)&rlDcBatch.verts[0].bgra)[0] == 0x33);
    assert(((const unsigned char *)&rlDcBatch.verts[0].bgra)[1] == 0x22);
    assert(((const unsigned char *)&rlDcBatch.verts[0].bgra)[2] == 0x11);
    assert(((const unsigned char *)&rlDcBatch.verts[0].bgra)[3] == 0x44);
    assertFlushAccounting();

    /* Cold external state work still resolves an unbind conservatively. */
    resetHarness();
    rlDcSetTexture(9);
    triangle();
    rlDcSetTexture(0);
    /* This header-level fixture does not compile rlgl.h's public wrapper. */
    rlDcExternalStateBarrierInternal();
    assert(drawCount == 1 && !textureEnabled && boundTexture == 0);
    assert(!rlDcBatch.pendingUnbind && !rlDcBatch.lastFlushedTexValid);
    assertFlushAccounting();

    return 0;
}
