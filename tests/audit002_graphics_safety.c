#include "raylib.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

// MinGW places all module log strings in one .rdata section, which keeps a few
// otherwise-dead functions during this focused --gc-sections link. These stubs
// satisfy those functions; none is reached by the tests below.
void TraceLog(int logLevel, const char *text, ...) { (void)logLevel; (void)text; }
void rlTextureParameters(unsigned int id, int param, int value) { (void)id; (void)param; (void)value; }

int main(void)
{
    assert(strcmp(TextSubtext("abcdef", -1, 2), "") == 0);
    assert(strcmp(TextSubtext("abcdef", 2, 99), "cdef") == 0);
    assert(strcmp(TextSubtext("abcdef", 99, 2), "") == 0);

    char *inserted = TextInsert("abcd", "XY", 2);
    assert(inserted != NULL);
    assert(strcmp(inserted, "abXYcd") == 0);
    free(inserted);
    assert(TextInsert("abcd", "XY", -1) == NULL);
    assert(TextInsert("abcd", "XY", 5) == NULL);

    char longDelimiter[900];
    memset(longDelimiter, 'd', sizeof(longDelimiter) - 1);
    longDelimiter[sizeof(longDelimiter) - 1] = '\0';
    const char *parts[] = { "prefix", "suffix" };
    const char *joined = TextJoin(parts, 2, longDelimiter);
    assert(strlen(joined) < 1024);

    char oversized[1400];
    memset(oversized, 'a', sizeof(oversized));
    oversized[700] = ',';
    oversized[sizeof(oversized) - 1] = '\0';
    int splitCount = 0;
    const char **split = TextSplit(oversized, ',', &splitCount);
    assert(splitCount == 2);
    assert(split[0][700] == '\0');
    assert(strlen(split[1]) <= 322);

    int codepointSize = 0;
    const char truncated4[] = { (char)0xf0, '\0' };
    assert(GetCodepointNext(truncated4, &codepointSize) == '?');
    assert(codepointSize == 1);

    float rgba[] = { 0.25f, 0.5f, 0.75f, 1.0f };
    Image floatImage = { rgba, 1, 1, 1, PIXELFORMAT_UNCOMPRESSED_R32G32B32A32 };
    Color sampled = GetImageColor(floatImage, 0, 0);
    assert(sampled.r == 63);
    assert(sampled.g == 127);
    assert(sampled.b == 191);
    assert(sampled.a == 255);

    Image invalid = GenImageColor(-1, 8, WHITE);
    assert(invalid.data == NULL);
    Image solid = GenImageColor(4, 3, (Color){ 1, 2, 3, 4 });
    assert(solid.data != NULL);
    assert(GetImageColor(solid, 3, 2).a == 4);

    Image badPiece = ImageFromImage(solid, (Rectangle){ -1, 0, 2, 2 });
    assert(badPiece.data == NULL);
    Image piece = ImageFromImage(solid, (Rectangle){ 1, 1, 2, 2 });
    assert(piece.data != NULL);

    free(piece.data);
    free(solid.data);
    return 0;
}
