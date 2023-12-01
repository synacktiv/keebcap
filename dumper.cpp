#define _WIN32_WINNT 0x0600
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "extern/json.hpp"
using json = nlohmann::json;

#include "kbdhdr.h"
#include "vk_names.h"

#include <stdio.h>

// returns a list of flags for a specified virtual key
json vkftos(int vk)
{
    json o = json::array();

    if(vk & KBDEXT)         o.push_back("KBDEXT");
    if(vk & KBDMULTIVK)     o.push_back("KBDMULTIVK");
    if(vk & KBDSPECIAL)     o.push_back("KBDSPECIAL");
    if(vk & KBDNUMPAD)      o.push_back("KBDNUMPAD");
    if(vk & KBDUNICODE)     o.push_back("KBDUNICODE");
    if(vk & KBDINJECTEDVK)  o.push_back("KBDINJECTEDVK");
    if(vk & KBDMAPPEDVK)    o.push_back("KBDMAPPEDVK");
    if(vk & KBDBREAK)       o.push_back("KBDBREAK");

    return o;
}

// function that does most of the work of parsing the kbd descriptor table and returns a json object
int parse_kbdlayout(const char * keymap, json & j)
{
    // load dll & make sure everything is fine
    HMODULE hmod = LoadLibraryA(keymap);
    if(!hmod)
    {
        fprintf(stderr, "[-] could not load '%s.dll'.\n", keymap);
        return -1;
    }

    FARPROC func = GetProcAddress(hmod, "KbdLayerDescriptor");
    if(!func)
    {
        fprintf(stderr, "[-] cant find KbdLayerDescriptor()\n", keymap);
        return 1;
    }
    PKBDTABLES kbd = ((PKBDTABLES(*)())func)();

    // dll is loaded, start parsing
    j = json({});
    j["kbd_name"]       = keymap;
    j["kbd_type"]       = kbd->dwType;
    j["kbd_subtype"]    = kbd->dwSubType;
    j["flag_altgr"]     = kbd->fLocaleFlags & KLLF_ALTGR       ? 1 : 0;
    j["flag_shiftlock"] = kbd->fLocaleFlags & KLLF_SHIFTLOCK   ? 1 : 0;
    j["flag_ltr"]       = kbd->fLocaleFlags & KLLF_LRM_RLM     ? 1 : 0;

    // shift states & modifiers
    j["shiftstates"] = json::array();
    for(int i=0; i<=kbd->pCharModifiers->wMaxModBits; i++)
        j["shiftstates"].push_back(kbd->pCharModifiers->ModNumber[i]);

    j["modifiers"] = json::array();
    for(int i=0; kbd->pCharModifiers->pVkToBit[i].Vk; i++)
    {
        json o = json();
        o["modbits"] = kbd->pCharModifiers->pVkToBit[i].ModBits;
        o["vk"] = kbd->pCharModifiers->pVkToBit[i].Vk;
        o["vkn"] = VKN(kbd->pCharModifiers->pVkToBit[i].Vk);
        j["modifiers"].push_back(o);
    }

    // vk to chars
    j["vk_to_wchars"] = json::array();
    for(int i=0; kbd->pVkToWcharTable[i].cbSize; i++)
    {
        json o = json();
        o["index"] = i+1;
        o["num_mods"] = kbd->pVkToWcharTable[i].nModifications;
        o["table"] = json::array();

        PVK_TO_WCHARS10 pvk2wch = (PVK_TO_WCHARS10)kbd->pVkToWcharTable[i].pVkToWchars;
        while(pvk2wch->VirtualKey)
        {
            json it = json();
            it["vk"] = pvk2wch->VirtualKey;
            it["vkn"] = VKN(pvk2wch->VirtualKey);
            it["attrs"] = pvk2wch->Attributes;
            it["wch"] = json::array();

            for(int j=0; j<kbd->pVkToWcharTable[i].nModifications; j++)
                it["wch"].push_back(pvk2wch->wch[j]);

            pvk2wch = (PVK_TO_WCHARS10)((char*)pvk2wch + kbd->pVkToWcharTable[i].cbSize);
            o["table"].push_back(it);
        }

        j["vk_to_wchars"].push_back(o);
    }

    // scan codes to virtual keys
    j["vsc_to_vk"] = json::array();
    USHORT * vvk = kbd->pusVSCtoVK;
    if(vvk)
    {
        for(int i=0; i<kbd->bMaxVSCtoVK; i++)
        {
            json o = json();
            o["sc"] = i;
            o["vk"] = vvk[i] & 0xff;
            o["vkn"] = VKN(vvk[i] & 0xff);
            o["flags"] = vkftos(vvk[i]);
            j["vsc_to_vk"].push_back(o);
        }
    }

    // scan codes to virtual keys extended
    j["vsc_to_vk_e0"] = json::array();
    PVSC_VK vv0 = kbd->pVSCtoVK_E0;
    for(int i=0; vv0 && vv0[i].Vsc; i++)
    {
        json o = json();
        o["sc"] = vv0[i].Vsc;
        o["vk"] = vv0[i].Vk & 0xff;
        o["vkn"] = VKN(vv0[i].Vk & 0xff);
        o["flags"] = vkftos(vv0[i].Vk);
        j["vsc_to_vk_e0"].push_back(o);
    }

    // scan codes to virtual keys extended
    j["vsc_to_vk_e1"] = json::array();
    PVSC_VK vv1 = kbd->pVSCtoVK_E1;
    for(int i=0; vv1 && vv1[i].Vsc; i++)
    {
        json o = json();
        o["sc"] = vv1[i].Vsc;
        o["vk"] = vv1[i].Vk & 0xff;
        o["vkn"] = VKN(vv1[i].Vk & 0xff);
        o["flags"] = vkftos(vv1[i].Vk);
        j["vsc_to_vk_e0"].push_back(o);
    }

    // dead keys
    j["deadkeys"] = json::array();
    PDEADKEY pd = kbd->pDeadKey;
    for(int i=0; pd && pd[i].dwBoth != 0; i++)
    {
        json o = json();
        o["vk1"] = pd[i].dwBoth >> 16;
        o["vk2"] = pd[i].dwBoth & 0xffff;
        o["combined"] = pd[i].wchComposed;
        o["flags"] = pd[i].uFlags;
        j["deadkeys"].push_back(o);
    }

    // key names
    j["key_names"] = json::array();
    PVSC_LPWSTR kn = kbd->pKeyNames;
    for(int i=0; kn[i].vsc != 0; i++)
    {
        json o = json();
        o["sc"] = kn[i].vsc;
        o["name"] = kn[i].pwsz;
        j["key_names"].push_back(o);
    }

    // key names extended
    j["key_names_ext"] = json::array();
    PVSC_LPWSTR kne = kbd->pKeyNamesExt;
    for(int i=0; kne[i].vsc != 0; i++)
    {
        json o = json();
        o["sc"] = kne[i].vsc;
        o["name"] = kne[i].pwsz;
        j["key_names_ext"].push_back(o);
    }

    // key names for dead keys
    j["key_names_dead"] = json::array();
    WCHAR ** knd = kbd->pKeyNamesDead;
    for(int i=0; knd && knd[i] != 0; i++)
    {
        json o = json();
        o["wch"] = knd[i][0];
        o["name"] = knd[i] + 1;
        j["key_names_dead"].push_back(o);
    }

    // ligatures
    j["ligatures"] = json::array();
    PLIGATURE5 lg = (PLIGATURE5)((BYTE*)kbd->pLigature);
    for(int i=0; lg && lg->VirtualKey; i++, lg = (PLIGATURE5)((BYTE*)kbd->pLigature + i*kbd->cbLgEntry))
    {
        json o = json();
        o["vk"] = lg->VirtualKey;
        o["modnum"] = lg->ModificationNumber;
        o["chars"] = json::array();
        for(int k=0; k<kbd->nLgMax; k++)
            o["chars"].push_back(lg->wch[k]);
        j["ligatures"].push_back(o);
    }

    // all good
    return 0;
}

// main function, loads the specified keyboard mapping dll
// and outputs JSON on stdout
int main(int ac, char ** av)
{
    json j;

    if(parse_kbdlayout(ac > 1 ? av[1] : "kbdfr", j) < 0)
        return -1;

    fprintf(stdout, "%s\n", j.dump(4).c_str());
    return 0;
}
