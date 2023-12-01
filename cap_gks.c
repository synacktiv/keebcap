#include "process.h"
#include <stdio.h>
#include <signal.h>

#ifndef QWORD
typedef unsigned __int64 QWORD;
#endif

int ABORT = 0;
short kbs_last[256] = {};

void sig_handler(int signal)
{
    fprintf(stderr, "> exiting..\n");
    if(signal == SIGABRT || signal == SIGINT)
        ABORT = 1;
}

void get_kb_state(short kbs[256])
{
    for(int i=0; i<256; i++)
        kbs[i] = GetKeyState(i);
}

int main(int ac, char ** av)
{
    fprintf(stderr, "> starting..\n");

    signal(SIGINT, sig_handler);

    get_kb_state(kbs_last);

    while(!ABORT)
    {
        short kbs[256] = {};
        get_kb_state(kbs);

        for(int i=255; i>=0; i--)
            if(kbs[i] != kbs_last[i])
            {
                int vsc = MapVirtualKeyA(i, MAPVK_VK_TO_VSC_EX);
                int e0 = ((vsc >> 8) & 0xff) == 0xe0;
                int e1 = ((vsc >> 8) & 0xff) == 0xe1;
                int sc = vsc & 0xff;
                int u = (kbs[i] & 0xc0000000) == 0;

                // we ignore generic modifiers to just keep handed ones (eg. VK_RSHIFT, VK_LMENU, ...)
                int skip = (i == VK_SHIFT || i == VK_CONTROL || i == VK_MENU);
                if(!skip)
                    process_kbd_event(sc, e0, e1, u, i);
            }

        memcpy(kbs_last, kbs, sizeof(kbs_last));

        Sleep(5);
    }

    return 0;
}

