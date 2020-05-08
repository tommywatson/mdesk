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

/*
 * Add these flags to mingw linker to get
 * rid of the console on a real win32 install
 * -Wl,-subsystem,windows
 */

#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

// Desktop current window information
typedef struct {
    uint32_t _n_hwnd;
    uint32_t _max_hwnd;
    HWND *_hwnds;
} Desktop;

// Ignore windows that have this in their title
typedef struct s_Ignore {
    struct s_Ignore *_next;
    const char *_name;
} Ignore;

// Global working data
typedef struct {
    uint8_t _log;
    FILE *_fp;
    char _logfile[256];
    uint32_t _locked;
    uint32_t _always_on_top;
    HINSTANCE _hinstance;
    HWND _hwnd;
    HHOOK _mouse_hook;
    uint32_t _x,_y,_w,_h;
    uint8_t _dragging;
    uint32_t _dx,_dy;
    uint32_t _button;
    HWND *_buttons;
    uint32_t _n_buttons;
    uint32_t _button_w;
    uint32_t _button_space;
    Desktop *_desktops;
    Ignore *_ignore;
} State;

// Global variables
static char szWindowClass[] = "mdesk";
static char szShackledragger[] = "shackledragger";
static State _state;

#define     ID_Exit             0x800
#define     ID_About            0x801
#define     ID_Lock             0x802
#define     ID_AOT              0x803

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// dumb logger
#define Log(text,...)   if(_state._fp){fprintf(_state._fp,text,##__VA_ARGS__); \
                            fflush(_state._fp);}

// Windows Main
int CALLBACK WinMain(
   _In_ HINSTANCE hInstance,
   _In_opt_ HINSTANCE hPrevInstance,
   _In_ LPSTR     lpCmdLine,
   _In_ int       nCmdShow
)
{
    void load_registry(void);
    (void)hPrevInstance;
    (void)lpCmdLine;
    WNDCLASSA wcex;
    HBRUSH brush = CreateSolidBrush(RGB(0x00,0x0b,0x59));

    wcex.style          = CS_HREDRAW|CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance,IDI_APPLICATION);
    wcex.hCursor        = LoadCursor(NULL,IDC_ARROW);
    wcex.hbrBackground  = brush;
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = szWindowClass;
    if (!RegisterClassA(&wcex)) {
        MessageBoxA(NULL,"Call to RegisterClassEx failed!",szWindowClass,0);
        return 1;
    }

    // Get the user dir for the logger
    ExpandEnvironmentStrings("%HOMEDRIVE%%HOMEPATH%",
                             _state._logfile,
                             sizeof(_state._logfile)/sizeof(*_state._logfile));
    // should really make sure this fits
    strcat(_state._logfile,"\\mdesk.log");

    //++_state._log;
    if(_state._log) {
#       ifndef __MINGW64_VERSION_MAJOR
#           pragma warning(disable : 4996)
#       endif
        _state._fp=fopen(_state._logfile,"wt");
    }

    // init
    load_registry();
    _state._hinstance=hInstance;
    _state._n_buttons=5;
    _state._button_w=20;
    _state._button_space=2;
    _state._w=_state._n_buttons*_state._button_w+5;
    _state._h=_state._button_w+4;
    _state._hwnd=CreateWindowExA(WS_EX_TOOLWINDOW,
                                 szWindowClass,
                                 szWindowClass,
                                 WS_POPUP,
                                 _state._x,
                                 _state._y,
                                 _state._w,
                                 _state._h,
                                 0,
                                 0,
                                 hInstance,
                                 0);

    if (!_state._hwnd) {
        MessageBoxA(NULL,"Call to CreateWindow failed!",szWindowClass,0);
        return 1;
    }
    void create_buttons(void);
    // Create the buttons
    create_buttons();

    // Show the window
    ShowWindow(_state._hwnd,nCmdShow);
    UpdateWindow(_state._hwnd);

    // Main message loop:
    MSG msg;
    while(GetMessage(&msg,0,0,0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    // Free up the brush
    DeleteObject(brush);

    // clean up memory
    for(uint32_t i=0;i<_state._n_buttons;++i) {
        free((_state._desktops+i)->_hwnds);
    }
    for(Ignore *ignore=_state._ignore;ignore;) {
        Ignore *tmp=ignore;
        ignore=ignore->_next;
        free(tmp);
    }

    // cleanup the logger
    if(_state._fp) {
        fclose(_state._fp);
    }

    return (int)msg.wParam;
}

/**
 * @brief Open the registry
 * @param key
 * @param path
 * @return key
 */
HKEY open_reg(HKEY *key) {
    char name[256];
    sprintf(name,"Software\\%s",szShackledragger);
    if(ERROR_SUCCESS!=RegCreateKeyExA(HKEY_CURRENT_USER,
                                      name,
                                      0,
                                      0,
                                      REG_OPTION_NON_VOLATILE,
                                      KEY_ALL_ACCESS,
                                      0,
                                      key,
                                      0)) {
        Log("Cannot open key\n");
        *key=INVALID_HANDLE_VALUE;
    }
    return *key;
}

/**
 * @brief Save a uint32 to the registry
 * @param key
 * @param value
 */
void save(char *key,uint32_t value) {
    HKEY hkey=open_reg(&hkey);

    Log("Saving %s = %d\n",key,value);
    if(hkey!=INVALID_HANDLE_VALUE) {
        LSTATUS status=RegSetValueExA(hkey,
                                      key,
                                      0,
                                      REG_DWORD,
                                      (void *)&value,sizeof(value));
        if(status!=ERROR_SUCCESS) {
             Log("Failed to write %s = %d [%ld:%0lx]\n",key,value,status,status);
         }
        RegCloseKey(hkey);
    }
}

/**
 * @brief Save a uint32 to the registry
 * @param key
 * @param value
 */
void load(char *key,uint32_t *value) {
    HKEY hkey=open_reg(&hkey);

    Log("Reading %s\n",key);
    if(hkey!=INVALID_HANDLE_VALUE) {
        DWORD type,length=sizeof(value);
        RegQueryValueExA(hkey,key,0,&type,(void *)value,&length);
        RegCloseKey(hkey);
        Log("Read %s = %d\n",key,*value);
    }
}

/**
 * @brief Save the position to the registry
 */
void save_position(void) {
    save("x",_state._x);
    save("y",_state._y);
    save("buttons",_state._n_buttons);
    save("locked",_state._locked);
    save("always on top",_state._always_on_top);
}

/**
 * @brief Load status from
 */
void load_registry(void) {
    load("x",&_state._x);
    load("y",&_state._y);
    load("buttons",&_state._n_buttons);
    load("locked",&_state._locked);
    load("always on top",&_state._always_on_top);
}

/**
 * @brief about
 */
void about(void) {
    MessageBox(_state._hwnd,
               "mdesk - braindead multiple desktops\n"
               "(c) 2020 Shackledragger LLC. All Rights Reserved.",
               "mdesk - about",
               MB_OK);
}

/**
 * @brief Ask if the user wants to exit
 */
void ask_exit(void) {
    if(MessageBoxA(0,
                   "Are you sure you want to quit?",
                   "MDesk - Exit?",
                   MB_SYSTEMMODAL|MB_OKCANCEL)==IDOK) {
        // close the window
        SendMessage(_state._hwnd,WM_CLOSE,0,0);
    }
}

/**
 * @brief Position the window
 */
void position_window(void) {
    // set position, keep on top
    if(_state._always_on_top) {
        SetWindowPos(_state._hwnd,
                     HWND_TOPMOST,
                     _state._x,
                     _state._y,
                     _state._w,
                     _state._h,
                     SWP_SHOWWINDOW);
    }
    else {
        // just move
        MoveWindow(_state._hwnd,
                   _state._x,
                   _state._y,
                   _state._w,
                   _state._h,
                   0);
    }
}

/**
 * @brief Ignore this window?
 * @param name
 * @return true/false
 */
uint8_t ignore(const char *name) {
    uint8_t ignore=!(name&&*name);

    if(!ignore) {
        for(Ignore *check=_state._ignore;check;check=check->_next) {
            //if(!strcmpi(name,check->_name)) {
            if(strstr(name,check->_name)) {
                ++ignore;
                Log("[I] %s\n",name);
                break;
            }
        }
    }
    return ignore;
}

/**
 * @brief Add a title to ignore list
 * @param name
 */
void add_ignore(const char *name) {
    Ignore *ignore=(Ignore *)malloc(sizeof(Ignore));
    if(ignore) {
        ignore->_name=name;
        ignore->_next=_state._ignore;
        _state._ignore=ignore;
    }
}

/**
 * @brief Set the default ignore list
 */
void default_ignores(void) {
    add_ignore(szWindowClass);
    add_ignore("Program Manager");
}

/**
 * @brief print the current state
 */
void print(void) {
    Log("\n******************* +++ %ld\n",(long)time(0));
    for(uint32_t i=0;i<_state._n_buttons;++i) {
		Desktop *desktop = _state._desktops + i;
		Log("[%d]:\n",i);
		for (uint32_t w = 0; w < desktop->_n_hwnd; ++w) {
			Log("  [%d][%d] 0x%p\n",
                i,
                w,
                (void *)*((_state._desktops+i)->_hwnds+w));
        }
    }
    Log("******************* ---\n");
}

/**
 * @brief Check if we are hiding this window
 * @return true/false
 */
uint32_t hiding(HWND hwnd) {
    uint32_t hiding=0;
    for(uint32_t i=0;!hiding&&i<_state._n_buttons;++i) {
		Desktop *desktop = _state._desktops + i;
		for (uint32_t w = 0; w < desktop->_n_hwnd; ++w) {
            if(*(desktop->_hwnds+w)==hwnd) {
                hiding=1;
                break;
            }
        }
    }
    return hiding;
}

/**
 * @brief Show all hidden windows
 */
void show_all(void) {
    for (uint32_t i = 0; i < _state._n_buttons; ++i) {
        Desktop *desktop = _state._desktops + i;
        for (uint32_t w = 0; w < desktop->_n_hwnd; ++w) {
            ShowWindowAsync(*(desktop->_hwnds + w), SW_SHOW);
        }
        // cleanup this desktop
        desktop->_n_hwnd=0;
    }
}

/**
 * @brief Show the current button's windows
 */
void show_windows(void) {
	Desktop *desktop = _state._desktops + _state._button;
	for (uint32_t i = 0; i < desktop->_n_hwnd; ++i) {
		ShowWindowAsync(*(desktop->_hwnds + desktop->_n_hwnd - i - 1), SW_SHOW);
	}
	// clear the list
	desktop->_n_hwnd = 0;
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
            case WM_RBUTTONUP: {
                //unhook();
                break;
            }
            case WM_MOUSEMOVE: {
                MOUSEHOOKSTRUCT *ptr=(MOUSEHOOKSTRUCT *)l_param;
                _state._x=ptr->pt.x-_state._dx;
                _state._y=ptr->pt.y-_state._dy;
                position_window();
                break;
            }
        }
    }
    return CallNextHookEx(_state._mouse_hook,code,w_param,l_param);
}

/**
 * @brief Left button down
 * @param w_param
 * @param l_param
 */
void wm_lbuttondown(WPARAM w_param,LPARAM l_param) {
    (void)w_param;
    if(!_state._locked&&!_state._dragging) {
        _state._dragging++;
        _state._dx=GET_X_LPARAM(l_param);
        _state._dy=GET_Y_LPARAM(l_param);

        // hook it or you're nobody
        /*
        if(_state._mouse_hook==0) {
            _state._mouse_hook=SetWindowsHookEx(WH_MOUSE_LL,mouse_hook,0,0);
            if(!_state._mouse_hook) {
                MessageBeep(MB_ICONERROR);
            }
        }
        else {
            MessageBeep(MB_ICONERROR);
        }
         */
    }
}

/**
 * @brief Right button down
 * @param w_param
 * @param l_param
 */
void wm_rbuttondown(WPARAM w_param, LPARAM l_param) {
    (void)w_param;
    HMENU hmenu=CreatePopupMenu();
    if(hmenu) {
        AppendMenuA(hmenu,MF_SEPARATOR,0,0);
        AppendMenuA(hmenu,MF_STRING,1,"Application");
        AppendMenuA(hmenu,MF_SEPARATOR,0,0);
        AppendMenuA(hmenu,MF_STRING,ID_AOT,"Always On &Top");
        AppendMenuA(hmenu,MF_STRING,ID_Lock,"&Lock");
        AppendMenuA(hmenu,MF_STRING,ID_About,"&About");
        AppendMenuA(hmenu,MF_STRING,ID_Exit,"E&xit");
        // Pop up!
        uint32_t n=TrackPopupMenuEx(hmenu,
                         TPM_BOTTOMALIGN|TPM_RETURNCMD,
                         _state._x+GET_X_LPARAM(l_param),
                         _state._y+GET_Y_LPARAM(l_param),
                         _state._hwnd,
                         0);
        DestroyMenu(hmenu);
        switch(n) {
            case ID_Lock:
                ++_state._locked;
                _state._locked&=1;
                save_position();
                break;
            case ID_AOT:
                ++_state._always_on_top;
                _state._always_on_top&=1;
                save_position();
                position_window();
                break;
            case ID_About:
                about();
                break;
            case ID_Exit:
                ask_exit();
                break;
            default:
                break;
        }
    }
    else {
        Log("Cannot create menu\n");
    }
}

/**
 * @brief Mouse up
 * @param w_param
 * @param l_param
 */
void wm_lbuttonup(WPARAM w_param,LPARAM l_param) {
    (void)w_param;
    (void)l_param;
    if(_state._dragging) {
        _state._dragging=0;
        save_position();
    }
    if (_state._mouse_hook) {
        //UnhookWindowsHookEx(_state._mouse_hook);
        _state._mouse_hook = 0;
    }
}

/**
 * @brief Mouse move
 * @param w_param
 * @param l_param
 */
void wm_mousemove(WPARAM w_param,LPARAM l_param) {
    (void)w_param;
    if(_state._dragging) {
        // move to the new place
        _state._x+=GET_X_LPARAM(l_param)-_state._dx;
        _state._y+=GET_Y_LPARAM(l_param)-_state._dy;
        position_window();
        Log("[%d, %d]\n",_state._x,_state._y);
    }
}

/**
 * @brief Get the button index from clicked hwnd
 * @param l_param
 * @return window
 */
uint32_t get_button(LPARAM l_param) {
    uint32_t button=0;
    for(uint32_t i=0;i<_state._n_buttons;++i) {
        if(*(_state._buttons+i)==(HWND)l_param) {
            button=i;
            break;
        }
    }
    return button;
}

/**
 * @brief Handle windows enumeration callback
 * @param hwnd
 * @param l_param
 * @return
 */
BOOL CALLBACK enum_windows(HWND hwnd,LPARAM l_param) {
    char name[128];
    // Get window name
    GetWindowTextA(hwnd,name,sizeof(name)/sizeof(*name));
    // don't hide hidden windows or windows on the ignore list
    if(!(!IsWindowVisible(hwnd)||ignore(name))) {
        Log("%ld Hiding [%s]\n",(long)time(0),(char *)name);
        // hide the window
        ShowWindowAsync(hwnd,SW_HIDE);
        // save it to the windows list, auto resize array if needed
        Desktop *desktop=_state._desktops+l_param;
        if(desktop->_n_hwnd>=desktop->_max_hwnd) {
            // need a resize
            HWND *hwnds=desktop->_hwnds;
            desktop->_max_hwnd=desktop->_max_hwnd?desktop->_max_hwnd<<1:1;
            desktop->_hwnds=(HWND *)malloc(sizeof(HWND)*desktop->_max_hwnd);
            // copy, memcpy is probably faster here...
            for(uint32_t i=0;i<desktop->_n_hwnd;++i) {
                *(desktop->_hwnds+i)=*(hwnds+i);
            }
            // clean up
            if(hwnds) {
                free(hwnds);
            }
        }
        // save the window
        *(desktop->_hwnds+desktop->_n_hwnd++)=hwnd;
    }
    return TRUE;
}

/**
 * @brief Handle the button click
 * @param w_param
 * @param l_param
 */
void wm_command(WPARAM w_param,LPARAM l_param) {
    switch(w_param) {
        case BN_CLICKED: {
            uint32_t button=get_button(l_param);
            // redundant not same button check
            if(button!=_state._button) {
                // Save active
                //(_state._desktops+_state._button)->_active=GetActiveWindow();
                // save the window info/hide windows
                EnumWindows((WNDENUMPROC)enum_windows,_state._button);
                // save new button, set enabled on old button
                EnableWindow(*(_state._buttons+_state._button),1);
                _state._button=button;
                EnableWindow(*(_state._buttons+_state._button),0);
                // show any windows hidden by this button
                show_windows();
                // log for testing
                print();
            }
            break;
        }
    }
}

/**
 * @brief Message handler
 * @param hWnd
 * @param message
 * @param w_param
 * @param l_param
 */
LRESULT CALLBACK WndProc(HWND hWnd,UINT message,WPARAM w_param,LPARAM l_param)
{
    switch(message)
    {
        case WM_DESTROY:
            // show all the hidden windows
            show_all();
            // cya!
            PostQuitMessage(0);
            break;
        case WM_LBUTTONDOWN:    // setup drag
            wm_lbuttondown(w_param,l_param);
            break;
		case WM_LBUTTONUP:      // cleanup drag
			wm_lbuttonup(w_param, l_param);
			break;
        case WM_MOUSEMOVE:      // handle drag
            wm_mousemove(w_param,l_param);
            break;
        case WM_RBUTTONDOWN:    // show popup menu
            wm_rbuttondown(w_param,l_param);
            break;
        case WM_COMMAND:
            wm_command(w_param,l_param);
            break;
        default:
            return DefWindowProc(hWnd, message, w_param, l_param);
            break;
    }
    return 0;
}

/**
 * @brief Create the buttons
 */
void create_buttons(void) {
    if(_state._n_buttons<1) {
        _state._n_buttons=5;
    }
    if(_state._buttons) {
        free(_state._buttons);
    }
    if(_state._desktops) {
        free(_state._desktops);
    }
    _state._buttons=(HWND *)malloc(_state._n_buttons*sizeof(HWND));
    _state._desktops=(Desktop *)malloc(_state._n_buttons*sizeof(Desktop));
    memset(_state._desktops,0,_state._n_buttons*sizeof(Desktop));
    for(uint32_t i=0;i<_state._n_buttons;++i) {
        char name[]={0,0};
        *name='1'+i;
        *(_state._buttons+i)=CreateWindowA("BUTTON",
                                           name,
                                           WS_TABSTOP|WS_VISIBLE|WS_CHILD
                                                |BS_DEFPUSHBUTTON,
                                           _state._button_space*(1+i)
                                                +_state._button_w*i,
                                           _state._button_space,
                                           _state._button_w,
                                           _state._button_w,
                                           _state._hwnd,
                                           0,
                                           _state._hinstance,
                                           0);
    }
    _state._w=_state._n_buttons*_state._button_w
              +_state._n_buttons*_state._button_space
              +_state._button_space*4;
    _state._h=_state._button_space*2+_state._button_w;
    position_window();
    EnableWindow(*(_state._buttons+_state._button),0);
    if(!_state._ignore) {
        default_ignores();
    }
}
