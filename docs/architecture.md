# Architecture

## Layering

```text
logic-analyzer-agent
  -> dsview-logic-analyzer
      -> dsview-logic MCP
          -> sigrok-cli / DSView / USB logic analyzer
```

## Responsibilities

- `logic-analyzer-agent`
  - top-level routing
  - measurement plan
  - channel mapping assumptions
  - sample-rate judgment
  - waveform and protocol validation meaning

- `dsview-logic-analyzer`
  - local DSView/sigrok workflow
  - setup checks
  - bounded capture guidance
  - decode workflow guidance
  - local hardware baseline notes

- `dsview-logic` MCP
  - stable tool calls
  - `sigrok-cli --scan`
  - bounded capture command construction
  - decoder listing
  - generic and UART decode wrappers

## Why MCP

Logic-analyzer operations are repetitive and tool-shaped. MCP keeps command
construction, output paths, timeout handling, and decoder options in a stable
interface. The agent and skills stay focused on engineering judgment.
