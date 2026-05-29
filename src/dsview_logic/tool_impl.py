from __future__ import annotations

import os
import shutil
import subprocess
import time
from pathlib import Path
from typing import Any

from .config import DEFAULT_OUTPUT_DIR


def _run(argv: list[str], timeout: int = 30, input_text: str | None = None) -> dict[str, Any]:
    try:
        proc = subprocess.run(
            argv,
            text=True,
            input=input_text,
            capture_output=True,
            timeout=timeout,
            check=False,
        )
        return {
            "ok": proc.returncode == 0,
            "returncode": proc.returncode,
            "stdout": proc.stdout,
            "stderr": proc.stderr,
            "command": argv,
        }
    except FileNotFoundError as exc:
        return {"ok": False, "returncode": 127, "stdout": "", "stderr": str(exc), "command": argv}
    except subprocess.TimeoutExpired as exc:
        return {
            "ok": False,
            "returncode": None,
            "stdout": exc.stdout or "",
            "stderr": exc.stderr or f"timeout after {timeout}s",
            "command": argv,
            "timeout": timeout,
        }


def _which(name: str) -> str | None:
    return shutil.which(name)


def _native_cli_path() -> str | None:
    configured = os.environ.get("DSVIEW_LOGIC_NATIVE_CLI")
    if configured:
        path = Path(configured).expanduser()
        if path.exists() and os.access(path, os.X_OK):
            return str(path)
    return _which("dslogic-cli")


def _safe_output_path(path_value: str | None, suffix: str) -> Path:
    DEFAULT_OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    if path_value:
        path = Path(path_value).expanduser()
        if not path.is_absolute():
            path = DEFAULT_OUTPUT_DIR / path
    else:
        stamp = time.strftime("%Y%m%d-%H%M%S")
        path = DEFAULT_OUTPUT_DIR / f"capture-{stamp}.{suffix}"
    path.parent.mkdir(parents=True, exist_ok=True)
    return path


def logic_analyzer_status(payload: dict[str, Any] | None = None) -> dict[str, Any]:
    payload = payload or {}
    dsview = _which("DSView") or _which("dsview")
    native = _native_cli_path()
    libsigrok4dsl = Path("/usr/local/lib/libsigrok4DSL.so").exists()
    result: dict[str, Any] = {
        "dsview_path": dsview,
        "native_dslogic_cli_path": native,
        "native_libsigrok4dsl_exists": libsigrok4dsl,
        "native_firmware_dir_exists": Path(os.environ.get("DSL_FW_DIR", "/opt/dslogic/res")).exists(),
        "dreamsourcelab_udev_rule": Path("/etc/udev/rules.d/60-dreamsourcelab.rules").exists(),
        "output_dir": str(DEFAULT_OUTPUT_DIR),
    }
    if dsview:
        r = _run([dsview, "--version"], timeout=10)
        result["dsview_version"] = (r["stdout"] + r["stderr"]).strip()
    if native:
        r = _run([native, "--version"], timeout=10)
        result["native_dslogic_version"] = (r["stdout"] + r["stderr"]).strip()
    result["native_ok"] = bool(native and libsigrok4dsl)
    result["gui_ok"] = bool(dsview)
    result["ok"] = bool(result["native_ok"])
    return result


def logic_analyzer_scan(payload: dict[str, Any] | None = None) -> dict[str, Any]:
    """Compatibility alias for native DSLogic/libsigrok4DSL scan."""
    return logic_analyzer_native_scan(payload or {})


def logic_analyzer_native_scan(payload: dict[str, Any] | None = None) -> dict[str, Any]:
    payload = payload or {}
    native = _native_cli_path()
    if not native:
        return {"ok": False, "error": "dslogic-cli not found"}
    r = _run([native, "scan"], timeout=int(payload.get("timeout", 20)))
    return {
        "ok": r["ok"],
        "stdout": r["stdout"],
        "stderr": r["stderr"],
        "command": r["command"],
        "returncode": r["returncode"],
    }


def logic_analyzer_list_decoders(payload: dict[str, Any] | None = None) -> dict[str, Any]:
    payload = payload or {}
    decoders = ["uart", "spi", "i2c"]
    query = str(payload.get("query", "")).lower().strip()
    if query:
        decoders = [d for d in decoders if query in d.lower()]
    return {
        "ok": True,
        "backend": "dslogic-cli",
        "decoders": decoders,
        "count": len(decoders),
        "note": "Native dslogic-cli currently provides pure-C UART/SPI/I2C decoders.",
    }


def logic_analyzer_capture(payload: dict[str, Any]) -> dict[str, Any]:
    """Compatibility alias for native DSLogic/libsigrok4DSL capture."""
    return logic_analyzer_native_capture(payload)


def logic_analyzer_native_capture(payload: dict[str, Any]) -> dict[str, Any]:
    """Capture with the native DSLogic/libsigrok4DSL backend.

    This is the backend required for DSLogic Plus high-rate modes such as
    100 MHz x 3 channels. It shells out to dslogic-cli run and writes raw binary
    logic samples.
    """
    native = _native_cli_path()
    if not native:
        return {"ok": False, "error": "dslogic-cli not found"}

    output = _safe_output_path(payload.get("output_file"), "bin")
    device_index = int(payload.get("device_index", 1))
    stream = int(payload.get("stream", 1))
    channel_mode = int(payload.get("channel_mode", 3))
    samplerate = int(payload.get("samplerate", 100_000_000))
    duration = payload.get("duration", payload.get("duration_sec", 0))
    timeout = int(payload.get("timeout", 30))

    commands = [
        f"open {device_index}",
        f"stream {stream}",
        f"channel_mode {channel_mode}",
        f"samplerate {samplerate}",
    ]
    if payload.get("vth"):
        commands.append(f"vth {payload['vth']}")
    for channel in payload.get("disable_channels", []):
        commands.append(f"enable {int(channel)} 0")
    for channel in payload.get("enable_channels", []):
        commands.append(f"enable {int(channel)} 1")
    if payload.get("trigger_reset", False):
        commands.append("trigger reset")
    if payload.get("trigger"):
        commands.extend(str(payload["trigger"]).splitlines())
    commands.append(f"capture {duration} {output} {timeout}")
    input_text = "\n".join(commands) + "\n"
    r = _run([native, "run"], timeout=timeout + 5, input_text=input_text)
    return {
        "ok": r["ok"] and output.exists() and output.stat().st_size > 0,
        "backend": "dslogic-cli",
        "mode": {
            "stream": stream,
            "channel_mode": channel_mode,
            "samplerate": samplerate,
        },
        "output_file": str(output),
        "size": output.stat().st_size if output.exists() else 0,
        "run_script": input_text,
        "command": r["command"],
        "returncode": r["returncode"],
        "stdout": r["stdout"],
        "stderr": r["stderr"],
    }


def logic_analyzer_decode_file(payload: dict[str, Any]) -> dict[str, Any]:
    """Compatibility alias for native raw binary decode."""
    return logic_analyzer_native_decode_file(payload)


def logic_analyzer_uart_decode_file(payload: dict[str, Any]) -> dict[str, Any]:
    """Compatibility alias for native UART decode."""
    native_payload = dict(payload)
    native_payload["decoder"] = "uart"
    if "rx" in native_payload and "channel" not in native_payload:
        native_payload["channel"] = native_payload["rx"]
    return logic_analyzer_native_decode_file(native_payload)


def logic_analyzer_native_decode_file(payload: dict[str, Any]) -> dict[str, Any]:
    native = _native_cli_path()
    if not native:
        return {"ok": False, "error": "dslogic-cli not found"}
    input_file = payload.get("input_file")
    decoder = str(payload.get("decoder", "uart")).lower()
    if not input_file:
        return {"ok": False, "error": "input_file is required"}
    if decoder not in {"uart", "spi", "i2c"}:
        return {"ok": False, "error": "decoder must be uart, spi, or i2c"}

    argv = [
        native,
        "decode",
        decoder,
        str(Path(input_file).expanduser()),
        "--samplerate",
        str(int(payload.get("samplerate", 100_000_000))),
    ]
    option_map = {
        "channel": "--channel",
        "baudrate": "--baudrate",
        "data_bits": "--data-bits",
        "parity": "--parity",
        "stop_bits": "--stop-bits",
        "clk": "--clk",
        "miso": "--miso",
        "mosi": "--mosi",
        "scl": "--scl",
        "sda": "--sda",
        "cpol": "--cpol",
        "cpha": "--cpha",
        "wordsize": "--wordsize",
        "bit_order": "--bit-order",
    }
    for key, option in option_map.items():
        if key in payload:
            argv += [option, str(payload[key])]
    r = _run(argv, timeout=int(payload.get("timeout", 60)))
    return {"ok": r["ok"], "stdout": r["stdout"], "stderr": r["stderr"], "command": r["command"], "returncode": r["returncode"]}


def logic_analyzer_dsview_info(payload: dict[str, Any] | None = None) -> dict[str, Any]:
    dsview = _which("DSView") or _which("dsview")
    if not dsview:
        return {"ok": False, "error": "DSView not found"}
    help_r = _run([dsview, "--help"], timeout=10)
    version_r = _run([dsview, "--version"], timeout=10)
    return {
        "ok": help_r["ok"] or version_r["ok"],
        "path": dsview,
        "version": (version_r["stdout"] + version_r["stderr"]).strip(),
        "help": (help_r["stdout"] + help_r["stderr"]).splitlines(),
        "note": "DSView is GUI-first; use dslogic-cli/libsigrok4DSL tools for scripted capture.",
    }


TOOL_REGISTRY = {
    "logic_analyzer_status": logic_analyzer_status,
    "logic_analyzer_scan": logic_analyzer_scan,
    "logic_analyzer_native_scan": logic_analyzer_native_scan,
    "logic_analyzer_list_decoders": logic_analyzer_list_decoders,
    "logic_analyzer_capture": logic_analyzer_capture,
    "logic_analyzer_native_capture": logic_analyzer_native_capture,
    "logic_analyzer_decode_file": logic_analyzer_decode_file,
    "logic_analyzer_uart_decode_file": logic_analyzer_uart_decode_file,
    "logic_analyzer_native_decode_file": logic_analyzer_native_decode_file,
    "logic_analyzer_dsview_info": logic_analyzer_dsview_info,
}
