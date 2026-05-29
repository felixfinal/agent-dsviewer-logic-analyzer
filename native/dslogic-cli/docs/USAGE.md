# Usage Guide

v3.2.0 — DSLogic logic analyzer native command-line interface.

## Quick Start

```bash
# 1. Scan for devices (no session needed)
dslogic-cli scan
# → {"devices":[{"index":1,"name":"DSLogic Plus",...}],"count":2}

# 2. 100MHz 3ch stream, auto-max duration
echo '
open 1
stream 1
samplerate 100000000
channel_mode 3
capture 0 /tmp/cap.bin 15
' | dslogic-cli run

# 3. Specified duration (auto-clamped to device max)
echo '
open 1
stream 1
samplerate 50000000
channel_mode 2
capture 0.5 /tmp/50m_500ms.bin 10
' | dslogic-cli run
```

**Output convention**: JSON on **stdout**, runtime events/progress on **stderr**.

---

## Command Reference

### Discovery
| Command | Notes |
|---------|-------|
| `scan` | List all attached devices (no session needed) |
| `info` | Show active device info (requires `open`) |

### Session
| Command | Notes |
|---------|-------|
| `open <idx>` | Activate device by scan index |
| `close` | Release device |
| `run` | Batch mode: read commands from stdin |

### Configuration
| Command | Notes |
|---------|-------|
| `modes` | List all capture modes |
| `channels` | List channels with enable state |
| `enable <ch> <0\|1>` | Enable/disable individual channel |
| `vth <1.8\|2.5\|3.3\|5.0>` | Voltage threshold |

### Mode & Rate
| Command | Notes |
|---------|-------|
| `stream <0\|1>` | 0=Buffer, 1=Stream |
| `channel_mode <id>` | Set capture mode |
| `samplerate <hz>` | Sample rate in Hz |
| `limit_samples <n>` | Manual sample limit (normally auto-set by `capture`) |

### Trigger
| Command | Notes |
|---------|-------|
| `trigger` | Show current state |
| `trigger reset` | Clear all stages |
| `trigger enable <0\|1>` | Global enable/disable |
| `trigger pos <0-100>` | Pre-trigger position % |
| `trigger stage <st> <mask> <t0> [t1] [logic] [inv0] [inv1] [cnt0] [cnt1]` | Configure stage |

### Capture
```
capture [duration_sec] [outfile|-] [timeout_sec]
```

| Param | Default | Description |
|-------|---------|-------------|
| `duration_sec` | 0 (auto-max) | Capture duration in seconds. Clamped to device max |
| `outfile` | none | Binary output file, or `-` for stdout |
| `timeout_sec` | 30 | Max wait time |

**Duration auto-logic**:
- `capture 0` → auto-max (device hardware limit)
- `capture 1.0` → request 1s, clamped to max if exceeds
- Auto-queries `SR_CONF_HW_DEPTH` (FPGA buffer / effective channels)
- Auto-disables unused channels (prevents USB bandwidth overflow)
- Stderr: `{"info":{"samplerate":...,"actual_dur":...,"samples":...,"max_dur":...}}`

---

## Channel Modes (DSLogic Plus)

| ID | Name | Max Rate | Max Ch | Data Rate | Type |
|----|------|----------|--------|-----------|------|
| 0 | STREAM20x16 | 20 MHz | 16 | ~40 MB/s | Stream |
| 1 | STREAM25x12 | 25 MHz | 12 | ~37.5 MB/s | Stream |
| 2 | STREAM50x6 | 50 MHz | 6 | ~37.5 MB/s | Stream |
| 3 | STREAM100x3 | 100 MHz | 3 | ~37.5 MB/s | Stream |
| 20 | BUFFER100x16 | 100 MHz | 16 | — | Buffer |
| 21 | BUFFER200x8 | 200 MHz | 8 | — | Buffer |
| 22 | BUFFER400x4 | 400 MHz | 4 | — | Buffer |

**Stream mode**: USB real-time. Limited by USB bandwidth (~38 MB/s effective).
**Buffer mode**: FPGA DDR first (256MB), then USB readback.

### Auto-Max Durations per Mode

| Mode | Effective Ch | HW Buffer/Ch | Auto Duration @Max Rate |
|------|-------------|-------------|------------------------|
| 0 (20MHz×16) | 16 | 16M | 0.800s |
| 1 (25MHz×12) | 12 | 21.3M | 0.853s |
| 2 (50MHz×6) | 6 | 42.7M | 0.853s |
| 3 (100MHz×3) | 3 | 85.3M | 0.853s |
| 20 (buffer) | 16 | 16M | 0.160s |
| 21 (buffer) | 8 | 32M | 0.160s |
| 22 (buffer) | 4 | 64M | 0.160s |

---

## Trigger Reference

### Stage Parameters

| Param | Type | Description |
|-------|------|-------------|
| `st` | uint | Stage index (0-based) |
| `mask_hex` | hex | Probe mask: D0=0x0001, D7=0x0080, D8=0x0100 |
| `t0` | string | Pattern 0: 8-char per group (0/1/R/F/X) |
| `t1` | string | Pattern 1 (default: `XXXXXXXX`) |
| `logic` | int | 0=OR, 1=AND, 2=NOR, 3=NAND |
| `inv0` | int | Invert t0 bits |
| `inv1` | int | Invert t1 bits |
| `cnt0` | uint | t0 match count |
| `cnt1` | uint | t1 match count |

**Pattern chars**: `0`=low, `1`=high, `R`=rising edge, `F`=falling edge, `X`=don't care.

### Examples

```bash
# D0 rising edge
trigger stage 0 0x0001 XXXXXXXR

# D3 falling edge
trigger stage 0 0x0008 XXXXXXFX

# D0 high + D1 low (level)
trigger stage 0 0x0003 XXXXXX10

# Two-stage: D0↑ then D1↓
trigger stage 0 0x0001 XXXXXXXR
trigger stage 1 0x0002 XXXXXXFX

# Free-running (no trigger)
trigger enable 0
```

---

## Data Format

Raw binary, little-endian bit packing:

| Channels | Format |
|----------|--------|
| 1-8 | 1 byte/sample, D0=bit0 |
| 9-16 | 2 bytes/sample, LE |

Python parsing:
```python
data = open('/tmp/cap.bin', 'rb').read()
for i, b in enumerate(data):
    print(f'{i:08d}  {b & 0x0F:04b}')  # lower nibble = D3-D0
```

---

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| `scan_failed` | Check USB, verify firmware files exist at `$DSL_FW_DIR` |
| `open_failed` | `pkill DSView` — only one process can claim DSLogic |
| `start_failed` | Verify samplerate + channel_mode combo is valid |
| 0 bytes captured | MCU halted? Set `channel_mode` after `stream 1`? |
| Overflow in stderr | `capture` auto-fixes this; check if device is in use |
| `libsigrok4DSL.so` not found | `sudo ldconfig` |

---

## Verified Profiles (2026-05-15)

Tested with DSLogic Plus, Raspberry Pi 5 ARM64.

All 11 tests pass: zero overflow, correct byte counts, real signal data.
See README.md for the full verification table.
