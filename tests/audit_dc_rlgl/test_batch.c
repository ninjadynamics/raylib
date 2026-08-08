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

static int drawCount;
static int drawSizes[8];
static int textureEnabled;
static unsigned int boundTexture;
static int textureEnableCount;
static int textureDisableCount;
static int zeroBindCount;
static int normalArrayEnabled;
static int normalPointerCount;

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
    if (cap == GL_NORMAL_ARRAY) normalArrayEnabled = 1;
}

void glDisableClientState(GLenum cap)
{
    if (cap == GL_NORMAL_ARRAY) normalArrayEnabled = 0;
}

void glVertexPointer(int size, GLenum type, GLsizei stride, const void *pointer)
{ (void)size; (void)type; (void)stride; (void)pointer; }

void glTexCoordPointer(int size, GLenum type, GLsizei stride, const void *pointer)
{ (void)size; (void)type; (void)stride; (void)pointer; }

void glColorPointer(int size, GLenum type, GLsizei stride, const void *pointer)
{ (void)size; (void)type; (void)stride; (void)pointer; }

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
    drawSizes[drawCount++] = count;
}

static void resetHarness(void)
{
    rlDcResetState();
    drawCount = 0;
    textureEnabled = 0;
    boundTexture = 0;
    textureEnableCount = 0;
    textureDisableCount = 0;
    zeroBindCount = 0;
    normalArrayEnabled = 0;
    normalPointerCount = 0;
}

static void triangle(void)
{
    assert(rlDcBegin(RL_TRIANGLES));
    rlDcAppendVertex(0.0f, 0.0f, 0.0f);
    rlDcAppendVertex(1.0f, 0.0f, 0.0f);
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
    triangle();
    rlDcFlushAll();
    assert(drawCount == 1 && drawSizes[0] == 3);
    assert(!normalArrayEnabled && normalPointerCount == 0);

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

    /* Even an oversized no-op line block cannot flush scratch vertices. */
    resetHarness();
    assert(rlDcBegin(RL_LINES));
    for (int i = 0; i < RLDC_BATCH_CAPACITY + RLDC_BATCH_OVERFLOW + 17; i++)
        capturedVertex((float)i, 0.0f, 0.0f);
    rlDcEnd();
    rlDcFlushAll();
    assert(drawCount == 0 && rlDcBatch.count == 0);

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

    /* Semantic rlColor RGBA lands directly in physical native BGRA bytes. */
    resetHarness();
    rlDcBatch.curR = 0x11;
    rlDcBatch.curG = 0x22;
    rlDcBatch.curB = 0x33;
    rlDcBatch.curA = 0x44;
    assert(rlDcBegin(RL_TRIANGLES));
    rlDcAppendVertex(0.0f, 0.0f, 0.0f);
    assert(rlDcBatch.verts[0].b == 0x33);
    assert(rlDcBatch.verts[0].g == 0x22);
    assert(rlDcBatch.verts[0].r == 0x11);
    assert(rlDcBatch.verts[0].a == 0x44);

    /* Cold external state work still resolves an unbind conservatively. */
    resetHarness();
    rlDcSetTexture(9);
    triangle();
    rlDcSetTexture(0);
    rlDcExternalStateBarrier();
    assert(drawCount == 1 && !textureEnabled && boundTexture == 0);
    assert(!rlDcBatch.pendingUnbind && !rlDcBatch.lastFlushedTexValid);

    return 0;
}
