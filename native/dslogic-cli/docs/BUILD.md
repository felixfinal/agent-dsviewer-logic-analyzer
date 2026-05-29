# Build Guide

## Project Layout

```
dslogic-cli/
├── README.md
├── LICENSE              ← MIT (CLI only)
├── CHANGELOG.md
├── .gitignore
├── src/
│   ├── dslogic-cli.c    ← canonical source (C99, ~560 lines)
│   ├── dslogic-cli       ← build artifact (git-ignored)
│   └── Makefile
├── third_party/
│   └── libsigrok4DSL/   ← vendored headers (GPLv3)
│       ├── LICENSE       ← GPLv3 (applies to library, not CLI)
│       ├── libsigrok.h
│       └── ...
├── docs/
│   ├── BUILD.md
│   └── USAGE.md
└── examples/
```

## Dependencies

`dslogic-cli` depends on DSView build artifacts. The vendored headers in
`third_party/libsigrok4DSL` are compile-time headers only; they are not enough
for runtime operation.

### Build
| Dependency | Package |
|-----------|---------|
| `gcc` | `build-essential` |
| `glib-2.0` headers | `libglib2.0-dev` |
| `pkg-config` | `pkg-config` |

### Runtime
| Dependency | Path | Source |
|-----------|------|--------|
| `libsigrok4DSL.so` | `/usr/local/lib/` | Built from DSView |
| `glib-2.0` | system | `apt install libglib2.0-dev` |
| FPGA firmware | `$DSL_FW_DIR` or `/opt/dslogic/res/` | DSView `res/` directory |

Before using the CLI from another agent, verify:

```bash
test -f /usr/local/lib/libsigrok4DSL.so
test -d "${DSL_FW_DIR:-/opt/dslogic/res}"
dslogic-cli --version
```

## Building libsigrok4DSL.so

```bash
# Clone and build DSView
git clone https://github.com/DreamSourceLab/DSView.git
cd DSView
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
make -j4
sudo make install
```

This installs both `libsigrok4DSL.so` and headers to `/usr/local/`.

## Build & Install CLI

```bash
cd src

# Build (zero warnings)
make

# Install to /usr/local/bin/
sudo make install

# Verify
dslogic-cli --version
# → {"version":"3.2.0"}

# Clean build artifacts
make clean
```

### Manual Compile (no Makefile)

```bash
gcc -Wall -Wextra -O2 \
    -DVERSION='"3.2.0"' \
    $(pkg-config --cflags glib-2.0) \
    -I/usr/local/include/libsigrok4DSL \
    -o dslogic-cli dslogic-cli.c \
    -L/usr/local/lib -lsigrok4DSL -Wl,-rpath,/usr/local/lib \
    $(pkg-config --libs glib-2.0)
```

## Install Paths

| File | Path |
|------|------|
| CLI binary | `/usr/local/bin/dslogic-cli` |
| Shared library | `/usr/local/lib/libsigrok4DSL.so` |
| Headers | `/usr/local/include/libsigrok4DSL/` |
| Firmware (default) | `/opt/dslogic/res/` |

## Firmware Setup

```bash
# Option A: Copy from DSView source
cp -r /path/to/DSView/res/*.bin /opt/dslogic/res/
cp -r /path/to/DSView/res/*.fw  /opt/dslogic/res/
cp -r /path/to/DSView/res/*.dsc /opt/dslogic/res/

# Option B: Custom path via environment variable
export DSL_FW_DIR=/path/to/custom/firmware
dslogic-cli scan
```

## Rebuilding After Source Changes

```bash
cd src && make clean && make && sudo make install
```

No shared library rebuild needed unless `libsigrok4DSL` itself changed.
