/* Compile twice: once normally and once with -DAUDIT002_DREAMCAST_CONFIG. */
#if defined(AUDIT002_DREAMCAST_CONFIG)
    #define PLATFORM_DREAMCAST
#endif
#include "../src/config.h"

#if defined(AUDIT002_DREAMCAST_CONFIG)
    #if defined(SUPPORT_CLIPBOARD_IMAGE)
        #error Dreamcast must not advertise clipboard image support
    #endif
    #if defined(SUPPORT_FILEFORMAT_BMP) || defined(SUPPORT_FILEFORMAT_JPG)
        #error Dreamcast clipboard gating must not force unused BMP/JPG decoders
    #endif
#else
    #if !defined(SUPPORT_CLIPBOARD_IMAGE)
        #error Existing non-Dreamcast clipboard image configuration changed
    #endif
#endif

int audit002_config_contract(void)
{
    return 0;
}
