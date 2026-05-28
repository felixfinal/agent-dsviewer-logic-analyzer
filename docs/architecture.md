# Architecture / 架构说明

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

逻辑分析仪操作重复且工具化明显。MCP 负责稳定封装命令、输出路径、超时和解码参数；
agent 与 skill 保留工程判断，例如采样率是否足够、波形是否证明了问题、下一步该测哪里。
