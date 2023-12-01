#include "process.h"
#include <stdio.h>
#include <signal.h>

#ifndef QWORD
typedef unsigned __int64 QWORD;
#endif

int ABORT = 0;

// to receive events for the rawkeyboard data
LRESULT CALLBACK wndproc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch(message)
    {
    case WM_DESTROY:
        return 0;
    case WM_INPUT:
        char rid_buf[64];
        UINT rid_size = sizeof(rid_buf);

        if(GetRawInputData((HRAWINPUT)lparam, RID_INPUT, rid_buf, &rid_size, sizeof(RAWINPUTHEADER)))
        {
            RAWINPUT * raw = (RAWINPUT*)rid_buf;
            if(raw->header.dwType == RIM_TYPEKEYBOARD)
            {
                RAWKEYBOARD * rk = &raw->data.keyboard;
                process_kbd_event(rk->MakeCode,
                    rk->Flags & RI_KEY_E0,
                    rk->Flags & RI_KEY_E1,
                    rk->Flags & RI_KEY_BREAK,
                    rk->VKey
                );
            }
        }
        break;
    }
    return DefWindowProc(window, message, wparam, lparam);
}

void sig_handler(int signal)
{
    fprintf(stderr, "> exiting..\n");
    if(signal == SIGABRT || signal == SIGINT)
        ABORT = 1;
}

int main(int ac, char ** av)
{
    fprintf(stderr, "> starting..\n");

    //define a window class which is required to receive RAWINPUT events
    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(WNDCLASSEX));
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.lpfnWndProc   = wndproc;
    wc.hInstance     = GetModuleHandle(NULL);
    wc.lpszClassName = "rawkbd_wndclass";

    // register class
    if(!RegisterClassExA(&wc))
        return -1;

    // create window
    HWND rawkbd_wnd = CreateWindowExA(0, wc.lpszClassName, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, GetModuleHandle(NULL), NULL);
    if(!rawkbd_wnd)
        return -2;

    // setup raw input device sink
    RAWINPUTDEVICE devs = { 0x01 /* generic */, 0x06 /* keyboard */, RIDEV_INPUTSINK, rawkbd_wnd };
    if(RegisterRawInputDevices(&devs, 1, sizeof(RAWINPUTDEVICE)) == FALSE)
        return -3;

    // loop until ctrl+c
    signal(SIGINT, sig_handler);
    MSG msg;
    while(!ABORT && GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    //some cleanup
    DestroyWindow(rawkbd_wnd);
    UnregisterClass("rawkbd_wndclass", GetModuleHandle(NULL));
    return 0;
}
