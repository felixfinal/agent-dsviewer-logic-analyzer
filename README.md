# Agent DSViewer Logic Analyzer

Agent DSViewer Logic Analyzer packages agent-facing tools and skills for using
DSView, DreamSourceLab DSLogic analyzers, and `sigrok-cli` in hardware
validation workflows.

Chinese documentation: [README_zh.md](README_zh.md)

## What Is Included

- `dsview-logic`: a Python MCP server and standalone runner for DSView/sigrok
  logic-analyzer work.
- Agent skills for routing, measurement planning, capture, and decoding.
- Documentation for DSView, DSLogic, sigrok, capture modes, protocol decoding,
  and UART validation.
- Example JSON payloads for capture and decode calls.

## Quick Start

Install dependencies and the Python package:

```bash
python3 -m venv .venv
. .venv/bin/activate
pip install -e .
```

List available runner tools:

```bash
python3 server.py --list-tools
```

Check local DSView/sigrok status:

```bash
python3 server.py logic_analyzer_status
```

See [INSTALL.md](INSTALL.md) for system dependencies, installation, build, and
skill registration.

## Repository Layout

```text
.
├── SKILL.md                         # agent-facing capability description
├── INSTALL.md                       # dependencies, install, build, registration
├── LICENSE                          # Apache-2.0 for this repository's own code/docs
├── README.md                        # English README
├── README_zh.md                     # Chinese README
├── server.py                        # standalone runner entrypoint
├── pyproject.toml                   # Python package metadata
├── run-dsview-logic-mcp.sh          # MCP stdio launcher
├── src/dsview_logic/                # MCP and runner source
├── skills/
│   ├── logic-analyzer-agent/        # top-level agent skill
│   └── dsview-logic-analyzer/       # DSView/sigrok execution skill
├── docs/                            # architecture and setup notes
└── examples/                        # example payloads
```

## Capability Summary

The package implements:

- local DSView/sigrok environment checks
- USB logic analyzer scanning
- bounded captures to CSV, VCD, and sigrok session files
- sigrok protocol decoder discovery
- generic saved-capture protocol decoding
- normalized UART decode from saved captures
- DSView GUI information reporting
- agent skill guidance for sample-rate selection, capture modes, waveform
  interpretation, and safety boundaries

The detailed agent-facing capability reference is in [SKILL.md](SKILL.md).

## Licensing

This repository is licensed under Apache-2.0. The license applies to the code
and documentation in this repository.

DSView, sigrok, libsigrok, protocol decoders, firmware blobs, and hardware vendor
files are external dependencies and remain under their own upstream licenses.
This repository does not vendor DSView upstream source trees or firmware blobs.
