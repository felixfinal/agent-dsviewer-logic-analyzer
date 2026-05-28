---
name: openclaw-logic-analyzer
description: Install and operate the OpenClaw DSView/sigrok logic-analyzer capability package. Use when an agent needs to install this repository, build the dsview-logic MCP server, register the included skills, or understand which logic-analyzer functions are implemented. For live measurement planning, use skills/logic-analyzer-agent; for concrete DSView/sigrok capture and decode workflows, use skills/dsview-logic-analyzer.
---

# OpenClaw Logic Analyzer

## Purpose

This repository packages the OpenClaw logic-analyzer tool layer:

- `dsview-logic` MCP server and standalone runner.
- `logic-analyzer-agent` top-level skill.
- `dsview-logic-analyzer` execution skill.
- Documentation and example payloads for DSView, DSLogic, sigrok, capture, and decode workflows.

## Implemented Functions

The Python tool layer implements these runner/MCP functions:

- `logic_analyzer_status`
  - Checks DSView path/version, `sigrok-cli` path/version, firmware directory, DreamSourceLab udev rule, and output directory.
- `logic_analyzer_scan`
  - Runs `sigrok-cli --scan` and returns discovered devices plus raw output.
- `logic_analyzer_list_decoders`
  - Lists sigrok protocol decoders and supports optional query filtering.
- `logic_analyzer_capture`
  - Runs bounded `sigrok-cli` captures with driver, channels, config, triggers, sample count or time, output file, and output format.
- `logic_analyzer_decode_file`
  - Runs a generic sigrok protocol decoder against a saved capture file.
- `logic_analyzer_uart_decode_file`
  - Decodes UART from saved `.sr`/srzip files with normalized baud/channel/options and returns reconstructed text plus raw annotations.
- `logic_analyzer_dsview_info`
  - Reports DSView GUI binary version/help and reminds agents that scripted capture should use `sigrok-cli`.

## System Dependencies

Required:

- Linux host with USB access to the analyzer.
- Python 3.11+.
- `sigrok-cli`.
- DSView GUI binary if GUI launch/version checks are needed.
- `sigrok-firmware-fx2lafw` or equivalent firmware files for supported FX2-based devices.
- Device udev rules when non-root USB access is needed.

Debian/Ubuntu baseline:

```bash
sudo apt update
sudo apt install -y python3 python3-venv python3-pip sigrok-cli sigrok-firmware-fx2lafw
```

DreamSourceLab development udev rule:

```bash
echo 'SUBSYSTEM=="usb", ATTRS{idVendor}=="2a0e", MODE="0666"' | sudo tee /etc/udev/rules.d/60-dreamsourcelab.rules
sudo udevadm control --reload-rules
sudo udevadm trigger
```

Reconnect the analyzer after changing udev rules.

## Build And Install

From the repository root:

```bash
python3 -m venv .venv
. .venv/bin/activate
pip install -U pip setuptools wheel
pip install -e .
```

Build distributable artifacts:

```bash
pip install build
python3 -m build
```

Run a minimal source validation:

```bash
python3 -m compileall src server.py
python3 server.py --list-tools
python3 server.py logic_analyzer_status
```

## Runner Usage

List tools:

```bash
python3 server.py --list-tools
```

Run status:

```bash
python3 server.py logic_analyzer_status
```

Scan devices:

```bash
python3 server.py logic_analyzer_scan
```

Run a short CSV capture:

```bash
python3 server.py logic_analyzer_capture --input "$(cat examples/capture-gpio-csv.json)"
```

Decode UART:

```bash
python3 server.py logic_analyzer_uart_decode_file --input "$(cat examples/decode-uart.json)"
```

## MCP Usage

Run the MCP stdio server from source:

```bash
DSVIEW_LOGIC_MODE=mcp python3 server.py
```

Run the packaged entry point after install:

```bash
dsview-logic-mcp
```

The helper launcher uses `.venv` by default:

```bash
./run-dsview-logic-mcp.sh
```

Override the venv path:

```bash
DSVIEW_LOGIC_VENV=/path/to/venv ./run-dsview-logic-mcp.sh
```

## Output Paths

Default capture output:

```text
deliverables/logic-analyzer/
```

Override output directory:

```bash
export DSVIEW_LOGIC_OUTPUT_DIR=/path/to/output
```

Override workspace root:

```bash
export DSVIEW_LOGIC_WORKSPACE=/path/to/workspace
```

## Install Included Skills

For OpenClaw workspace-style skill installation:

```bash
mkdir -p ~/.openclaw/workspace/skills
cp -a skills/logic-analyzer-agent ~/.openclaw/workspace/skills/
cp -a skills/dsview-logic-analyzer ~/.openclaw/workspace/skills/
```

Then run the local skill audit command if available:

```bash
openclaw skills check
```

## Agent Routing

- Start with `logic-analyzer-agent` when the task asks for hardware validation,
  signal interpretation, channel mapping, sampling strategy, or protocol-level proof.
- Route concrete DSView/sigrok actions to `dsview-logic-analyzer`.
- Prefer MCP tools for scan, status, capture, and decode when the MCP server is available.
- Use the standalone runner as fallback when MCP is not loaded in the current agent session.

## Safety Boundaries

- Keep captures bounded by samples or time unless the user explicitly requests a continuous capture.
- Do not assume physical probe wiring. Record the channel-to-signal mapping.
- Do not claim hardware validation until a device scan and at least one bounded capture succeed.
- Do not vendor or redistribute upstream DSView source trees or firmware blobs from this repository.
