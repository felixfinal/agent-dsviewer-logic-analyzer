# Install And Build

This file contains dependencies, installation, build, and skill registration
instructions. Capability details live in [SKILL.md](SKILL.md).

## System Dependencies

Required:

- Linux host with USB access to the analyzer.
- Python 3.11+.
- `dslogic-cli` plus `libsigrok4DSL.so`.
- DSView GUI binary if GUI launch/version checks are needed.
- DSView firmware resources under `/opt/dslogic/res` or `DSL_FW_DIR`.
- Device udev rules when non-root USB access is needed.

Debian/Ubuntu baseline:

```bash
sudo apt update
sudo apt install -y python3 python3-venv python3-pip build-essential pkg-config libglib2.0-dev cmake git
```

## Important Native Dependency Boundary

`dslogic-cli` is not a standalone logic-analyzer stack. It is a native command
line frontend for DSView's `libsigrok4DSL.so`, using the same DSLogic driver
path as DSView.

When another agent installs this project, it must not stop at copying/building
`native/dslogic-cli`. A working installation also needs:

- DSView built and installed so `/usr/local/lib/libsigrok4DSL.so` is present.
- DSView firmware/resource files copied from `DSView/res/` into
  `/opt/dslogic/res`, or `DSL_FW_DIR` pointing at an equivalent directory.
- The installed `libsigrok4DSL.so` ABI to match the headers used by
  `native/dslogic-cli/third_party/libsigrok4DSL` closely enough for the CLI.
- `python3 server.py logic_analyzer_status` returning `native_ok=true`.

If `native_ok=false`, fix the DSView/libsigrok4DSL install. Do not fall back to
`sigrok-cli`; it is intentionally not part of this workflow.

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

## Native DSLogic Backend

The native backend is the only supported execution path for DSLogic scripted
scan, capture, and decode in this repository.

Build and install DSView's `libsigrok4DSL.so` first:

```bash
git clone https://github.com/DreamSourceLab/DSView.git
cd DSView
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
make -j4
sudo make install
sudo ldconfig
```

Install DSView firmware resources for the native backend. The default native
firmware path is `/opt/dslogic/res`; alternatively set `DSL_FW_DIR`.

```bash
sudo mkdir -p /opt/dslogic/res
sudo cp /path/to/DSView/DSView/res/DSLogic*.bin /opt/dslogic/res/
sudo cp /path/to/DSView/DSView/res/DSLogic*.fw /opt/dslogic/res/
sudo cp /path/to/DSView/DSView/res/DSLogic*.dsc /opt/dslogic/res/
```

Build and install the native CLI from this repository:

```bash
cd native/dslogic-cli/src
make
sudo make install
dslogic-cli --version
```

If the binary is not on `PATH`, point the MCP layer at it:

```bash
export DSVIEW_LOGIC_NATIVE_CLI=/path/to/dslogic-cli
```

Native smoke tests:

```bash
python3 server.py logic_analyzer_status
python3 server.py logic_analyzer_native_scan
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

Run a native high-rate DSLogic Plus capture:

```bash
python3 server.py logic_analyzer_native_capture --input '{
  "channel_mode": 3,
  "samplerate": 100000000,
  "output_file": "native-100m-x3.bin",
  "timeout": 15
}'
```

Decode UART:

```bash
python3 server.py logic_analyzer_native_decode_file --input "$(cat examples/native-decode-uart-100m.json)"
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
