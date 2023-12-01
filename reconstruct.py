#!/usr/bin/env python3
import argparse
import os
import sys
import json
from vk_names import *
from kbdlmap import KLID_TO_LAYOUT

# options that can be overriden with argparse
DEBUG = False
FORCE_LAYOUT = None

# some useful defines from kbd.h
SGCAPS      = 0x0002
CAPLOK      = 0x0001
WCH_NONE    = 0xF000
WCH_DEAD    = 0xF001
WCH_LGTR    = 0xF002

# our internal keyboard state variable
state = {
    'vk':           [ 0 for i in range(0x100) ],    # 1 if the key is pressed, 0 if released
    'dead':         None,                           # unicode character in the deadchar buffer
    'capslock':     0,                              # is capslock toggled?
    'numlock':      1,                              # is numlock toggled?
    'scrolllock':   0,                              # useless
}

# utility functions
def jsonl_load(f):
    ret = []
    for line in f:
        ret.append(json.loads(line))
    return ret

def json_load(path):
    with open(path, 'r') as f:
        return json.load(f)

def debug(msg):
    if not DEBUG:
        return
    sys.stderr.write(msg)
    sys.stderr.flush()

def logmsg(msg):
    sys.stderr.write(msg)
    sys.stderr.flush()

def output(msg):
    sys.stdout.write(msg)
    sys.stdout.flush()

def load_layout(fpath):
    layout = json_load(fpath)
    debug('> loaded layout %s\n' % fpath)
    return layout


def sc_to_vk(sc, e0, e1, state, layout):
    '''
        converts input scan code to virtual key
    '''

    # check in vsc_to_vk map first
    for it in layout['vsc_to_vk']:
        if it['sc'] == sc:
            # skip this entry if it has the "extended flags" and neither E0 nor E1 are set
            if 'KBDEXT' in it['flags'] and not e0 and not e1:
                continue
            # E0 or E1 but no KBDEXT flag, skip that one too
            elif 'KBDEXT' not in it['flags'] and (e0 or e1):
                continue
            return it['vk'], it['vkn']

    # check extended scan codes
    if e0:
        for it in layout['vsc_to_vk_e0']:
            if it['sc'] == sc:
                return it['vk'], it['vkn']
    if e1:
        for it in layout['vsc_to_vk_e1']:
            if it['sc'] == sc:
                return it['vk'], it['vkn']

    return None, None

def fix_numpad_vk(vk, state):
    '''
        numpad keys are handled differently, the mapping towards VK_NUMPAD*
        is not present in the keyboard layout dlls, juste the one to VK_INSERT, ...
        so fix it manually :/
    '''
    if not state['numlock']:
        return vk
    fix_map = {
        VK_INSERT:    VK_NUMPAD0,
        VK_END:       VK_NUMPAD1,
        VK_DOWN:      VK_NUMPAD2,
        VK_NEXT:      VK_NUMPAD3,
        VK_LEFT:      VK_NUMPAD4,
        VK_CLEAR:     VK_NUMPAD5,
        VK_RIGHT:     VK_NUMPAD6,
        VK_HOME:      VK_NUMPAD7,
        VK_UP:        VK_NUMPAD8,
        VK_PRIOR:     VK_NUMPAD9,
    }
    if vk in fix_map:
        return fix_map[vk]
    return vk

def shiftstate_to_column(ss, layout):
    col = layout['shiftstates'][ss]
    debug('> shiftstate: %d => col %d\n' % (ss, col))
    if col == 15:
        return None
    return col

def vk_to_chars(vk, col, layout):
    '''
        The first item of the returned tuple is the output wchar_t value as an int.
        The second value is always none, except if the 1st value is WCH_DEAD,
            in which case the 2nd value is the dead character.
    '''
    # go through all the sub tables
    for vkmap in layout['vk_to_wchars']:
        # skip tables not containing enough columns (one or more shift states)
        if col >= vkmap['num_mods']:
            continue

        # go through all entries
        for i in range(len(vkmap['table'])):
            it = vkmap['table'][i]

            # does this entry match the current virtual key?
            if it['vk'] == vk:

                # here we handle a couple tricky cases

                # regular CAPSLOCK, code assumes VK_SHIFT is modifier with bit 1
                # (which is the case for all keymaps shipped in windows)
                if it['attrs'] == CAPLOK and not col and state['capslock']:
                    # capslock is engaged, the key has the CAPLOCK flag
                    # adjust the column as if VK_SHIFT was pressed
                    col = 1

                # skip SGCAPS entry if we have CAPSLOCK on and SHIFT
                # refer to §FIXME for more information
                if it['attrs'] == SGCAPS and state['capslock']:
                    continue

                # handle dead characters
                if it['wch'][col] == WCH_DEAD:
                    # also return the associated dead key
                    return it['wch'][col], vkmap['table'][i+1]['wch'][col]

                # regular case, we can just return the char value
                return it['wch'][col], None
    # nothing found
    return None, None

def vk_to_ligature(vk, modnum, layout):
    for lig in layout['ligatures']:
        if lig['vk'] == vk and modnum == lig['modnum']:
            return lig['chars']
    return None

def deadchar_find(deadchar, vk, layout):
    debug('deadchar : %c (%d) vk %d \n' % (chr(deadchar), deadchar, vk))
    for it in layout['deadkeys']:
        if it['vk1'] == deadchar and it['vk2'] == vk:
            return it['combined']
    return None

def output_char(ch, dead, vk, state, layout):
    out = []

    if ch is None or ch == WCH_NONE:
        return ''

    # previous key was a deadchar
    if state['dead'] is not None:
        comb = deadchar_find(state['dead'], ch if not dead else dead, layout)
        # chained dead chars?
        if comb and dead:
            debug('> chained dead chars!\n')
            state['dead'] = comb
            return ''
        if comb:
            debug('> good dead char!\n')
            out.append(comb)
        else:
            debug('> bad dead char combo!\n')
            out.append(state['dead'])
            out.append(dead)
        state['dead'] = None

    # current key is deadchar
    elif ch == WCH_DEAD:
        debug('> dead char pressed "%c"!\n' % chr(dead))
        state['dead'] = dead
        return ''

    # handle ligatures
    elif ch == WCH_LGTR:
        out = vk_to_ligature(vk, state['modnum'], layout)
        debug('> ligature "%s"!\n' % ', '.join([ chr(c) for c in out]))

    # current key is not a deadchar
    else:
        out.append(ch)

    debug('> outputing : %r\n' % [ chr(c) for c in out])
    out = ''.join([ chr(c) for c in out]).replace('\r', '\r\n') # windows end of lines
    output(out)
    return out


def reconstruct(events):
    klid = None
    layout  = None
    out = ''

    if FORCE_LAYOUT:
        layout = load_layout(layout_name)

    for evt in events:
        debug('> processing event: %r\n' % evt)

        # load current keyboard layout if not done yet or if it changed
        if not FORCE_LAYOUT and (not klid or evt['klid'].lower() != klid):
            klid = evt['klid'].lower()
            layout_name = KLID_TO_LAYOUT.get(klid)
            if not layout_name:
                logmsg('> unknown klid \'%s\'\n' % evt['klid'])
                sys.exit(1)

            layout_path = 'layouts/' + layout_name.lower().replace('.dll', '.json')
            layout = load_layout(layout_path)

        # convert scancode to virtual key
        vk, vkn = sc_to_vk(evt['sc'], evt['e0'], evt['e1'], state, layout)
        if not vk:
            debug('> ignoring sc: %d e0: %d e1: %d => no entry in map\n' % (evt['sc'], evt['e0'], evt['e1']))
            continue

        debug('> got vk %d vkn %s\n' % (vk, vkn))
        vkf = fix_numpad_vk(vk, state)
        if vk != vkf:
            vk = vkf
            vkn = VK_NAMES[vkf]
            debug('> fixed numpad vk: %d %s\n' % (vk, vkn))

        # handle *lock keys
        if vk == VK_CAPITAL and not evt['keyup']:
            state['capslock'] = 1 if not state['capslock'] else 0
        if vk == VK_NUMLOCK and not evt['keyup']:
            state['numlock'] = 1 if not state['numlock'] else 0
        if vk == VK_SCROLL and not evt['keyup']:
            state['scrolllock'] = 1 if not state['scrolllock'] else 0

        debug('> lock caps(%d) num(%d) scroll(%d)\n' % (state['capslock'], state['numlock'], state['scrolllock']))

        # is key a right/left handed modifier?
        if vk == VK_LSHIFT or vk == VK_RSHIFT:
            vk = VK_SHIFT
        if vk == VK_LCONTROL or vk == VK_RCONTROL:
            vk = VK_CONTROL
        if not layout['flag_altgr']:
            # if keyboard uses altgr, right alt = CTRL + ALT
            if vk == VK_LMENU or vk == VK_RMENU:
                vk = VK_MENU
        else:
            if vk == VK_LMENU:
                vk = VK_MENU
            elif vk == VK_RMENU:
                # altgr = shift + menu
                state['vk'][VK_CONTROL] = 1 if not evt['keyup'] else 0
                state['vk'][VK_MENU] = 1 if not evt['keyup'] else 0

        # is key a modifier?
        for mod in layout['modifiers']:
            if vk == mod['vk']:
                # yes, update state
                if evt['keyup']:
                    state['vk'][vk] = 0
                else:
                    state['vk'][vk] = 1
                break

        # calculate current shift state
        shiftstate = 0
        for mod in layout['modifiers']:
            if state['vk'][mod['vk']]:
                shiftstate |= mod['modbits']
        state['modnum'] = shiftstate

        debug('> current shift state: 0x%x\n' % shiftstate)
        col = shiftstate_to_column(shiftstate, layout)
        if col is None:
            debug('> bad shiftstate combination, skipping keypress\n')
            continue

        debug('> current shift col: %d\n' % col)

        if evt['keyup']:
            # sys.stderr.write('> key up, no chars\n')
            continue

        ch, dead = vk_to_chars(vk, col, layout)
        out += output_char(ch, dead, vk, state, layout)

    return out


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-l',
        '--layout',
        default=None,
        type=str,
        help='force specific keyboard layout and ignore context from input jsonl. ex value: "kbdfr.dll"',
    )
    parser.add_argument(
        '-c',
        '--check',
        default=None,
        type=argparse.FileType('r', encoding='utf-8'),
        help='path to file containing the valid utf8 encoded reference text',
    )
    parser.add_argument('-v',
        '--verbose',
        default=False,
        action='store_true',
        help='enable verbose output',
    )
    parser.add_argument(
        'files',
        nargs='+',
        type=argparse.FileType('r', encoding='utf-8'),
        help='input jsonl file(s)',
    )
    return parser.parse_args()


def main():
    args = parse_args()

    if args.verbose:
        global DEBUG
        DEBUG = True

    if args.layout:
        name = args.layout.lower()
        if not name.endswith('.dll'):
            name += '.dll'

        path = 'layouts/' + name.replace('.dll', '.json')
        if not os.path.exists(path):
            logmsg('> invalid layout name \'%s\', cant find layout file \'%s\'' % (args.layout, path))
            sys.exit(1)
        global FORCE_LAYOUT
        FORCE_LAYOUT = path
        logmsg('> forcing layout "%s"\n' % FORCE_LAYOUT)


    out = ''
    for f in args.files:
        logmsg('> processing file \'%s\'\n' % f.name)
        events = jsonl_load(f)
        out += reconstruct(events)
        f.close()

    if args.check:
        content = args.check.read()
        if out != content:
            logmsg('> reconstructed output differs from reference file: %s\n' % args.check.name)
            logmsg('> expected: "%s"\n' % content)
            logmsg('> result:   "%s"\n' % out)
            sys.exit(1)
        else:
            logmsg('> reconstructed output matches reference file: %s\n' % args.check.name)
        args.check.close()

    return


if __name__ == '__main__':
    main()
