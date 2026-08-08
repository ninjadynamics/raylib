#include "raylib.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>

static int callbackCount = 0;

/* utils.c's exported data-code helper references this raylib utility even
 * though the callback test never reaches that path. */
const char *GetFileNameWithoutExt(const char *filePath) { return filePath; }

static void TestLog(int logLevel, const char *text, va_list args)
{
    assert(logLevel == LOG_FATAL);
    assert(strcmp(text, "%s %d") == 0);
    assert(strcmp(va_arg(args, const char *), "owned") == 0);
    assert(va_arg(args, int) == 7);
    callbackCount++;
}

int main(void)
{
    SetTraceLogCallback(TestLog);
    TraceLog(LOG_FATAL, "%s %d", "owned", 7);
    assert(callbackCount == 1);   // Custom callbacks own fatal policy; call must return.
    return 0;
}
