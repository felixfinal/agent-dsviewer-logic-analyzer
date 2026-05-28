---
name: dsview-logic-analyzer
description: Concrete DSView/DreamSourceLab and sigrok logic-analyzer workflow for local setup, device scan, bounded digital captures, saved waveform files, and protocol decoding. Use for DSView 1.3.x, DSLogic, sigrok-cli, fx2lafw firmware, udev permission checks, or validating a connected USB logic analyzer. Use logic-analyzer-agent as the top-level entry when broader measurement planning or signal interpretation is needed.
---

# DSView Logic Analyzer

## Stable local tool layer
Use the `dsview-logic` MCP when available. It wraps:
- `DSView` GUI presence/version checks.
- `sigrok-cli` status and `--scan`.
- bounded captures to files.
- protocol decoder discovery and decode runs.
- normalized UART decode from saved `.sr`/srzip captures.

If MCP is unavailable in the current session, use the same runner directly. Practical direct runner examples:


```bash
python3 server.py --list-tools
python3 server.py logic_analyzer_status
python3 server.py logic_analyzer_scan
python3 server.py logic_analyzer_uart_decode_file --input '{"input_file":"deliverables/logic-analyzer/example.sr","channel":"0","baudrate":115200}'
```

When installed as a Python package, `dsview-logic-runner` provides the same runner CLI.

## Capture defaults
- Default output root: `deliverables/logic-analyzer/`.
- Prefer short captures first: 1 MHz / 1000 samples, or a few milliseconds.
- On this Debian/sigrok build, raw session output format is `srzip`; `--output-format sr` may fail with `Unknown output module 'sr'`. Save with `.sr` extension plus `output_format=srzip` when a PulseView/DSView-style session file is needed.
- Use `csv` output for quick script analysis and validation; use `vcd` output when the user wants waveform viewing in common EDA tools.
- When measuring square waves, distinguish **edge interval** from **full period**. A signal toggled every 1 ms has adjacent-edge spacing ~1 ms but full high-to-high period ~2 ms. State this explicitly before calling a waveform wrong.

## DreamSourceLab DSLogic notes
- Local device seen here as DreamSourceLab DSLogic Plus / DSLogic Pro via sigrok after firmware load; scan strings may change after FX2 firmware upload/re-enumeration.
- Correct DSLogicPlus firmware for sigrok was taken from DreamSourceLab/DSView commit `886b847c21c606df3138ce7ad8f8e8c363ee758b`: install `DSLogicPlus.fw` as `dreamsourcelab-dslogic-plus-fx2.fw` and `DSLogicPlus.bin` as `dreamsourcelab-dslogic-plus-fpga.fw` under `/usr/share/sigrok-firmware/`.
- DSLogic Plus supports multiple logic capture modes. From the local DSView/libsigrok4DSL source, relevant Plus modes are:
  - Stream: `20MHz x16ch`, `25MHz x12ch`, `50MHz x6ch`, `100MHz x3ch`.
  - Buffer: `100MHz x16ch`, `200MHz x8ch`, `400MHz x4ch`.
- For high samplerates, reduce enabled channels to match the mode limit. For low-speed GPIO validation, use all needed channels at 1 MHz and keep interpretation simple.

## HPM5300EVK GPIO validation lesson
- 2026-05-19 validation used HPM5300EVK/HPM5361 + DSLogic. Final confirmed capture: `deliverables/logic-analyzer/hpm5300evk-final-dsview-confirmed-20260519-103650.csv`.
- The firmware eventually used distinct divider-style GPIO outputs: PB08 / PA27 / PA29 / PA25. At 1 MHz capture, expected adjacent-edge intervals can appear as approximately `1/2/4/8 ms`; DSView full-period readings may be `2/4/8/16 ms`. Both are consistent if measuring different definitions.
- During live rewiring, do not over-interpret a single channel being flat. Ask for the stable final wiring state and prefer a known-good channel as a roaming probe before blaming firmware, headers, or analyzer hardware.

## UART decode workflow
- For TTL UART, capture the TX line as a single digital channel first. Example baseline validated on HPM5300EVK: UART2 TX on PB08 → DSLogic D0, `115200 8N1`, 1 MHz sampling.
- Save both a script-friendly `csv` and a decoder-friendly `.sr` session when validating a new protocol.
- Preferred MCP decode tool: `logic_analyzer_uart_decode_file` with payload like:
  - `input_file`: saved `.sr` / srzip path
  - `channel`: logic analyzer channel, usually `"0"`
  - `baudrate`: e.g. `115200`
  - optional `data_bits`, `parity`, `stop_bits`, `direction`
- Generic sigrok UART decoder option name is `parity`, not `parity_type`:
  `uart:rx=0:baudrate=115200:data_bits=8:parity=none:stop_bits=1:format=ascii`.
- 2026-05-19 HPM5300EVK UART validation files: `deliverables/logic-analyzer/hpm5300evk-uart2-pb08-20260519-104152.csv` and `.sr`; decoded text included `HPM5300EVK UART2 PB08 115200 8N1 seq=210/211/212`.

## Boundaries
- DSView itself is primarily GUI; command-line automation should use `sigrok-cli`.
- Do not start long continuous captures by default.
- Do not claim hardware verification until `logic_analyzer_scan` sees a device and at least one bounded capture succeeds.
