#include "process.h"
#include <stdio.h>
#include <signal.h>

#ifndef QWORD
typedef unsigned __int64 QWORD;
#endif

int ABORT = 0;

void sig_handler(int signal)
{
    fprintf(stderr, "> exiting..\n");
    if(signal == SIGABRT || signal == SIGINT)
        ABORT = 1;
}

LRESULT CALLBACK LowLevelKeyboardProc(int code, WPARAM wparm, LPARAM lparm)
{
    PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)lparm;
    // fprintf(stderr, "code(%d) wparm(%d) lparm(%p) vsc(0x%x) vk(0x%x)\n", code, wparm, lparm, p->scanCode, p->vkCode);
    process_kbd_event(p->scanCode,
        p->flags & LLKHF_EXTENDED,
        0,
        p->flags & LLKHF_UP,
        p->vkCode
    );

    return CallNextHookEx(NULL, code, wparm, lparm);
}

int main(int ac, char ** av)
{
    fprintf(stderr, "> starting..\n");

    HHOOK hhkLowLevelKybd = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, NULL, 0);

    signal(SIGINT, sig_handler);

    MSG msg;
    while(!ABORT && GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hhkLowLevelKybd);
    return 0;
}
