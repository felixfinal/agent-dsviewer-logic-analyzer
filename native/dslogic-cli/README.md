# dslogic-cli — DSLogic Native CLI

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-3.2.0-green.svg)](CHANGELOG.md)
[![Language](https://img.shields.io/badge/language-C99-orange.svg)](src/dslogic-cli.c)

**Native command-line interface for DreamSourceLab DSLogic logic analyzers**, built directly on `libsigrok4DSL.so` — the same library DSView GUI uses. No Python, no GUI, no dependencies beyond `glib2`. Single C file (~820 lines). JSON output on stdout. Capture raw logic data and decode UART/SPI/I2C — all in one binary.

## ✨ Features

- **Zero-overhead capture** — same FPGA path as DSView, no Python or GUI overhead
- **Smart auto-capture** — `capture 0` auto-calculates max duration from hardware limits, auto-disables unused channels to prevent USB overflow
- **All modes supported** — stream modes (20/25/50/100 MHz) and buffer modes (100/200/400 MHz)
- **Full trigger support** — single-stage, multi-stage, edge/level, pre-trigger positioning
- **JSON output** — all results on stdout as JSON, diagnostics on stderr — easy to pipe to `jq`
- **Run mode** — pipe multiple commands via stdin for multi-step workflows
- **Configurable firmware path** — `DSL_FW_DIR` environment variable, useful for non-standard installs

## 📦 Quick Start

```bash
# 1. Scan for devices (no session needed)
dslogic-cli scan

# 2. 100MHz stream capture, 3 channels, 1 second max
echo '
open 1
stream 1
channel_mode 3
samplerate 100000000
capture 0 /tmp/capture.bin 15
' | dslogic-cli run

# 3. With trigger on D0 rising edge, 20% pre-trigger
echo '
open 1
trigger reset
trigger stage 0 0x0001 XXXXXXXR
trigger pos 20
stream 1
channel_mode 3
samplerate 100000000
capture 0 /tmp/triggered.bin 30
' | dslogic-cli run

# 4. Buffer mode — 400MHz × 4 channels (max speed)
echo '
open 1
stream 0
channel_mode 22
samplerate 400000000
capture 0 /tmp/fast.bin 10
' | dslogic-cli run
```

## 📋 Command Reference

### Discovery
| Command | Description |
|---------|-------------|
| `scan` | List all attached devices |
| `info` | Show active device details |

### Session
| Command | Description |
|---------|-------------|
| `open <idx>` | Activate device by scan index |
| `close` | Release device |
| `run` | Batch mode: read commands from stdin |

### Configuration
| Command | Description |
|---------|-------------|
| `modes` | List capture modes |
| `channels` | List channels with enable state |
| `enable <ch> <0\|1>` | Enable/disable channel (0-indexed) |
| `vth <1.8\|2.5\|3.3\|5.0>` | Voltage threshold |

### Mode & Rate
| Command | Description |
|---------|-------------|
| `stream <0\|1>` | 0=Buffer, 1=Stream |
| `channel_mode <id>` | Set capture mode (see table) |
| `samplerate <hz>` | Sample rate in Hz |
| `limit_samples <n>` | Manual sample limit (auto-set by capture) |

### Trigger
| Command | Description |
|---------|-------------|
| `trigger` | Show current state |
| `trigger reset` | Clear all stages |
| `trigger enable <0\|1>` | Global enable/disable |
| `trigger pos <0-100>` | Pre-trigger position (%) |
| `trigger stage <st> <mask> <t0> [t1] ...` | Configure trigger stage |

### Capture
```
capture [duration_sec] [outfile] [timeout_sec]
```
- `duration_sec`: 0 = auto-max, otherwise clamped to device limit
- `outfile`: binary output path, or `-` for stdout
- `timeout_sec`: max wait time (default 30)

### Meta
| Command | Output |
|---------|--------|
| `--version` | `{"version":"3.2.0"}` |
| `--help` | Usage text |

## 📊 Channel Modes (DSLogic Plus)

| ID | Name | Max Rate | Max Ch | Data Rate | Type |
|----|------|----------|--------|-----------|------|
| 0 | STREAM20x16 | 20 MHz | 16 | ~40 MB/s | Stream |
| 1 | STREAM25x12 | 25 MHz | 12 | ~37.5 MB/s | Stream |
| 2 | STREAM50x6 | 50 MHz | 6 | ~37.5 MB/s | Stream |
| 3 | STREAM100x3 | 100 MHz | 3 | ~37.5 MB/s | Stream |
| 20 | BUFFER100x16 | 100 MHz | 16 | — | Buffer |
| 21 | BUFFER200x8 | 200 MHz | 8 | — | Buffer |
| 22 | BUFFER400x4 | 400 MHz | 4 | — | Buffer |

- **Stream**: USB real-time, limited by USB 2.0 bandwidth (~38 MB/s)
- **Buffer**: FPGA DDR first (256MB), then USB readback — independent of USB bandwidth

### Auto-Max Durations

| Mode | Effective Ch | Max Duration | Max Samples |
|------|-------------|-------------|-------------|
| 0 (20MHz×16) | 16 | 0.84s | 16.8M |
| 1 (25MHz×12) | 12 | 0.90s | 22.4M |
| 2 (50MHz×6) | 6 | 0.90s | 44.7M |
| 3 (100MHz×3) | 3 | 0.90s | 89.5M |
| 20 (buffer) | 16 | 0.17s | 16.8M |
| 21 (buffer) | 8 | 0.17s | 33.6M |
| 22 (buffer) | 4 | 0.17s | 67.1M |

## 🔧 Build & Install

## Important Dependency Boundary

`dslogic-cli` is a native CLI frontend for DSView's DSLogic driver stack. It is
not a standalone driver implementation. The CLI source is ours, but successful
runtime operation depends on DSView build artifacts:

- `libsigrok4DSL.so` installed, normally at `/usr/local/lib/libsigrok4DSL.so`
- DSView firmware/resource files available at `/opt/dslogic/res` or `DSL_FW_DIR`
- ABI compatibility between the vendored compile-time headers and the installed
  `libsigrok4DSL.so`

The headers under `third_party/libsigrok4DSL` are only for compiling this CLI.
They do not replace the DSView shared library or firmware resources.

### Prerequisites

```bash
# System dependencies
sudo apt install build-essential pkg-config libglib2.0-dev
```

Headers for `libsigrok4DSL` are **vendored** in `third_party/` — no need to install DSView to compile.

However, **`libsigrok4DSL.so` is required at runtime** (`/usr/local/lib/`). Build it from [DSView](https://github.com/DreamSourceLab/DSView):

```bash
git clone https://github.com/DreamSourceLab/DSView.git
cd DSView && mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
make -j4 && sudo make install
```

### Build

```bash
cd src
make            # compile
sudo make install   # install to /usr/local/bin/
make clean      # remove build artifacts
```

### Environment

| Variable | Purpose | Default |
|----------|---------|---------|
| `DSL_FW_DIR` | Path to FPGA firmware bitstreams | `/opt/dslogic/res` |

Firmware files (`.bin`, `.fw`, `.dsc`) are **not** included in this repo. Copy them from your DSView installation or the [DSView firmware directory](https://github.com/DreamSourceLab/DSView/tree/master/res).

## 🔬 Protocol Decode

Pure C protocol decoders — no Python, no `libsigrokdecode` required.

```bash
# UART on D0, 115200 8N1
dslogic-cli decode uart capture.bin --samplerate 100000000

# UART with custom settings
dslogic-cli decode uart capture.bin --samplerate 100000000 \
    --channel 0 --baudrate 9600 --data-bits 8 --parity odd

# SPI on D0(CLK) D1(MISO) D2(MOSI), mode 0
dslogic-cli decode spi capture.bin --samplerate 100000000 \
    --clk 0 --miso 1 --mosi 2 --cpol 0 --cpha 0

# I2C on D0(SCL) D1(SDA)
dslogic-cli decode i2c capture.bin --samplerate 100000000 --scl 0 --sda 1
```

### Output

One JSON object per decoded element on stdout:

```json
{"start":64,"end":9612,"type":"data","value":221}
{"start":9664,"end":19212,"type":"error","value":221,"frame_error":true}
```

### UART Options

| Option | Default | Values |
|--------|---------|--------|
| `--channel` | 0 | D0-D15 |
| `--baudrate` | 115200 | any |
| `--data-bits` | 8 | 5-9 |
| `--parity` | none | none/odd/even |
| `--stop-bits` | 1 | 1/1.5/2 |

### SPI Options

| Option | Default | Values |
|--------|---------|--------|
| `--clk` | 0 | D0-D15 |
| `--miso` | 1 | D0-D15 or -1 (disabled) |
| `--mosi` | 2 | D0-D15 or -1 (disabled) |
| `--wordsize` | 8 | any |
| `--cpol` | 0 | 0/1 |
| `--cpha` | 0 | 0/1 |
| `--bit-order` | msb | msb/lsb |

### I2C Options

| Option | Default | Values |
|--------|---------|--------|
| `--scl` | 0 | D0-D15 |
| `--sda` | 1 | D0-D15 |

## 🐍 Data Format

Raw binary, little-endian bit packing:

| Channels | Format |
|----------|--------|
| 1-8 | 1 byte/sample, D0=bit0, D7=bit7 |
| 9-16 | 2 bytes/sample, LE |

```python
data = open('/tmp/capture.bin', 'rb').read()
for i, b in enumerate(data):
    print(f'{i:08d}  {b & 0x0F:04b}')  # D3-D0
```

## 📈 Verified

All 11 test profiles verified 2026-05-15 with DSLogic Plus on Raspberry Pi 5 ARM64:

| # | Ch | Mode | Rate | Duration | Samples | Trigger | Result |
|---|----|------|------|----------|---------|---------|--------|
| 1 | 16 | Stream | 20 MHz | 0.84s | 16.8M | — | ✅ |
| 2 | 12 | Stream | 25 MHz | 0.90s | 22.4M | — | ✅ |
| 3 | 6 | Stream | 50 MHz | 0.90s | 44.7M | — | ✅ |
| 4 | 3 | Stream | 100 MHz | 0.90s | 89.5M | — | ✅ |
| 5 | 16 | Buffer | 100 MHz | 0.17s | 16.8M | — | ✅ |
| 6 | 8 | Buffer | 200 MHz | 0.17s | 33.6M | — | ✅ |
| 7 | 4 | Buffer | 400 MHz | 0.17s | 67.1M | — | ✅ |
| 8 | 3 | Stream | 100 MHz | 0.90s | 89.5M | D0↑ | ✅ |
| 9 | 3 | Stream | 100 MHz | 0.90s | 89.5M | D3↓ | ✅ |
| 10 | 6 | Stream | 50 MHz | 0.90s | 44.7M | disabled | ✅ |
| 11 | 3 | Stream | 100 MHz | 0.20s | 20.0M | — | ✅ |

**Zero overflow, correct byte counts, real signal data.**

## 🔗 Related

- [DreamSourceLab DSView](https://github.com/DreamSourceLab/DSView) — upstream GUI application
- [sigrok](https://sigrok.org/) — the protocol analysis framework
- `docs/BUILD.md` — detailed build guide
- `docs/USAGE.md` — comprehensive usage with trigger examples and troubleshooting

## 📄 License

- **dslogic-cli** (CLI source): [MIT](LICENSE)
- **libsigrok4DSL headers** (vendored in `third_party/`): [GPLv3](third_party/libsigrok4DSL/LICENSE) — part of the [libsigrok](https://sigrok.org/) project
- **libsigrok4DSL.so**: GPLv3 — built separately from [DSView](https://github.com/DreamSourceLab/DSView)
