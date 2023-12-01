# Keebcap
This repository contains the code associated to this [article](https://www.synacktiv.com/publications/writing-a-decent-win32-keylogger) in which we talk about building a win32 keylogger that supports all languages not using input method editors.

All tests were done on Windows 10 (22h2).

# tree

```bash
.   # keylog code
├── cap_gks.c           # sample keylogger using GetKeyState()
├── cap_llh.c           # sample keylogger using SetWindowsHookEx(WH_KEYBOARD_LL)
├── cap_rid.c           # sample keylogger using GetRawInputData()
├── process.c           # retrieve key-strokes context
├── process.h
├── vk_names.h          # virtual key to string (name) macro
│
│   # processing code
├── dumper.cpp          # program to dump windows KBD*.DLL contents to json
├── kbdhdr.h            # slightly modified windows headers
├── reconstruct.py      # reconstructs character stream from keylogs
├── replayer.py         # win32 python program to replay scan codes and extract resulting characters
│
│   # various
├── kbdlmap.py          # KLID to dll name mapping
├── vk_names.py         # virtual key number to/from name 
├── Makefile            # build keylogger with mingw-w64
│
│   # external dependencies
├── extern              
│   └── json.hpp        # by Niels Lohmann from https://github.com/nlohmann/json
│
│   # json files
├── inputs              # input test cases        
└── layouts             # output of dumper on all win32 kbd*.dll
```

# Dependencies

```bash
$ sudo apt install mingw-w64 python3
```

# Usage

On windows, type either one of those commands, assuming the exe have been dropped on the desktop:

```cmd
c:\Users\user\Desktop> cap_gks.exe > keylog1.jsonl
c:\Users\user\Desktop> cap_llh.exe > keylog2.jsonl
c:\Users\user\Desktop> cap_rid.exe > keylog3.jsonl
```

To dump the contents of a DLL to json:
```cmd
c:\Users\user\Desktop> dmp.exe kbdfr > kbdfr.json
```

To reinject keystrokes:
```cmd
c:\Users\user\Desktop> python replayer.py keylog1.jsonl > keylog1_ref.txt
```

To reconstruct the text from keystrokes (and verify the results)
```bash
$ python3 reconstruct.py -v -l kbdfr.dll -c keylog1_ref.txt keylog1.jsonl
```

