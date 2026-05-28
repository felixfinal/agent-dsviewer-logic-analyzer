# Local DSView / sigrok baseline

Harvested from `hermers_pi_deepseek` on 2026-05-19 and mirrored locally.

Remote baseline:
- `DSView 1.3.2` at `/usr/local/bin/DSView`.
- `sigrok-cli 0.7.2` from Debian trixie.
- `sigrok-firmware-fx2lafw` installed.
- DreamSourceLab udev rule: `SUBSYSTEM=="usb", ATTRS{idVendor}=="2a0e", MODE="0666"`.

Local baseline after整理:
- DSView binary copied to `/usr/local/bin/DSView`.
- `sigrok-cli` and `sigrok-firmware-fx2lafw` installed from apt.
- DreamSourceLab udev rule installed at `/etc/udev/rules.d/60-dreamsourcelab.rules`.
- OpenClaw MCP server files live under `.openclaw/mcp/servers/dsview-logic/`.
