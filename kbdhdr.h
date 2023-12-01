#ifndef KBDHDR_H
#define KBDHDR_H

#include <wchar.h>

#define CAPLOK 0x01
#define WCH_NONE 0xF000
#define WCH_DEAD 0xF001
#define WCH_LGTR 0xF002

typedef struct {
    BYTE Vk;
    BYTE ModBits;
} VK_TO_BIT, *PVK_TO_BIT;

#pragma warning (disable: 4200)
typedef struct {
    PVK_TO_BIT pVkToBit;
    WORD       wMaxModBits;
    BYTE       ModNumber[];
} MODIFIERS, *PMODIFIERS;

typedef struct _VSC_VK {
    BYTE Vsc;
    USHORT Vk;
} VSC_VK, *PVSC_VK;

typedef struct _VK_VSC {
    BYTE Vk;
    BYTE Vsc;
} VK_VSC, *PVK_VSC;

#define TYPEDEF_VK_TO_WCHARS(n) typedef struct _VK_TO_WCHARS##n { \
    BYTE  VirtualKey; \
    BYTE  Attributes; \
    WCHAR wch[n]; \
} VK_TO_WCHARS##n, *PVK_TO_WCHARS##n;

TYPEDEF_VK_TO_WCHARS(1)
TYPEDEF_VK_TO_WCHARS(2)
TYPEDEF_VK_TO_WCHARS(3)
TYPEDEF_VK_TO_WCHARS(4)
TYPEDEF_VK_TO_WCHARS(5)
TYPEDEF_VK_TO_WCHARS(6)
TYPEDEF_VK_TO_WCHARS(7)
TYPEDEF_VK_TO_WCHARS(8)
TYPEDEF_VK_TO_WCHARS(9)
TYPEDEF_VK_TO_WCHARS(10)

typedef struct _VK_TO_WCHAR_TABLE {
    PVK_TO_WCHARS1 pVkToWchars;
    BYTE           nModifications;
    BYTE           cbSize;
} VK_TO_WCHAR_TABLE, *PVK_TO_WCHAR_TABLE;

typedef struct {
    DWORD  dwBoth;
    WCHAR  wchComposed;
    USHORT uFlags;
} DEADKEY, *PDEADKEY;

#define TYPEDEF_LIGATURE(n) typedef struct _LIGATURE##n { \
    BYTE  VirtualKey; \
    WORD  ModificationNumber; \
    WCHAR wch[n]; \
} LIGATURE##n, *PLIGATURE##n;

TYPEDEF_LIGATURE(1)
TYPEDEF_LIGATURE(2)
TYPEDEF_LIGATURE(3)
TYPEDEF_LIGATURE(4)
TYPEDEF_LIGATURE(5)

typedef struct {
    BYTE   vsc;
    WCHAR * pwsz;
} VSC_LPWSTR, *PVSC_LPWSTR;

typedef WCHAR * DEADKEY_LPWSTR;

typedef struct tagKbdLayer {
    PMODIFIERS pCharModifiers;
    PVK_TO_WCHAR_TABLE pVkToWcharTable;
    PDEADKEY pDeadKey;
    PVSC_LPWSTR pKeyNames;
    PVSC_LPWSTR pKeyNamesExt;
    WCHAR * * pKeyNamesDead;
    USHORT  * pusVSCtoVK;
    BYTE    bMaxVSCtoVK;
    PVSC_VK pVSCtoVK_E0;
    PVSC_VK pVSCtoVK_E1;
    DWORD fLocaleFlags;
    BYTE       nLgMax;
    BYTE       cbLgEntry;
    PLIGATURE1 pLigature;
    DWORD      dwType;
    DWORD      dwSubType;
} KBDTABLES, *PKBDTABLES;

// locale flags
#define KLLF_ALTGR      1
#define KLLF_SHIFTLOCK  2
#define KLLF_LRM_RLM    4

// VK flags
#define KBDEXT        (USHORT)0x0100
#define KBDMULTIVK    (USHORT)0x0200
#define KBDSPECIAL    (USHORT)0x0400
#define KBDNUMPAD     (USHORT)0x0800
#define KBDUNICODE    (USHORT)0x1000
#define KBDINJECTEDVK (USHORT)0x2000
#define KBDMAPPEDVK   (USHORT)0x4000
#define KBDBREAK      (USHORT)0x8000


typedef struct _VK_FUNCTION_PARAM {
    BYTE  NLSFEProcIndex;
    ULONG NLSFEProcParam;
} VK_FPARAM, *PVK_FPARAM;

typedef struct _VK_TO_FUNCTION_TABLE {
    BYTE Vk;
    BYTE NLSFEProcType;
    BYTE NLSFEProcCurrent;
    BYTE NLSFEProcSwitch;
    VK_FPARAM NLSFEProc[8];
    VK_FPARAM NLSFEProcAlt[8];
} VK_F, *PVK_F;

typedef struct tagKbdNlsLayer {
    USHORT OEMIdentifier;
    USHORT LayoutInformation;
    UINT  NumOfVkToF;
    PVK_F pVkToF;
    INT     NumOfMouseVKey;
    USHORT * pusMouseVKey;
} KBDNLSTABLES, *PKBDNLSTABLES;


#endif
