# Agent DSViewer 逻辑分析仪能力包

Agent DSViewer Logic Analyzer 是一套面向 agent 工作流的逻辑分析仪能力包，
用于把 DSView、DreamSourceLab DSLogic 和 `sigrok-cli` 接入硬件验证流程。

English documentation: [README.md](README.md)

## 包含内容

- `dsview-logic`：用于 DSView/sigrok 逻辑分析仪工作的 Python MCP 服务和本地 runner。
- Agent skill：负责任务路由、测量规划、采集和协议解码。
- DSView、DSLogic、sigrok、采集模式、协议解码、UART 验证说明。
- 用于采集和解码调用的 JSON 示例。

## 快速开始

安装 Python 包：

```bash
python3 -m venv .venv
. .venv/bin/activate
pip install -e .
```

列出 runner 工具：

```bash
python3 server.py --list-tools
```

检查本机 DSView/sigrok 状态：

```bash
python3 server.py logic_analyzer_status
```

系统依赖、安装、构建和 skill 注册见 [INSTALL.md](INSTALL.md)。

## 仓库结构

```text
.
├── SKILL.md                         # 面向 agent 的能力说明
├── INSTALL.md                       # 依赖、安装、构建、注册说明
├── LICENSE                          # 本仓库自有代码/文档使用 Apache-2.0
├── README.md                        # 英文 README
├── README_zh.md                     # 中文 README
├── server.py                        # 独立 runner 入口
├── pyproject.toml                   # Python 包配置
├── run-dsview-logic-mcp.sh          # MCP stdio 启动脚本
├── src/dsview_logic/                # MCP 与 runner 源码
├── skills/
│   ├── logic-analyzer-agent/        # 顶层 agent skill
│   └── dsview-logic-analyzer/       # DSView/sigrok 执行层 skill
├── docs/                            # 架构与安装说明
└── examples/                        # 示例 payload
```

## 能力摘要

本包已实现：

- 本机 DSView/sigrok 环境检查
- USB 逻辑分析仪扫描
- 有边界采集，输出 CSV、VCD 和 sigrok session 文件
- sigrok 协议解码器发现
- 对已保存采集文件执行通用协议解码
- 对已保存采集文件执行规范化 UART 解码
- DSView GUI 信息读取
- 面向 agent 的采样率选择、采集模式、波形解释和安全边界说明

详细能力说明见 [SKILL.md](SKILL.md)。

## 许可证

本仓库使用 Apache-2.0。该许可证覆盖本仓库内自有代码和文档。

DSView、sigrok、libsigrok、协议解码器、固件 blob、硬件厂商文件都是外部依赖，
仍遵循各自上游许可证。本仓库不内置 DSView 上游源码树或固件 blob。
