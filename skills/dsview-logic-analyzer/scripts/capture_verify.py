# DSLogic Capture Verification Script
#
# Quick check: does a DSLogic capture contain real signal data, noise, or a test pattern?
# Usage: python3 capture_verify.py /tmp/cap.bin [channel=0] [max_bytes=200000]
#
# This script answers 3 questions:
# 1. Is the capture alive? (multiple distinct byte values)
# 2. Is it a real signal or a clock/test pattern? (pulse width regularity)
# 3. Does D0 specifically have edges? (pin-level check)

import sys
from collections import Counter

def verify_capture(path, channel=0, max_bytes=200000):
    data = open(path, 'rb').read(max_bytes)
    total = len(data)

    # 1. Byte-level diversity check
    counter = Counter(data)
    unique = len(counter)
    all_ff = counter.get(0xFF, 0)
    all_00 = counter.get(0x00, 0)

    print(f"=== DSLogic Capture Verification ===")
    print(f"File: {path}")
    print(f"Analyzed: {total} bytes")
    print(f"\n--- Byte Diversity ---")
    print(f"Unique byte values: {unique}")
    print(f"0xFF count: {all_ff} ({100*all_ff/total:.1f}%)")
    print(f"0x00 count: {all_00} ({100*all_00/total:.1f}%)")
    top = counter.most_common(5)
    print(f"Top values: {[(f'0x{v:02X}', c) for v, c in top]}")

    if unique == 1:
        val = top[0][0]
        print(f"\n❌ CAPTURE DEAD — every byte is 0x{val:02X}")
        if val == 0xFF:
            print("   Likely: vth too high, probe disconnected, or CPU halted")
            print("   Check: vth=1.8, enable only D0, verify CPU running (go)")
        elif val == 0x00:
            print("   Likely: probe shorted to GND, or all channels reading LOW")
        return

    if unique <= 3 and all_ff + all_00 > 0.9 * total:
        print(f"\n⚠️  Near-binary — only 0xFF/0x00 (test pattern or 1-bit signal)")
    else:
        print(f"\n✅ Multi-value signal — capture is alive")

    # 2. Edge analysis on D0
    bits = [(b >> channel) & 1 for b in data]
    edges = []
    for i in range(1, len(bits)):
        if bits[i] != bits[i-1]:
            edges.append(i)

    if not edges:
        print(f"\n❌ D{channel}: NO EDGES — pin is static {'HIGH' if bits[0] else 'LOW'}")
        return

    print(f"\n--- D{channel} Edge Analysis ---")
    print(f"Total edges: {len(edges)}")
    print(f"Idle state: {'HIGH' if bits[0] else 'LOW'}")

    pws = [edges[i] - edges[i-1] for i in range(1, min(len(edges), 50))]
    unique_pws = set(pws)
    print(f"First 30 pulse widths (samples): {pws[:30]}")
    print(f"Unique pulse widths: {len(unique_pws)}")

    if len(unique_pws) <= 2:
        print(f"\n❌ Regular pattern → CLOCK / SQUARE WAVE, not data")
        if len(unique_pws) == 2:
            a, b = sorted(unique_pws)
            print(f"   Pattern: {a}/{b} samples (duty cycle: {100*a/(a+b):.0f}/{100*b/(a+b):.0f})")
    elif len(unique_pws) >= 15:
        print(f"\n✅ Irregular pattern → likely REAL DATA (UART, SPI, etc.)")
    else:
        print(f"\n⚠️  Semi-regular — could be bit-bang UART or simple protocol")

    # 3. Byte-level pattern detection
    print(f"\n--- Byte Pattern ---")
    print(f"First 48 bytes (hex):")
    for i in range(0, min(48, len(data)), 16):
        hex_str = ' '.join(f'{b:02X}' for b in data[i:i+16])
        print(f"  {i:4d}: {hex_str}")

if __name__ == '__main__':
    path = sys.argv[1] if len(sys.argv) > 1 else '/tmp/cap.bin'
    ch = int(sys.argv[2]) if len(sys.argv) > 2 else 0
    mb = int(sys.argv[3]) if len(sys.argv) > 3 else 200000
    verify_capture(path, ch, mb)
