# OpenClaw 逻辑分析仪能力包

OpenClaw Logic Analyzer 是一套面向 OpenClaw 智能体工作流的逻辑分析仪能力包，
用于把 DSView、DreamSourceLab DSLogic 和 `sigrok-cli` 接入可自动化的硬件验证流程。

English documentation: [README.md](README.md)

## 包含内容

- `dsview-logic`：用于 DSView/sigrok 逻辑分析仪工作的 Python MCP 服务。
- 本地命令行 runner：不启动 MCP 时也能直接验证。
- OpenClaw skill：负责任务路由、测量规划、采集和协议解码。
- DSView、DSLogic、sigrok、udev、UART 工作流说明。

## 实现能力

- 检查 DSView 和 `sigrok-cli` 是否可用。
- 通过 `sigrok-cli --scan` 扫描已连接的逻辑分析仪。
- 执行有边界的采集，输出 CSV、VCD 或 sigrok session 文件。
- 列出 sigrok 支持的协议解码器。
- 对已保存的采集文件执行通用协议解码。
- 用默认 8N1 参数快速解码 UART。
- 将信号判断、通道规划、采样率判断留在 skill 层，MCP 只做稳定工具接口。

## 仓库结构

```text
.
├── SKILL.md                         # agent 安装、编译、能力说明入口
├── README.md                        # 英文 README
├── README_zh.md                     # 中文 README
├── server.py                        # 独立 runner 入口
├── pyproject.toml                   # Python 包配置
├── run-dsview-logic-mcp.sh          # MCP stdio 启动脚本
├── src/dsview_logic/                # MCP 与 runner 源码
├── skills/
│   ├── logic-analyzer-agent/        # 顶层 OpenClaw skill
│   └── dsview-logic-analyzer/       # 执行层 workflow skill
├── docs/                            # 架构与安装说明
└── examples/                        # 示例 payload
```

## 依赖

系统依赖：

- Linux，并且能访问逻辑分析仪 USB 设备。
- Python 3.11+。
- DSView GUI，通常命令名为 `DSView`。
- `sigrok-cli`。
- `sigrok-firmware-fx2lafw`。
- 如需普通用户访问 DreamSourceLab DSLogic，需要配置 udev rule。

Debian/Ubuntu 示例：

```bash
sudo apt update
sudo apt install -y python3 python3-venv python3-pip sigrok-cli sigrok-firmware-fx2lafw
```

Python 环境：

```bash
python3 -m venv .venv
. .venv/bin/activate
pip install -e .
```

构建源码包和 wheel：

```bash
python3 -m pip install build
python3 -m build
```

## 快速开始

列出 runner 工具：

```bash
python3 server.py --list-tools
```

检查本机工具状态：

```bash
python3 server.py logic_analyzer_status
```

扫描逻辑分析仪：

```bash
python3 server.py logic_analyzer_scan
```

以 1 MHz 采集 1000 个样本并保存为 CSV：

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

从已保存的 sigrok session 解码 UART：

```bash
python3 server.py logic_analyzer_uart_decode_file --input '{
  "input_file": "deliverables/logic-analyzer/uart.sr",
  "channel": "0",
  "baudrate": 115200
}'
```

默认输出目录是 `deliverables/logic-analyzer/`。可用环境变量覆盖：

```bash
export DSVIEW_LOGIC_OUTPUT_DIR=/path/to/output
```

## MCP 用法

作为 MCP stdio 服务运行：

```bash
DSVIEW_LOGIC_MODE=mcp python3 server.py
```

editable install 后也可直接运行：

```bash
dsview-logic-mcp
```

可用 MCP 工具：

- `logic_analyzer_status`
- `logic_analyzer_scan`
- `logic_analyzer_list_decoders`
- `logic_analyzer_capture`
- `logic_analyzer_decode_file`
- `logic_analyzer_uart_decode_file`
- `logic_analyzer_dsview_info`

## OpenClaw Skill

`skills/` 下包含两个 skill：

- `logic-analyzer-agent`：逻辑分析仪领域入口，负责测量规划、通道假设、采样率判断和结果解释。
- `dsview-logic-analyzer`：DSView/sigrok 执行层，负责环境检查、设备扫描、有边界采集、波形文件和协议解码。

建议安装：

```bash
mkdir -p ~/.openclaw/workspace/skills
cp -a skills/logic-analyzer-agent ~/.openclaw/workspace/skills/
cp -a skills/dsview-logic-analyzer ~/.openclaw/workspace/skills/
```

给 agent 使用的安装、编译和能力说明见 [SKILL.md](SKILL.md)。

## 注意事项

- DSView 主要是 GUI 工具，脚本化采集走 `sigrok-cli`。
- 默认只做有边界采集，避免无限持续采集。
- 不要凭记忆判断接线，必须记录实际探针和通道映射。
- 方波测量时要区分相邻边沿间隔和完整周期。
- 本仓库不内置 DSView 上游源码树或固件 blob；DSView 和固件请从官方来源或系统包安装。
