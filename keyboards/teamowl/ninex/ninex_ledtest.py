#!/usr/bin/env python3
"""Light each ninex LED in turn and report which key the user presses.

Uses the VialRGB direct effect to drive one LED at a time, and the ninex
keymap's raw-HID slot events to read back which physical key was pressed.
Verifies that rgb_matrix.layout and matrix_pins agree with the hardware.
"""
import os, select, sys, time

VID, PID = 0x1209, 0x4650
RAW_USAGE = bytes([0x06, 0x60, 0xFF, 0x09, 0x61])   # usage page 0xFF60, usage 0x61

# VIA / VialRGB
ID_LIGHTING_SET, ID_LIGHTING_GET = 0x07, 0x08
VIALRGB_SET_MODE, VIALRGB_DIRECT_FASTSET = 0x41, 0x42
VIALRGB_GET_MODE, VIALRGB_GET_NUMBER_LEDS = 0x41, 0x43
EFFECT_DIRECT = 1
# ninex keymap raw-HID
RH_APP_CMD, RH_SET_LAYER = 0xC0, 0x01
RH_SLOT_PRESS = 0x03
LAYER_BASE, LAYER_RAW = 0, 1

# LED chain index -> expected matrix [row, col], from keyboard.json
EXPECTED = [(0,0), (0,1), (0,2), (0,3),
            (1,3), (1,2), (1,1), (1,0),
            (2,0), (2,1), (2,2), (2,3)]
COLS = 4
NAMES = {0: "top-left",    1: "top #2",    2: "top #3",    3: "top-right",
         4: "middle-left", 5: "middle #2", 6: "middle #3", 7: "middle-right",
         8: "bottom-left", 9: "bottom #2", 10: "bottom #3", 11: "bottom-right"}


def find_raw_node():
    for name in sorted(os.listdir('/sys/class/hidraw')):
        base = f'/sys/class/hidraw/{name}/device'
        try:
            uevent = open(f'{base}/uevent').read()
            desc = open(f'{base}/report_descriptor', 'rb').read()
        except OSError:
            continue
        if f'{VID:08X}:{PID:08X}' not in uevent.upper():
            continue
        if desc.startswith(RAW_USAGE):
            return f'/dev/{name}'
    return None


class Pad:
    def __init__(self, path):
        self.fd = os.open(path, os.O_RDWR | os.O_NONBLOCK)

    def send(self, payload):
        buf = bytes(payload) + bytes(32 - len(payload))
        os.write(self.fd, b'\x00' + buf)       # leading 0 = no report id

    def read(self, timeout):
        end = time.monotonic() + timeout
        while True:
            left = end - time.monotonic()
            if left <= 0:
                return None
            if select.select([self.fd], [], [], left)[0]:
                try:
                    return os.read(self.fd, 32)
                except BlockingIOError:
                    continue

    def drain(self):
        while self.read(0.05) is not None:
            pass

    def request(self, payload, timeout=1.0):
        self.send(payload)
        end = time.monotonic() + timeout
        while time.monotonic() < end:
            r = self.read(end - time.monotonic())
            if r and r[0] == payload[0]:
                return r
        return None

    def set_leds(self, colors):
        """colors: list of (h, s, v), one per LED, sent 9 at a time."""
        for start in range(0, len(colors), 9):
            chunk = colors[start:start + 9]
            args = [start & 0xFF, start >> 8, len(chunk)]
            for hsv in chunk:
                args += list(hsv)
            self.send([ID_LIGHTING_SET, VIALRGB_DIRECT_FASTSET] + args)


def main():
    path = find_raw_node()
    if not path:
        sys.exit('no ninex raw-HID interface found (usage page 0xFF60)')
    try:
        pad = Pad(path)
    except PermissionError:
        sys.exit(f'{path}: permission denied - needs chmod 666 or a udev rule')
    print(f'using {path}', flush=True)

    n = pad.request([ID_LIGHTING_GET, VIALRGB_GET_NUMBER_LEDS])
    count = (n[2] | (n[3] << 8)) if n else 0
    print(f'device reports {count} LEDs', flush=True)
    if count != len(EXPECTED):
        sys.exit(f'expected {len(EXPECTED)} LEDs, got {count}')

    saved = pad.request([ID_LIGHTING_GET, VIALRGB_GET_MODE])
    saved_args = list(saved[2:8]) if saved else [2, 0, 64, 0, 255, 160]

    # raw layer: key presses report as slot events instead of typing digits
    pad.send([RH_APP_CMD, RH_SET_LAYER, LAYER_RAW])
    pad.send([ID_LIGHTING_SET, VIALRGB_SET_MODE, EFFECT_DIRECT, 0, 0, 0, 0, 0])
    time.sleep(0.2)
    pad.drain()

    off, lit = (0, 0, 0), (85, 255, 160)     # green
    results = []
    for i in range(count):
        pad.set_leds([lit if j == i else off for j in range(count)])
        row, col = EXPECTED[i]
        print(f'\nLED {i:2d} lit  (expect {NAMES[i]}, matrix [{row},{col}], '
              f'slot {row * COLS + col}) - press the lit key', flush=True)
        slot = None
        deadline = time.monotonic() + 30
        while time.monotonic() < deadline:
            r = pad.read(deadline - time.monotonic())
            if r and r[0] == RH_SLOT_PRESS and r[2] == 1:
                slot = r[1]
                break
        want = row * COLS + col
        if slot is None:
            print('  TIMEOUT - no key press seen', flush=True)
        elif slot == want:
            print(f'  OK    pressed slot {slot}', flush=True)
        else:
            gr, gc = divmod(slot, COLS)
            print(f'  WRONG pressed slot {slot} = matrix [{gr},{gc}] '
                  f'({NAMES.get(slot, "?")}), expected slot {want}', flush=True)
        results.append((i, want, slot))

    # restore
    pad.send([ID_LIGHTING_SET, VIALRGB_SET_MODE] + saved_args)
    pad.send([RH_APP_CMD, RH_SET_LAYER, LAYER_BASE])

    print('\n=== summary ===', flush=True)
    bad = [r for r in results if r[2] != r[1]]
    for i, want, got in results:
        mark = 'ok  ' if want == got else 'FAIL'
        print(f'{mark} LED {i:2d} -> expected slot {want}, got {got}', flush=True)
    print(f'\n{len(results) - len(bad)}/{len(results)} correct', flush=True)


if __name__ == '__main__':
    main()
