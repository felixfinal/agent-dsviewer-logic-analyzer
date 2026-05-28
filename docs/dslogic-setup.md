# DSLogic Setup Notes / DSLogic 安装说明

## System Packages

On Debian/Ubuntu:

```bash
sudo apt update
sudo apt install -y sigrok-cli sigrok-firmware-fx2lafw
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

## Known Local Baseline

The original local validation used:

- DSView 1.3.2
- Debian `sigrok-cli` 0.7.2
- DreamSourceLab DSLogic Plus / DSLogic Pro detected by sigrok after firmware load
- HPM5300EVK GPIO and UART2 TX validation at low sample rates

## Capture Advice

- Start with low-risk short captures, for example 1 MHz and 1000 samples.
- For high sample rates, reduce enabled channels according to the analyzer mode.
- Save CSV for script analysis and `.sr`/srzip for decoder workflows.
- For UART, capture TX first, then decode with the normalized UART helper.
