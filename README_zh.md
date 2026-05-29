# Agent DSViewer 逻辑分析仪能力包

Agent DSViewer Logic Analyzer 是一套面向 agent 工作流的逻辑分析仪能力包，
用于把 DreamSourceLab DSLogic USB 逻辑分析仪通过已验证的 native
`dslogic-cli` / `libsigrok4DSL` 后端接入硬件验证流程。

English documentation: [README.md](README.md)

## 包含内容

- `dsview-logic`：用于 DSView/libsigrok4DSL native 逻辑分析仪工作的 Python MCP 服务和本地 runner。
- `native/dslogic-cli`：C native 后端，直接调用 DSView 的 `libsigrok4DSL.so`，
  用于 DSLogic Plus 的 `100MHz x3ch` 等高速模式。
- Agent skill：负责任务路由、测量规划、采集和协议解码。
- DSView、DSLogic、native 采集模式、协议解码说明。
- 用于采集和解码调用的 JSON 示例。

## 重要依赖边界

`dslogic-cli` 是本仓库中的 native CLI 代码，但它不是一个完全独立的逻辑分析仪
驱动栈。它运行时链接 DSView 的 `libsigrok4DSL.so`，并依赖 DSView 的
firmware/resource 文件。

其它 agent 安装本项目时，必须先准备完整 native 依赖：

- 编译/安装 DSView，确保 `/usr/local/lib/libsigrok4DSL.so` 存在
- 将 DSView `res/` 下的 firmware/resource 复制到 `/opt/dslogic/res`，或设置
  `DSL_FW_DIR`
- 编译/安装 `native/dslogic-cli/src`
- 确认 `python3 server.py logic_analyzer_status` 返回 `native_ok=true`

`native/dslogic-cli/third_party/` 下 vendored 的 `libsigrok4DSL` headers
只用于编译 CLI，不是运行时库。

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

检查本机 DSView/libsigrok4DSL native 状态：

```bash
python3 server.py logic_analyzer_status
```

native 后端安装完成后，可执行 DSLogic Plus 100MHz x3 stream 采集：

```bash
python3 server.py logic_analyzer_native_capture --input '{
  "device_index": 1,
  "stream": 1,
  "channel_mode": 3,
  "samplerate": 100000000,
  "duration": 0,
  "output_file": "dslogic-100m-x3.bin",
  "timeout": 15
}'
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
├── native/dslogic-cli/              # DSLogic/libsigrok4DSL native CLI 后端
├── src/dsview_logic/                # MCP 与 runner 源码
├── skills/
│   ├── logic-analyzer-agent/        # 顶层 agent skill
│   └── dsview-logic-analyzer/       # DSView/libsigrok4DSL 执行层 skill
├── docs/                            # 架构与安装说明
└── examples/                        # 示例 payload
```

## 能力摘要

本包已实现：

- 本机 DSView/libsigrok4DSL 环境检查
- USB 逻辑分析仪扫描
- 通过 `dslogic-cli` 执行有边界的 DSLogic Plus native 采集，包括 stream
  `100MHz x3ch`
- 对 native raw binary 采集文件执行 UART、SPI、I2C 解码
- DSView GUI 信息读取
- 面向 agent 的采样率选择、采集模式、波形解释和安全边界说明

详细能力说明见 [SKILL.md](SKILL.md)。

## 许可证

本仓库的 Python MCP 层、agent skills 和顶层文档使用 Apache-2.0。

`native/dslogic-cli` 目录保留自己的许可证：

- `native/dslogic-cli/LICENSE`：CLI 实现使用 MIT。
- `native/dslogic-cli/third_party/libsigrok4DSL/LICENSE`：从 DSView/libsigrok4DSL
  提取的头文件子集使用 GPLv3。

DSView/libsigrok4DSL、固件 blob、硬件厂商文件都是外部依赖，
仍遵循各自上游许可证。本仓库不内置 DSView 上游源码树或固件 blob。
