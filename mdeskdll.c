/**
 * mdesk - brain dead multiple desktop for win32
 * © 2020 Shackledragger LLC. All Rights Reserved.
 *
 * http://shackledragger.com
 */

/*
MIT License

Copyright (c) 2020 Shackledragger LLC.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */

#define MDESKDLL_SOURCE
#include "mdeskdll.h"

#include <stdio.h>

// dumb logger
#define Log(text,...)   if(_state._fp){fprintf(_state._fp,text,##__VA_ARGS__); \
                            fflush(_state._fp);}

typedef struct {
    uint8_t _log;
    FILE *_fp;
    HINSTANCE _hinstance;
    HHOOK _mouse_hook;
    HWND _window;
    uint32_t _dx,_dy,_x,_y,_w,_h;
    uint32_t _always_on_top;
    char _logfile[256];
} State;

static State _state;

/**
 * @brief dll main
 */
BOOL WINAPI DllMain
(
    _In_ HINSTANCE hinstDLL,
    _In_ DWORD fdwReason,
    _In_ LPVOID lpvReserved
)
{
    (void)fdwReason;
    (void)lpvReserved;
    _state._hinstance=hinstDLL;
    // Get the user dir for the logger
    ExpandEnvironmentStrings("%HOMEDRIVE%%HOMEPATH%",
                             _state._logfile,
                             sizeof(_state._logfile)/sizeof(*_state._logfile));
    // should really make sure this fits
    strcat(_state._logfile,"\\mdeskdll.log");

    //++_state._log;
    if(_state._log) {
        _state._fp=fopen(_state._logfile,"wt");
    }
    return TRUE;
}

/**
 * @brief Position the window
 */
void position_window(void) {
    // set position, keep on top
    if(_state._always_on_top) {
        SetWindowPos(_state._window,
                     HWND_TOPMOST,
                     _state._x,
                     _state._y,
                     _state._w,
                     _state._h,
                     SWP_SHOWWINDOW);
    }
    else {
        // just move
        MoveWindow(_state._window,
                   _state._x,
                   _state._y,
                   _state._w,
                   _state._h,
                   0);
    }
}

/**
 * @brief Unhook the mouse
 */
void mouse_unhook(void) {
    if(_state._mouse_hook) {
        Log("UNHOOK\n");
        UnhookWindowsHookEx(_state._mouse_hook);
        _state._mouse_hook=0;
    }
}

/**
 * @brief Mouse hook
 * @param code
 * @param w_param
 * @param l_param
 * @return
 */
LRESULT mouse_hook(int code,WPARAM w_param,LPARAM l_param) {
    if(code>=0) {
        switch(w_param) {
            case WM_LBUTTONUP: {
                mouse_unhook();
                break;
            }
            case WM_MOUSEMOVE: {
                POINT p;
                GetCursorPos(&p);
                _state._x=p.x-_state._dx;
                _state._y=p.y-_state._dy;
                position_window();
                break;
            }
        }
    }
    return CallNextHookEx(0,code,w_param,l_param);
}

/**
 * @brief DLL hook mouse call
 * @param window
 * @param dx
 * @param dy
 * @param w
 * @param h
 * @param aot
 */
DLLExport void mdeskdll_hook
(
    HWND window,
    uint32_t dx,
    uint32_t dy,
    uint32_t w,
    uint32_t h,
    uint32_t aot
)
{
    // hook it or you're nobody
    if(_state._mouse_hook==0) {
        Log("HOOK\n");
        _state._window=window;
        _state._dx=dx;
        _state._dy=dy;
        _state._w=w;
        _state._h=h;
        _state._always_on_top=aot;
        if(!_state._mouse_hook) {
            MessageBeep(MB_ICONERROR);
        }
        _state._mouse_hook=SetWindowsHookEx(WH_CALLWNDPROC|WH_MOUSE_LL,mouse_hook,_state._hinstance,0);
    }
    else {
        MessageBeep(MB_ICONERROR);
    }
}

/**
 * @brief DLL unhook mouse
 */
DLLExport void mdeskdll_unhook(void) {
    mouse_unhook();
}