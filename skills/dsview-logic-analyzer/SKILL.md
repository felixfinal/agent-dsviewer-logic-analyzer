---
name: dsview-logic-analyzer
description: Concrete DSView/DreamSourceLab logic-analyzer workflow using only the verified dslogic-cli/libsigrok4DSL backend. Use for DSView native library setup, DSLogic, dslogic-cli, libsigrok4DSL, DSLogic firmware resources, udev permission checks, 100MHz x3ch capture, raw binary protocol decode, or validating a connected USB logic analyzer. Use logic-analyzer-agent as the top-level entry when broader measurement planning or signal interpretation is needed.
---

# DSView Logic Analyzer

## Stable local tool layer
Use the `dsview-logic` MCP when available. It wraps:
- `DSView` GUI presence/version checks only as environment context.
- `dslogic-cli`/`libsigrok4DSL` readiness checks.
- native `dslogic-cli scan`.
- bounded native captures to raw `.bin` files.
- native UART/SPI/I2C decode from raw `.bin` files.
- DSLogic Plus stream/buffer modes, including stream `100MHz x3ch`.

If MCP is unavailable in the current session, use the same runner directly.
Practical direct runner examples:

```bash
python3 server.py --list-tools
python3 server.py logic_analyzer_status
python3 server.py logic_analyzer_scan
python3 server.py logic_analyzer_native_scan
python3 server.py logic_analyzer_native_capture --input '{"channel_mode":3,"samplerate":100000000,"output_file":"native-100m-x3.bin","timeout":15}'
python3 server.py logic_analyzer_native_decode_file --input '{"input_file":"deliverables/logic-analyzer/native-100m-x3.bin","decoder":"uart","channel":"0","baudrate":115200}'
```

When installed as a Python package, `dsview-logic-runner` provides the same
runner CLI.

## Capture defaults
- Default output root: `deliverables/logic-analyzer/`.
- Prefer short bounded captures first unless validating an auto-max profile.
- Native capture output is raw binary `.bin`, not `.sr`, `csv`, or `vcd`.
- Use `scripts/capture_verify.py` for raw `.bin` sanity checks.
- When measuring square waves, distinguish **edge interval** from **full
  period**. A signal toggled every 1 ms has adjacent-edge spacing ~1 ms but
  full high-to-high period ~2 ms. State this explicitly before calling a
  waveform wrong.

## DreamSourceLab DSLogic notes
- Local native access is through `dslogic-cli` linked to DSView's
  `libsigrok4DSL.so`.
- Install DSView firmware resources under `/opt/dslogic/res` or set
  `DSL_FW_DIR`.
- DSLogic Plus supports multiple logic capture modes. From the local
  DSView/libsigrok4DSL source, relevant Plus modes are:
  - Stream: `20MHz x16ch`, `25MHz x12ch`, `50MHz x6ch`, `100MHz x3ch`.
  - Buffer: `100MHz x16ch`, `200MHz x8ch`, `400MHz x4ch`.
- For high samplerates, reduce enabled channels to match the mode limit. For
  low-speed GPIO validation, use all needed channels at 1 MHz and keep
  interpretation simple.
- `100MHz x3ch` path: `dslogic-cli` -> `libsigrok4DSL.so` -> DSLogic Plus
  stream mode.
- Native backend install requirements: build/install DSView's
  `libsigrok4DSL.so`, install firmware resources under `/opt/dslogic/res` or
  set `DSL_FW_DIR`, then build `native/dslogic-cli/src`.

## Installation warning for other agents
- Treat `dslogic-cli` as a DSView/libsigrok4DSL companion tool, not a
  standalone logic-analyzer stack.
- The vendored `third_party/libsigrok4DSL` headers are only a compile-time
  convenience for the CLI. They do not replace DSView's built
  `libsigrok4DSL.so`.
- A working install requires DSView/libsigrok4DSL runtime artifacts:
  - `/usr/local/lib/libsigrok4DSL.so` from a DSView build/install.
  - firmware/resource files copied from DSView `res/` into `/opt/dslogic/res`
    or made available through `DSL_FW_DIR`.
  - matching enough DSView/libsigrok4DSL ABI between the headers used to
    build `dslogic-cli` and the installed shared library.
- Other agents must run `logic_analyzer_status` and require `native_ok=true`
  before attempting capture. If `native_ok=false`, fix DSView/libsigrok4DSL
  installation first.

## Backend rule
- Use `dslogic-cli` for all DSLogic scan/capture/decode operations.
- Treat `logic_analyzer_status.native_ok` as the readiness gate.
- `gui_ok` only means the GUI binary exists; it is not a scripted capture
  path.

## Native 100MHz x3 workflow
The native command sequence is:

```text
open 1
stream 1
channel_mode 3
samplerate 100000000
capture 0 deliverables/logic-analyzer/native-100m-x3.bin 15
```

Required checks:
- `logic_analyzer_status.native_ok` must be true.
- `logic_analyzer_native_scan` must see the DSLogic device.
- Use bounded `timeout`; do not start open-ended captures.
- Output is raw binary logic data. Decode it with
  `logic_analyzer_native_decode_file` or tooling that understands the native
  raw packing.

## References
Included references:

- `references/channel-modes.md` — DSLogic Plus mode table and USB bandwidth
  math.
- `references/dsl-driver-internals.md` — libsigrok4DSL stream stop condition
  and buffer sizing.
- `references/stream-overflow-analysis.md` — root cause discussion of buffer
  overflow during long captures.
- `references/dsl-format.md` — DSView `.dsl` archive layout vs CLI raw
  binary.
- `references/pure-c-decoders.md` — native UART/SPI/I2C decoder behavior.
- `scripts/capture_verify.py` — quick raw `.bin` sanity checker.

## Native capture pitfalls
- Set `channel_mode` explicitly after `stream`; default stream mode is `0`
  (`20MHz x16ch`), not `100MHz x3ch`.
- Use numeric channel indices with `enable`: `enable 0 1`, not `enable D0 1`.
- For 3.3V logic, use `vth 1.8`; `vth 3.3` is wrong because it sets the
  comparator threshold at the signal rail and can produce all-high/all-low or
  cloned waveforms.
- Single-channel captures may be bit-packed as 8 samples per byte. If bytes
  are `N/8` of reported samples, unpack before decode or enable multiple
  channels.
- `capture` output reports raw bytes written, not decoded sample count.
- Stop DSView GUI before native CLI capture; only one process can own the
  USB device.
- If the target MCU is halted by a debugger, resume it before capturing.

## Verified native profiles
DSLogic Plus native backend design has been validated for these profiles:

- Stream mode 0: `20MHz x16ch`, auto-max, 32 MB output.
- Stream mode 1: `25MHz x12ch`, auto-max, 32 MB output.
- Stream mode 2: `50MHz x6ch`, auto-max, 32 MB output.
- Stream mode 3: `100MHz x3ch`, auto-max, ~89.5M samples / 32 MB output.
- Buffer mode 20: `100MHz x16ch`, auto-max, 32 MB output.
- Buffer mode 21: `200MHz x8ch`, auto-max, 32 MB output.
- Buffer mode 22: `400MHz x4ch`, auto-max, 32 MB output.
- Triggered stream mode 3 captures on D0 rising and D3 falling also passed.

## UART decode workflow
- For TTL UART, capture the TX line as a digital channel first.
- Preferred MCP decode tool: `logic_analyzer_native_decode_file` with payload
  like:
  - `input_file`: native raw `.bin` path
  - `decoder`: `uart`
  - `channel`: logic analyzer channel, usually `"0"`
  - `samplerate`: capture samplerate, e.g. `100000000`
  - `baudrate`: e.g. `115200`
  - optional `data_bits`, `parity`, `stop_bits`
- The native CLI includes built-in UART/SPI/I2C decoders; use those instead of
  external decoder stacks.

## Boundaries
- DSView itself is primarily GUI. Scripted captures must use native
  `dslogic-cli`.
- Do not start long continuous captures by default.
- Do not claim native 100MHz x3 verification until
  `logic_analyzer_native_scan` sees a device and a native bounded capture
  succeeds.
- Keep native `dslogic-cli` license notices and vendored header license
  notices intact.
