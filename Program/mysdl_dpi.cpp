#include "mysdl_dpi.h"

#include <SDL.h>

void MySDL_GetDisplayDPI(int displayIndex, float* hdpi, float* vdpi, float* defaultDpi)
{
    static const float kSysDefaultDpi =
#ifdef __APPLE__
        72.0f;
#elif defined(_WIN32)
        96.0f;
#else
        static_assert(false, "No system default DPI set for this platform");
#endif

    if (SDL_GetDisplayDPI(displayIndex, NULL, hdpi, vdpi))
    {
        if (hdpi) *hdpi = kSysDefaultDpi;
        if (vdpi) *vdpi = kSysDefaultDpi;
    }

    if (defaultDpi) *defaultDpi = kSysDefaultDpi;
}
