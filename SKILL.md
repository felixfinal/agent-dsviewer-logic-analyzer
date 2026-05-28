---
name: agent-dsviewer-logic-analyzer
description: Agent-facing capability reference for DSView/sigrok logic-analyzer workflows. Use when an agent needs to understand the implemented scan, capture, decode, DSLogic mode, UART validation, and safety capabilities in this repository. For dependency installation and build steps, read INSTALL.md instead.
---

# Agent DSViewer Logic Analyzer

## Role

This skill describes the implemented capabilities in this repository. It is the
agent-facing index for deciding what the DSView/sigrok logic-analyzer tool layer
can do and when to use each function.

Dependency installation, Python build, MCP registration, and skill copy commands
are intentionally kept in `INSTALL.md`.

## Layer Model

- `logic-analyzer-agent`
  - Top-level planning skill.
  - Owns measurement intent, channel mapping assumptions, sampling strategy, and pass/fail interpretation.
- `dsview-logic-analyzer`
  - Concrete DSView/sigrok execution skill.
  - Owns setup checks, capture workflow, decoder workflow, and known hardware notes.
- `dsview-logic`
  - MCP/runner tool layer implemented in `src/dsview_logic/`.
  - Owns stable status, scan, capture, and decode calls.

## Implemented Tool Functions

### `logic_analyzer_status`

Checks local tool readiness and returns:

- DSView executable path and version when available.
- `sigrok-cli` executable path and version.
- whether `/usr/share/sigrok-firmware` exists.
- whether the DreamSourceLab udev rule exists at `/etc/udev/rules.d/60-dreamsourcelab.rules`.
- default output directory.
- overall `ok` flag requiring both DSView and `sigrok-cli`.

Use this before any hardware capture task.

### `logic_analyzer_scan`

Runs `sigrok-cli --scan` and returns:

- parsed non-empty device lines.
- raw scan text.
- command argv and return code.

Use this after the analyzer is physically connected. Do not claim hardware
validation until scan sees a device.

### `logic_analyzer_list_decoders`

Runs `sigrok-cli --list-supported`, extracts the protocol decoder section, and
supports optional query filtering.

Use this before protocol work when the decoder name or support status is unclear.
Examples of useful queries: `uart`, `i2c`, `spi`, `can`, `pwm`.

### `logic_analyzer_capture`

Builds and runs bounded `sigrok-cli` captures.

Supported payload fields:

- `driver`: optional sigrok driver, for example `dreamsourcelab-dslogic`.
- `channels`: enabled digital channels, for example `0,1,2,3`.
- `config`: string or object converted to `--config`, commonly `{"samplerate": "1m"}`.
- `triggers`: optional sigrok trigger expression.
- `wait_trigger`: optional boolean to wait for trigger.
- `samples`: bounded sample count, default `1000`.
- `time`: bounded capture duration; overrides `samples` when supplied.
- `output_file`: output path or basename.
- `output_format`: `csv`, `vcd`, `srzip`, or user-facing alias `sr`.
- `timeout`: subprocess timeout in seconds.

Output behavior:

- Relative `output_file` values are written under `deliverables/logic-analyzer/`.
- `output_format="sr"` is normalized to sigrok's `srzip` writer while keeping `.sr` as the normal filename suffix.
- Return includes file size, effective format, argv, shell-safe command string, stdout, stderr, and return code.

Use CSV for quick script checks, VCD for generic waveform viewers, and `.sr`
session files for protocol decoder workflows.

### `logic_analyzer_decode_file`

Runs a generic sigrok protocol decoder against a saved capture.

Supported payload fields:

- `input_file`: required saved capture path.
- `decoder`: required sigrok decoder expression.
- `annotations`: optional annotation selector.
- `samplenum`: optional boolean to include sample numbers.
- `timeout`: subprocess timeout in seconds.

Use this for decoders other than the normalized UART helper or when custom
decoder options are needed.

### `logic_analyzer_uart_decode_file`

Decodes UART from saved `.sr`/srzip captures with stable defaults.

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
- generated sigrok decoder string.
- selected annotation.
- raw stdout/stderr.

The implementation preserves decoded ASCII spaces and maps common bracketed hex
tokens such as `[0D]` and `[0A]` to carriage return and newline.

### `logic_analyzer_dsview_info`

Reports DSView GUI binary information:

- executable path.
- version output.
- help output lines.
- note that DSView is GUI-first and scripted capture should use `sigrok-cli`.

Use this for local setup diagnostics, not for scripted capture.

## DSLogic Capture Modes

Known DSLogic Plus mode guidance derived from the local DSView/libsigrok4DSL
source baseline:

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
- Start with low-risk smoke captures such as `1MHz` and `1000` samples.
- For protocol decoding, choose a sample rate comfortably above the protocol
  bit rate; for UART 115200, the local baseline used 1 MHz successfully.
- Treat captures as snapshots of current wiring. Do not infer stable wiring from an earlier run.

## Output Formats

- `csv`
  - Best for quick validation scripts and frequency/edge interval checks.
- `vcd`
  - Best for generic waveform viewers and EDA tooling.
- `sr` / `srzip`
  - Best for sigrok decoder workflows.
  - On the local Debian/sigrok baseline, the raw session writer is `srzip`; the tool accepts `sr` as a user-facing alias.

## Supported Workflow Patterns

### GPIO / Square Wave Validation

1. Record probe-to-channel mapping.
2. Run status and scan.
3. Capture a short CSV at a conservative sample rate.
4. Compute edge intervals or inspect waveform.
5. Distinguish adjacent-edge interval from full high-to-high or low-to-low period.

For a 50% square wave, adjacent edge spacing is half of the full period.

### UART Validation

1. Capture TX as one digital channel first.
2. Save both CSV for basic signal checks and `.sr`/srzip for decoder work.
3. Decode with `logic_analyzer_uart_decode_file`.
4. Report channel, baud rate, frame options, capture file path, decoded text,
   and whether stop-bit or framing errors appeared in raw decoder output.

Validated local baseline:

- HPM5300EVK UART2 TX on PB08 to DSLogic D0.
- `115200 8N1`.
- 1 MHz sampling.
- decoded repeated text like `HPM5300EVK UART2 PB08 115200 8N1 seq=N`.

### Generic Protocol Decode

1. Use `logic_analyzer_list_decoders` to confirm decoder availability.
2. Capture to `.sr`/srzip.
3. Run `logic_analyzer_decode_file` with the exact decoder expression.
4. Report decoder expression and selected annotations.

## Safety Boundaries

- Keep captures bounded by samples or time unless the user explicitly requests continuous capture.
- Do not assume probe wiring. Record the channel-to-signal mapping in the result.
- Prefer read-only/status checks until the user says the analyzer is connected.
- Do not claim hardware validation until both scan and at least one bounded capture succeed.
- Do not vendor or redistribute upstream DSView source trees or firmware blobs from this repository.
