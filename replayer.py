import sys
import ctypes
from ctypes import *
from ctypes import wintypes as w
import time
import threading
import json

# ============================================================================ #
# sendinput interface
# ============================================================================ #

# required flags / defines
KEYEVENTF_EXTENDEDKEY = 0x1
KEYEVENTF_KEYUP = 0x2
KEYEVENTF_UNICODE = 0x4
KEYEVENTF_SCANCODE = 0x8
INPUT_KEYBOARD = 1

# not defined by wintypes
ULONG_PTR = c_ulong if sizeof(c_void_p) == 4 else c_ulonglong

class KEYBDINPUT(Structure):
    _fields_ = [('wVk' ,w.WORD),
                ('wScan',w.WORD),
                ('dwFlags',w.DWORD),
                ('time',w.DWORD),
                ('dwExtraInfo',ULONG_PTR)]

class MOUSEINPUT(Structure):
    _fields_ = [('dx' ,w.LONG),
                ('dy',w.LONG),
                ('mouseData',w.DWORD),
                ('dwFlags',w.DWORD),
                ('time',w.DWORD),
                ('dwExtraInfo',ULONG_PTR)]

class HARDWAREINPUT(Structure):
    _fields_ = [('uMsg' ,w.DWORD),
                ('wParamL',w.WORD),
                ('wParamH',w.WORD)]

class DUMMYUNIONNAME(Union):
    _fields_ = [('mi',MOUSEINPUT),
                ('ki',KEYBDINPUT),
                ('hi',HARDWAREINPUT)] 

class INPUT(Structure):
    _anonymous_ = ['u']
    _fields_ = [('type',w.DWORD),
                ('u',DUMMYUNIONNAME)]

user32 = WinDLL('user32')
user32.SendInput.argtypes = w.UINT, POINTER(INPUT), c_int
user32.SendInput.restype = w.UINT

def send_scancode(code, up, ext):
    ''' uses SendInput to send a specified scancode, setting the appropriate flags for key up/down and e0/e1 extended flags '''
    i = INPUT()
    i.type = INPUT_KEYBOARD
    i.ki = KEYBDINPUT(0, code, KEYEVENTF_SCANCODE, 0, 0)
    if up:
        i.ki.dwFlags |= KEYEVENTF_KEYUP
    if ext:
        i.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY

    return user32.SendInput(1, byref(i), sizeof(INPUT)) == 1


# ============================================================================ #
# switch keyboard layout
# ============================================================================ #

# defines
WM_INPUTLANGCHANGEREQUEST = 0x0050

# prototypes
user32.GetForegroundWindow.restype = POINTER(w.HWND)
user32.SendMessageA.argtypes = POINTER(w.HWND),w.UINT,w.WPARAM,w.LPARAM
user32.SendMessageA.restype = c_int

def switch_layout(klid):
    '''
        uses SendMessageA to the current foreground window instead of
        ActivateKeyboardLayout / LoadkeyboardLayout which will not work
        if not called from the main thread of the current program,
        which is not the case with python ...
    '''
    return user32.SendMessageA( user32.GetForegroundWindow(), WM_INPUTLANGCHANGEREQUEST, 0, int(klid, 16)) == 0


# ============================================================================ #
# main
# ============================================================================ #

def main():

    # argument checking
    if len(sys.argv) < 2:
        print('usage: %s <input_file.jsonl>' % sys.argv[0])
        exit(0)

    # parse jsonl input
    events = []
    with open(sys.argv[1], 'r') as f:
        for line in f:
            events.append(json.loads(line))
    if not len(events):
        sys.stderr.write('[-] invalid file contents\n')
        exit(1)

    # we start a thread that reads stdin in blocking mode
    def input_thread(args):
        input_str = sys.stdin.read()
        sys.stderr.write('> out:\n')
        sys.stderr.flush()
        sys.stdout.write(input_str)
        sys.stdout.flush()

    th = threading.Thread(target=input_thread, args=(1,))
    th.start()

    # main loop
    last_time = 0
    lklid = ''

    for evt in events:
        # sync everything from the 1st input timing
        if not last_time:
            last_time = evt['time']

        # wait the required amount of time before sending next keypress
        time.sleep(0.001 * (evt['time'] - last_time))
        last_time = evt['time']

        # different keyboard layout? if so, activate it
        if lklid != evt['klid']:
            if switch_layout(evt['klid']):
                sys.stderr.write("switched to %s\n" % evt['klid'])
                sys.stderr.flush()
                lklid = evt['klid']
            else:
                sys.stderr.write("failed to switch to %s\n" % evt['klid'])
                sys.stderr.flush()

        # now send the input
        send_scancode(evt['sc'], evt['keyup'], evt['e0'] | evt['e1'])

    time.sleep(.1)
    print('> closing...')
    sys.stdin.close()
    th.join()


if __name__ == '__main__':
    main()
