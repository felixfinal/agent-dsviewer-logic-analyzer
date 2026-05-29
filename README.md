# Agent DSViewer Logic Analyzer

Agent DSViewer Logic Analyzer is an agent-facing toolkit for using
DreamSourceLab DSLogic USB logic analyzers through the verified native
`dslogic-cli`/`libsigrok4DSL` backend in hardware validation workflows.

Chinese documentation: [README_zh.md](README_zh.md)

## What Is Included

- `dsview-logic`: a Python MCP server and standalone runner for native
  DSLogic/libsigrok4DSL logic-analyzer work.
- `native/dslogic-cli`: a C native backend that talks directly to DSView's
  `libsigrok4DSL.so` and is the intended path for DSLogic Plus high-rate
  modes such as `100MHz x3ch`.
- Agent skills for routing, measurement planning, capture, and decoding.
- Documentation for DSView, DSLogic, native capture modes, and protocol
  decoding.
- Example JSON payloads for capture and decode calls.

## Important Dependency Boundary

`dslogic-cli` is the native CLI code in this repository, but it is not a
standalone replacement for DSView's driver stack. It links against DSView's
`libsigrok4DSL.so` and uses DSView firmware/resource files at runtime.

Agents installing this project must prepare all of these pieces before capture:

- build/install DSView so `/usr/local/lib/libsigrok4DSL.so` exists
- copy DSView `res/` firmware/resources into `/opt/dslogic/res` or set
  `DSL_FW_DIR`
- build/install `native/dslogic-cli/src`
- confirm `python3 server.py logic_analyzer_status` returns `native_ok=true`

The vendored `libsigrok4DSL` headers under `native/dslogic-cli/third_party/`
are for compiling the CLI only; they are not the runtime library.

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

Check local DSView/libsigrok4DSL native status:

```bash
python3 server.py logic_analyzer_status
```

After the native backend is built and installed, run a DSLogic Plus 100 MHz x3
stream capture:

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
├── native/dslogic-cli/              # native DSLogic/libsigrok4DSL CLI backend
├── src/dsview_logic/                # MCP and runner source
├── skills/
│   ├── logic-analyzer-agent/        # top-level agent skill
│   └── dsview-logic-analyzer/       # DSView/libsigrok4DSL execution skill
├── docs/                            # architecture and setup notes
└── examples/                        # example payloads
```

## Capability Summary

The package implements:

- local DSView/libsigrok4DSL environment checks
- USB logic analyzer scanning
- bounded native DSLogic Plus captures through `dslogic-cli`, including stream
  mode `100MHz x3ch`
- native raw binary decode helpers for UART, SPI, and I2C
- DSView GUI information reporting
- agent skill guidance for sample-rate selection, capture modes, waveform
  interpretation, and safety boundaries

The detailed agent-facing capability reference is in [SKILL.md](SKILL.md).

## Licensing

This repository's Python MCP layer, agent skills, and top-level documentation
are licensed under Apache-2.0.

The native CLI under `native/dslogic-cli` carries its own license files:

- `native/dslogic-cli/LICENSE`: MIT for the CLI implementation.
- `native/dslogic-cli/third_party/libsigrok4DSL/LICENSE`: GPLv3 for the vendored
  `libsigrok4DSL` header subset extracted from DSView/libsigrok4DSL.

DSView/libsigrok4DSL, firmware blobs, and hardware vendor files are external
dependencies and remain under their own upstream licenses.
This repository does not vendor DSView upstream source trees or firmware blobs.
