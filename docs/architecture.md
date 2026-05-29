# Architecture

## Layering

```text
logic-analyzer-agent
  -> dsview-logic-analyzer
      -> dsview-logic MCP
          -> dslogic-cli / libsigrok4DSL / DSView / USB logic analyzer
```

## Responsibilities

- `logic-analyzer-agent`
  - top-level routing
  - measurement plan
  - channel mapping assumptions
  - sample-rate judgment
  - waveform and protocol validation meaning

- `dsview-logic-analyzer`
  - local DSView/libsigrok4DSL native workflow
  - setup checks
  - bounded capture guidance
  - decode workflow guidance
  - local hardware baseline notes

- `dsview-logic` MCP
  - stable tool calls
  - native `dslogic-cli` scan/capture/decode wrappers
  - bounded raw binary capture command construction
  - native UART/SPI/I2C decoder listing

- `native/dslogic-cli`
  - C backend for DreamSourceLab DSLogic hardware
  - links to DSView's `libsigrok4DSL.so`
  - exposes DSLogic Plus stream/buffer modes including stream `100MHz x3ch`

## Why MCP

Logic-analyzer operations are repetitive and tool-shaped. MCP keeps command
construction, output paths, timeout handling, and decoder options in a stable
interface. The agent and skills stay focused on engineering judgment.

## Backend Rule

Use `dslogic-cli` for all scripted scan/capture/decode workflows. The default
high-rate profile is:

```text
stream 1
channel_mode 3
samplerate 100000000
```

That path outputs raw binary logic data and should be decoded with
`logic_analyzer_native_decode_file` or external tooling that understands the
native capture packing.
