# DSLogic Setup Notes / DSLogic 安装说明

## System Packages

On Debian/Ubuntu:

```bash
sudo apt update
sudo apt install -y build-essential pkg-config libglib2.0-dev cmake git
```

Install DSView separately if your distribution does not package it.

## udev Rule

For DreamSourceLab USB devices, a permissive local development rule can be:

```udev
SUBSYSTEM=="usb", ATTRS{idVendor}=="2a0e", MODE="0666"
```

Install it as:

```bash
echo 'SUBSYSTEM=="usb", ATTRS{idVendor}=="2a0e", MODE="0666"' | sudo tee /etc/udev/rules.d/60-dreamsourcelab.rules
sudo udevadm control --reload-rules
sudo udevadm trigger
```

Unplug and reconnect the analyzer after changing rules.

## Native Backend Setup

Build and install DSView's native library:

```bash
git clone https://github.com/DreamSourceLab/DSView.git
cd DSView
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
make -j4
sudo make install
sudo ldconfig
```

Install firmware resources for `dslogic-cli`:

```bash
sudo mkdir -p /opt/dslogic/res
sudo cp /path/to/DSView/DSView/res/DSLogic*.bin /opt/dslogic/res/
sudo cp /path/to/DSView/DSView/res/DSLogic*.fw /opt/dslogic/res/
sudo cp /path/to/DSView/DSView/res/DSLogic*.dsc /opt/dslogic/res/
```

Build the native CLI included in this repository:

```bash
cd native/dslogic-cli/src
make
sudo make install
```

The 100 MHz x3 capture mode is:

```text
open 1
stream 1
channel_mode 3
samplerate 100000000
capture 0 /tmp/dslogic-100m-x3.bin 15
```

## Capture Advice

- Start with low-risk short captures, for example 1 MHz and 1000 samples.
- For high sample rates, reduce enabled channels according to the analyzer
  mode.
- For DSLogic Plus `100MHz x3ch`, use native `dslogic-cli`.
- Native output is raw `.bin`; use `scripts/capture_verify.py` and
  `logic_analyzer_native_decode_file` for validation/decode.
- For UART, capture TX first, then decode with the native UART helper.
