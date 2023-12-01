#ifndef _PROCESS_H
#define _PROCESS_H

#define _WIN32_WINNT 0x0600
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef struct
{
    DWORD       time;
    DWORD       pid;
    HWND        hwnd;
    HKL         hkl;
    char        procname[16];
    char        klid[KL_NAMELENGTH];
} klg_ctx_t;

klg_ctx_t get_context(void);

void process_kbd_event(int vsc, int e0, int e1, int keyup, int vk);

#endif
