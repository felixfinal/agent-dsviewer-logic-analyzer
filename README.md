# OpenClaw Logic Analyzer

OpenClaw Logic Analyzer is a hardware-tooling package for using DSView,
DreamSourceLab DSLogic analyzers, and `sigrok-cli` from an agent workflow.

Chinese documentation: [README_zh.md](README_zh.md)

## What Is Included

- `dsview-logic`: a Python MCP server for DSView/sigrok logic-analyzer work.
- A standalone runner for local validation without starting MCP.
- OpenClaw skills for routing, measurement planning, capture, and decoding.
- Setup notes for DSView, DSLogic, sigrok, udev, and UART workflows.

## Capabilities

- Check whether DSView and `sigrok-cli` are installed.
- Scan connected logic analyzers with `sigrok-cli --scan`.
- Run bounded captures to CSV, VCD, or sigrok session files.
- List sigrok protocol decoders.
- Decode saved captures with generic sigrok decoders.
- Decode UART captures with normalized 8N1 defaults.
- Keep measurement judgment in skills while MCP stays as the stable tool layer.

## Repository Layout

```text
.
├── SKILL.md                         # agent installation/build/capability entry
├── README.md                        # English README
├── README_zh.md                     # Chinese README
├── server.py                        # standalone runner entrypoint
├── pyproject.toml                   # Python package metadata
├── run-dsview-logic-mcp.sh          # MCP stdio launcher
├── src/dsview_logic/                # MCP and runner source
├── skills/
│   ├── logic-analyzer-agent/        # top-level OpenClaw skill
│   └── dsview-logic-analyzer/       # execution workflow skill
├── docs/                            # architecture and setup notes
└── examples/                        # example payloads
```

## Dependencies

System dependencies:

- Linux with USB access to the logic analyzer.
- Python 3.11+.
- DSView GUI, usually installed as `DSView`.
- `sigrok-cli`.
- `sigrok-firmware-fx2lafw`.
- udev rule for DreamSourceLab DSLogic devices when non-root USB access is needed.

Example Debian/Ubuntu packages:

```bash
sudo apt update
sudo apt install -y python3 python3-venv python3-pip sigrok-cli sigrok-firmware-fx2lafw
```

Python setup:

```bash
python3 -m venv .venv
. .venv/bin/activate
pip install -e .
```

Build source and wheel distributions:

```bash
python3 -m pip install build
python3 -m build
```

## Quick Start

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

## MCP Usage

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

## OpenClaw Skills

Two skills are included under `skills/`:

- `logic-analyzer-agent`: top-level domain entry. It decides measurement plans,
  channel assumptions, sample-rate adequacy, and validation meaning.
- `dsview-logic-analyzer`: concrete DSView/sigrok workflow for setup, scan,
  bounded capture, saved waveform files, and protocol decoding.

Suggested installation:

```bash
mkdir -p ~/.openclaw/workspace/skills
cp -a skills/logic-analyzer-agent ~/.openclaw/workspace/skills/
cp -a skills/dsview-logic-analyzer ~/.openclaw/workspace/skills/
```

See [SKILL.md](SKILL.md) for the agent-facing installation, build, and
capability summary.

## Notes

- DSView is GUI-first. Scripted capture uses `sigrok-cli`.
- Keep captures bounded by sample count or time.
- Do not infer wiring from memory. Record the actual probe-to-channel mapping.
- For square waves, distinguish adjacent-edge interval from full high-to-high
  or low-to-low period.
- This repository does not vendor the DSView upstream source tree or firmware
  blobs. Install DSView and firmware from their official sources or OS packages.
