#ifndef MDESKDLL_H

#   define MDESKDLL_H

#   include <stdint.h>
#   include <windows.h>

#   ifdef MSDESKDLL_SOURCE
#       define DLLExport __declspec(dllexport)
#   else
#       define DLLExport __declspec(dllimport)
#   endif

    extern DLLExport void mdeskdll_hook(HWND window,
                                        uint32_t dx,
                                        uint32_t dy,
                                        uint32_t w,
                                        uint32_t h,
                                        uint32_t aot);
    extern DLLExport void mdeskdll_unhook(void);

#endif
