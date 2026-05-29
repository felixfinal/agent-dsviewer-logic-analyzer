---
name: agent-dsviewer-logic-analyzer
description: Agent-facing capability reference for DSView/libsigrok4DSL native logic-analyzer workflows. Use when an agent needs to understand the implemented dslogic-cli scan, capture, decode, DSLogic mode, and safety capabilities in this repository. For dependency installation and build steps, read INSTALL.md instead.
---

# Agent DSViewer Logic Analyzer

## Role

This skill describes the implemented capabilities in this repository. It is the
agent-facing index for deciding what the DSView/libsigrok4DSL native
logic-analyzer tool layer can do and when to use each function.

Dependency installation, Python build, MCP registration, and skill copy commands
are intentionally kept in `INSTALL.md`.

## Layer Model

- `logic-analyzer-agent`
  - Top-level planning skill.
  - Owns measurement intent, channel mapping assumptions, sampling strategy, and
    pass/fail interpretation.
- `dsview-logic-analyzer`
  - Concrete DSView/libsigrok4DSL native execution skill.
  - Owns setup checks, capture workflow, decoder workflow, and known hardware
    notes.
- `dsview-logic`
  - MCP/runner tool layer implemented in `src/dsview_logic/`.
  - Owns stable status, scan, capture, and decode calls.
- `native/dslogic-cli`
  - Native C backend for DreamSourceLab DSLogic devices.
  - Links to DSView's `libsigrok4DSL.so`.
  - Owns high-rate DSLogic Plus modes such as stream `100MHz x3ch`.

## Installation Boundary

`dslogic-cli` is the native CLI code in this repository, but it depends on
DSView build outputs. Other agents must treat the install as:

1. Build/install DSView's `libsigrok4DSL.so`.
2. Install/copy DSView firmware resources to `/opt/dslogic/res` or set
   `DSL_FW_DIR`.
3. Build/install `native/dslogic-cli/src`.
4. Verify `logic_analyzer_status.native_ok == true`.

The vendored `native/dslogic-cli/third_party/libsigrok4DSL` headers are not a
replacement for the runtime `libsigrok4DSL.so`; they only let the CLI compile.
If native readiness fails, fix DSView/libsigrok4DSL.

## Implemented Tool Functions

### `logic_analyzer_status`

Checks local tool readiness and returns:

- DSView executable path and version when available.
- native `dslogic-cli` executable path and version when available.
- whether `/usr/local/lib/libsigrok4DSL.so` exists.
- whether native firmware resources exist.
- whether the DreamSourceLab udev rule exists at
  `/etc/udev/rules.d/60-dreamsourcelab.rules`.
- default output directory.
- `native_ok`, `gui_ok`, and overall `ok`.

Use this before any hardware capture task.

### `logic_analyzer_scan`

Compatibility wrapper for native `dslogic-cli scan`.
Alias for `logic_analyzer_native_scan`.

Use this after the analyzer is physically connected. Do not claim hardware
validation until scan sees a device.

### `logic_analyzer_native_scan`

Runs `dslogic-cli scan`.

Use this for all DSLogic hardware scan operations.

### `logic_analyzer_list_decoders`

Returns native `dslogic-cli` built-in decoders and supports optional query
filtering.

Use this before protocol work when the decoder name or support status is
unclear. Supported native decoders are currently `uart`, `i2c`, and `spi`.

### `logic_analyzer_capture`

Compatibility alias for `logic_analyzer_native_capture`.

### `logic_analyzer_native_capture`

Runs `dslogic-cli run` for native DSLogic captures.

Default payload values implement the DSLogic Plus 100 MHz x3 stream path:

- `device_index`: `1`
- `stream`: `1`
- `channel_mode`: `3`
- `samplerate`: `100000000`
- `duration`: `0` for auto-max hardware-limited capture
- `output_file`: optional raw binary file name
- `timeout`: subprocess timeout seconds

The generated command script is equivalent to:

```text
open 1
stream 1
channel_mode 3
samplerate 100000000
capture 0 deliverables/logic-analyzer/dslogic-100m-x3.bin 15
```

Use this for DSLogic Plus high-rate stream/buffer modes. The output is raw
binary logic data.

### `logic_analyzer_decode_file`

Compatibility alias for `logic_analyzer_native_decode_file`.

### `logic_analyzer_uart_decode_file`

Compatibility alias for native UART decode from raw `.bin` captures.

Supported payload fields:

- `input_file`: required saved capture path.
- `channel` or `rx`: analyzer channel, default `0`.
- `baudrate`: default `115200`.
- `data_bits`: default `8`.
- `parity`: default `none`.
- `stop_bits`: default `1`.
- `format`: default `ascii`.
- `direction`: `rx` or `tx`, default `rx`.
- `samplenum`: optional boolean to include sample numbers.
- `timeout`: subprocess timeout in seconds.

Return includes:

- reconstructed `text`.
- individual decoded `tokens`.
- raw stdout/stderr.

### `logic_analyzer_native_decode_file`

Runs `dslogic-cli decode` against a native raw binary capture.

Supported decoders:

- `uart`
- `spi`
- `i2c`

Common payload fields:

- `input_file`: required raw binary file.
- `decoder`: `uart`, `spi`, or `i2c`.
- `samplerate`: defaults to `100000000`.
- UART options: `channel`, `baudrate`, `data_bits`, `parity`, `stop_bits`.
- SPI options: `clk`, `miso`, `mosi`, `cpol`, `cpha`, `wordsize`, `bit_order`.
- I2C options: `scl`, `sda`.

### `logic_analyzer_dsview_info`

Reports DSView GUI binary information:

- executable path.
- version output.
- help output lines.
- note that DSView is GUI-first and scripted capture should use
  `dslogic-cli/libsigrok4DSL`.

Use this for local setup diagnostics, not for scripted capture.

## DSLogic Capture Modes

DSLogic Plus mode guidance derived from the DSView/libsigrok4DSL source:

| Mode | Maximum capture shape | Agent guidance |
| --- | --- | --- |
| Stream | `20MHz x16ch` | Use when many channels matter more than high sample rate. |
| Stream | `25MHz x12ch` | Use for medium-width captures. |
| Stream | `50MHz x6ch` | Use for faster signals with reduced channel count. |
| Stream | `100MHz x3ch` | Use when high stream rate matters and only a few channels are needed. |
| Buffer | `100MHz x16ch` | Use for short high-speed captures across all channels. |
| Buffer | `200MHz x8ch` | Use for short high-speed captures with half channel width. |
| Buffer | `400MHz x4ch` | Use for highest-rate short captures with limited channels. |

Agent rules:

- Reduce enabled channels before requesting high sample rates.
- Use the native `dslogic-cli` backend for all DSLogic scan/capture/decode work.
- Start with low-risk smoke captures such as `1MHz` and `1000` samples.
- For protocol decoding, choose a sample rate comfortably above the protocol
  bit rate.
- Treat captures as snapshots of current wiring. Do not infer stable wiring from
  an earlier run.

## Output Format

- Native captures write raw binary `.bin`.
- Use `scripts/capture_verify.py` for quick signal sanity checks.
- Use `logic_analyzer_native_decode_file` for UART/SPI/I2C protocol decode.

## Supported Workflow Patterns

### GPIO / Square Wave Validation

1. Record probe-to-channel mapping.
2. Run status and scan.
3. Capture a short bounded raw `.bin` at a conservative sample rate.
4. Run `scripts/capture_verify.py` or native decode as appropriate.
5. Distinguish adjacent-edge interval from full high-to-high or low-to-low
   period. For a 50% square wave, adjacent edge spacing is half of the full
   period.

### UART Validation

1. Capture TX as one digital channel first.
2. Save a native raw `.bin` capture.
3. Decode with `logic_analyzer_native_decode_file`.
4. Report channel, baud rate, frame options, capture file path, decoded text,
   and whether stop-bit or framing errors appeared in raw decoder output.

### Generic Protocol Decode

1. Use `logic_analyzer_list_decoders` to confirm native decoder availability.
2. Capture to raw `.bin`.
3. Run `logic_analyzer_native_decode_file`.
4. Report decoder options and decoded output.

### Native 100 MHz x3 Capture

1. Confirm `logic_analyzer_status.native_ok` is true.
2. Run `logic_analyzer_native_scan` and confirm the DSLogic device is visible.
3. Run `logic_analyzer_native_capture` with `stream=1`, `channel_mode=3`, and
   `samplerate=100000000`.
4. Decode raw binary with `logic_analyzer_native_decode_file` when needed.
5. Report the raw file path, byte count, mode, sample rate, and trigger state.

## Safety Boundaries

- Keep captures bounded by samples or time unless the user explicitly requests
  continuous capture.
- Do not assume probe wiring. Record the channel-to-signal mapping in the
  result.
- Prefer read-only/status checks until the user says the analyzer is connected.
- Do not claim hardware validation until both scan and at least one bounded
  capture succeed.
- Do not vendor or redistribute upstream DSView source trees or firmware blobs
  from this repository.
- Native `dslogic-cli` vendors only the header subset needed to compile. Keep
  its local license notices intact.
