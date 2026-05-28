# Install And Build

This file contains dependencies, installation, build, and skill registration
instructions. Capability details live in [SKILL.md](SKILL.md).

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

## DSLogic udev Rule

DreamSourceLab development udev rule:

```bash
echo 'SUBSYSTEM=="usb", ATTRS{idVendor}=="2a0e", MODE="0666"' | sudo tee /etc/udev/rules.d/60-dreamsourcelab.rules
sudo udevadm control --reload-rules
sudo udevadm trigger
```

Reconnect the analyzer after changing udev rules.

## Python Install

From the repository root:

```bash
python3 -m venv .venv
. .venv/bin/activate
pip install -U pip setuptools wheel
pip install -e .
```

## Build

Build distributable artifacts:

```bash
pip install build
python3 -m build
```

Build without network isolation when dependencies are already installed locally:

```bash
python3 -m build --no-isolation
```

## Validate

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

For agent runtimes that use a filesystem skill directory, copy the included
skills into that runtime's skill root. Example:

```bash
export AGENT_SKILL_DIR="$HOME/.agent/skills"
mkdir -p "$AGENT_SKILL_DIR"
cp -a skills/logic-analyzer-agent "$AGENT_SKILL_DIR/"
cp -a skills/dsview-logic-analyzer "$AGENT_SKILL_DIR/"
```

Then run your runtime's local skill audit command if available.
