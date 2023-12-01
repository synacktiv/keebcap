#include "process.h"
#include "vk_names.h"

#include <tlhelp32.h>
#include <stdio.h>


void process_kbd_event(int vsc, int e0, int e1, int keyup, int vk)
{
    klg_ctx_t ctx = get_context();

    // fprintf(stdout, "{ \"time\": %d, \"process\": \"%s\", \"pid\": %d, \"layout\": \"%s\", \"klid\": \"%s\", \"hkl\": \"%.8x\", \"keyup\": %d, \"sc\": %d, \"e0\": %d, \"e1\": %d, \"vk\": %d, \"vkn\": \"%s\" }\n",
    fprintf(stdout, "{ \"time\": %d, \"klid\": \"%s\", \"keyup\": %d, \"sc\": %d, \"e0\": %d, \"e1\": %d, \"vk\": %d, \"vkn\": \"%s\" }\n",
        ctx.time,
        // ctx.procname,
        // ctx.pid,
        // ctx.layout,
        // *(UINT*)&ctx.hkl,
        ctx.klid,
        // *(UINT*)&ctx.hkl,
        keyup?1:0,
        vsc,
        e0?1:0,
        e1?1:0,
        vk,
        VKN(vk)
    );

}

// gets the process name (.exe) for input *pid*
// outputs at most *out_len* bytes in *out_buf*
// returns 0 on success, -1 on failure
int get_proc_name(DWORD pid, char * out_buf, size_t out_len)
{
    PROCESSENTRY32 pe32 = {};
    HANDLE snap = NULL;
    int ret = -1;

    if(!(snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)))
        goto fail;

    pe32.dwSize = sizeof(PROCESSENTRY32);

    if(!Process32First(snap, &pe32))
        goto fail;

    do
    {
        if(pe32.th32ProcessID == pid)
        {
            strncpy(out_buf, pe32.szExeFile, out_len);
            ret = 0;
            break;
        }
    }
    while(Process32Next(snap, &pe32));

fail:
    if(snap)
        CloseHandle(snap);
    return ret;
}


// https://stackoverflow.com/a/72381670
int hkl_to_klid(HKL hkl, char out_klid[KL_NAMELENGTH])
{
    memset(out_klid, 0, KL_NAMELENGTH);

    WORD device = HIWORD(hkl);

    if((device & 0xf000) == 0xf000) // `Device Handle` contains `Layout ID`
    {
        WORD layoutId = device & 0x0fff;

        HKEY key;
        if(RegOpenKeyA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts", &key) != ERROR_SUCCESS)
            return -1;

        DWORD index = 0;
        char buffer[KL_NAMELENGTH];
        DWORD len = sizeof(buffer);

        while(RegEnumKeyExA(key, index, buffer, &len, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
        {
            char layoutIdBuffer[MAX_PATH] = {};
            DWORD layoutIdBufferSize = sizeof(layoutIdBuffer);
            if(RegGetValueA(key, buffer, "Layout Id", RRF_RT_REG_SZ, NULL, layoutIdBuffer, &layoutIdBufferSize) == ERROR_SUCCESS)
            {
                if(layoutId == strtoul(layoutIdBuffer, NULL, 16))
                {
                    _strupr(buffer);
                    strncpy(out_klid, buffer, KL_NAMELENGTH);
                    return 0;
                }
            }
            len = (DWORD)sizeof(buffer);
            ++index;
        }

        RegCloseKey(key);
    }
    else
    {
        // Use input language only if keyboard layout language is not available. This
        // is crucial in cases when keyboard is installed more than once or under
        // different languages. For example when French keyboard is installed under US
        // input language we need to return French keyboard identifier.
        if (device == 0)
        {
            device = LOWORD(hkl);
        }

        snprintf(out_klid, KL_NAMELENGTH, "%08X", device);
        return 0;
    }

    return -1;
}


// returns the current context
// ie, information on the process with the foreground window
klg_ctx_t get_context(void)
{
    static klg_ctx_t ctx = {};
    static DWORD time_first = 0;
    HANDLE th;

    if(!time_first)
        time_first = GetTickCount();

    ctx.time = GetTickCount() - time_first;

    // get foreground window (ie, the one receiving keyboard input)
    ctx.hwnd = GetForegroundWindow();
    // get associated thread id
    DWORD thid = GetWindowThreadProcessId(ctx.hwnd, NULL);
    // request querry information permission on this thread
    th = OpenThread(THREAD_QUERY_INFORMATION, FALSE, thid);
    // get associated pid
    ctx.pid = GetProcessIdOfThread(th);

    // get process name from pid
    get_proc_name(ctx.pid, ctx.procname, sizeof(ctx.procname));

    // windows will return hkl=0 for cmd.exe subprocesses :/
    // so we default to the current keyboard layout in that case
    if(!strcmp(ctx.procname, "cmd.exe"))
        thid = 0;

    // get keyboard layout for this thread
    ctx.hkl = GetKeyboardLayout(thid);
    CloseHandle(th);

    // convert hkl to klid
    if(hkl_to_klid(ctx.hkl, ctx.klid) < 0)
        fprintf(stderr, "bad layout :/\n");

    // all good
    return ctx;
}

