# OpenClaw Logic Analyzer / OpenClaw 逻辑分析仪能力包

OpenClaw Logic Analyzer is a small hardware-tooling package for using DSView,
DreamSourceLab DSLogic analyzers, and `sigrok-cli` from an agent workflow. It
contains:

- a Python MCP server: `dsview-logic`
- a command-line runner for local validation
- OpenClaw skills for routing, measurement planning, capture, and decoding
- practical notes for DSView / sigrok / UART workflows

OpenClaw Logic Analyzer 是一套面向 OpenClaw 智能体工作流的逻辑分析仪能力包，
用于把 DSView、DreamSourceLab DSLogic 和 `sigrok-cli` 接入可自动化的硬件验证流程。
本仓库包含：

- Python MCP 服务：`dsview-logic`
- 本地命令行 runner，便于不启动 MCP 时直接验证
- OpenClaw skill：负责任务路由、测量规划、采集和协议解码
- DSView / sigrok / UART 实测工作流说明

## What It Can Do / 能力范围

- Check whether DSView and `sigrok-cli` are installed.
- Scan connected logic analyzers with `sigrok-cli --scan`.
- Run bounded captures to CSV, VCD, or sigrok session files.
- List sigrok protocol decoders.
- Decode saved captures with generic sigrok decoders.
- Decode UART captures with normalized 8N1 defaults.
- Keep signal interpretation in skills while MCP stays as a stable tool layer.

- 检查 DSView 和 `sigrok-cli` 是否可用。
- 通过 `sigrok-cli --scan` 扫描已连接的逻辑分析仪。
- 执行有边界的采集，输出 CSV、VCD 或 sigrok session 文件。
- 列出 sigrok 支持的协议解码器。
- 对已保存的采集文件执行通用协议解码。
- 用默认 8N1 参数快速解码 UART。
- 将信号判断、通道规划、采样率判断留在 skill 层，MCP 只做稳定工具接口。

## Repository Layout / 仓库结构

```text
.
├── server.py                         # standalone entrypoint / 独立入口
├── pyproject.toml                    # Python package metadata / Python 包配置
├── run-dsview-logic-mcp.sh           # MCP stdio launcher / MCP 启动脚本
├── src/dsview_logic/                 # MCP and runner source / MCP 与 runner 源码
├── skills/
│   ├── logic-analyzer-agent/         # top-level OpenClaw skill / 顶层路由 skill
│   └── dsview-logic-analyzer/        # execution workflow skill / 执行层 skill
├── docs/                             # architecture and setup notes / 架构与安装说明
└── examples/                         # example payloads / 示例 payload
```

## Dependencies / 依赖

System dependencies:

- Linux with USB access to the logic analyzer
- Python 3.11+
- DSView GUI, usually installed as `DSView`
- `sigrok-cli`
- `sigrok-firmware-fx2lafw`
- udev rule for DreamSourceLab DSLogic devices when non-root USB access is needed

系统依赖：

- Linux，并且能访问逻辑分析仪 USB 设备
- Python 3.11+
- DSView GUI，通常命令名为 `DSView`
- `sigrok-cli`
- `sigrok-firmware-fx2lafw`
- 如需普通用户访问 DreamSourceLab DSLogic，需要配置 udev rule

Example Debian/Ubuntu packages:

```bash
sudo apt update
sudo apt install -y python3 python3-venv python3-pip sigrok-cli sigrok-firmware-fx2lafw
```

Python dependency:

```bash
python3 -m venv .venv
. .venv/bin/activate
pip install -e .
```

For building a wheel:

```bash
python3 -m pip install build
python3 -m build
```

构建 wheel：

```bash
python3 -m pip install build
python3 -m build
```

## Quick Start / 快速开始

List available runner tools:

```bash
python3 server.py --list-tools
```

Check local tool status:

```bash
python3 server.py logic_analyzer_status
```

Scan analyzers:

```bash
python3 server.py logic_analyzer_scan
```

Capture 1000 samples at 1 MHz to CSV:

```bash
python3 server.py logic_analyzer_capture --input '{
  "driver": "dreamsourcelab-dslogic",
  "channels": "0,1,2,3",
  "config": {"samplerate": "1m"},
  "samples": 1000,
  "output_format": "csv",
  "output_file": "gpio-smoke.csv"
}'
```

Decode UART from a saved sigrok session:

```bash
python3 server.py logic_analyzer_uart_decode_file --input '{
  "input_file": "deliverables/logic-analyzer/uart.sr",
  "channel": "0",
  "baudrate": 115200
}'
```

Output files default to `deliverables/logic-analyzer/`. Override with:

```bash
export DSVIEW_LOGIC_OUTPUT_DIR=/path/to/output
```

默认输出目录是 `deliverables/logic-analyzer/`。可用环境变量覆盖：

```bash
export DSVIEW_LOGIC_OUTPUT_DIR=/path/to/output
```

## MCP Usage / MCP 用法

Run as an MCP stdio server:

```bash
DSVIEW_LOGIC_MODE=mcp python3 server.py
```

Or after editable install:

```bash
dsview-logic-mcp
```

Available MCP tools:

- `logic_analyzer_status`
- `logic_analyzer_scan`
- `logic_analyzer_list_decoders`
- `logic_analyzer_capture`
- `logic_analyzer_decode_file`
- `logic_analyzer_uart_decode_file`
- `logic_analyzer_dsview_info`

## OpenClaw Skills / OpenClaw Skill

Two skills are included under `skills/`:

- `logic-analyzer-agent`: top-level domain entry. It decides measurement plans,
  channel assumptions, sample-rate adequacy, and validation meaning.
- `dsview-logic-analyzer`: concrete DSView/sigrok workflow for setup, scan,
  bounded capture, saved waveform files, and protocol decoding.

两个 skill 位于 `skills/`：

- `logic-analyzer-agent`：逻辑分析仪领域入口，负责测量规划、通道假设、采样率判断和结果解释。
- `dsview-logic-analyzer`：DSView/sigrok 执行层，负责环境检查、设备扫描、有边界采集、波形文件和协议解码。

Suggested installation:

```bash
mkdir -p ~/.openclaw/workspace/skills
cp -a skills/logic-analyzer-agent ~/.openclaw/workspace/skills/
cp -a skills/dsview-logic-analyzer ~/.openclaw/workspace/skills/
```

建议安装方式：

```bash
mkdir -p ~/.openclaw/workspace/skills
cp -a skills/logic-analyzer-agent ~/.openclaw/workspace/skills/
cp -a skills/dsview-logic-analyzer ~/.openclaw/workspace/skills/
```

## Notes / 注意事项

- DSView is GUI-first. Scripted capture uses `sigrok-cli`.
- Keep captures bounded by sample count or time.
- Do not infer wiring from memory. Record the actual probe-to-channel mapping.
- For square waves, distinguish adjacent-edge interval from full high-to-high
  or low-to-low period.
- This repository does not vendor the DSView upstream source tree or firmware
  blobs. Install DSView and firmware from their official sources or OS packages.

- DSView 主要是 GUI 工具，脚本化采集走 `sigrok-cli`。
- 默认只做有边界采集，避免无限持续采集。
- 不要凭记忆判断接线，必须记录实际探针和通道映射。
- 方波测量时要区分相邻边沿间隔和完整周期。
- 本仓库不内置 DSView 上游源码树或固件 blob；DSView 和固件请从官方来源或系统包安装。
