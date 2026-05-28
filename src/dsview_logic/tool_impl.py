from __future__ import annotations

import json
import os
import shlex
import shutil
import subprocess
import time
from pathlib import Path
from typing import Any

from .config import DEFAULT_OUTPUT_DIR


def _run(argv: list[str], timeout: int = 30) -> dict[str, Any]:
    try:
        proc = subprocess.run(argv, text=True, capture_output=True, timeout=timeout, check=False)
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
    sigrok = _which("sigrok-cli")
    result: dict[str, Any] = {
        "dsview_path": dsview,
        "sigrok_cli_path": sigrok,
        "fx2lafw_firmware_dir_exists": Path("/usr/share/sigrok-firmware").exists(),
        "dreamsourcelab_udev_rule": Path("/etc/udev/rules.d/60-dreamsourcelab.rules").exists(),
        "output_dir": str(DEFAULT_OUTPUT_DIR),
    }
    if dsview:
        r = _run([dsview, "--version"], timeout=10)
        result["dsview_version"] = (r["stdout"] + r["stderr"]).strip()
    if sigrok:
        r = _run([sigrok, "--version"], timeout=10)
        result["sigrok_version"] = (r["stdout"] + r["stderr"]).splitlines()[:20]
    result["ok"] = bool(dsview and sigrok)
    return result


def logic_analyzer_scan(payload: dict[str, Any] | None = None) -> dict[str, Any]:
    payload = payload or {}
    sigrok = _which("sigrok-cli")
    if not sigrok:
        return {"ok": False, "error": "sigrok-cli not found"}
    r = _run([sigrok, "--scan"], timeout=int(payload.get("timeout", 20)))
    text = (r["stdout"] + r["stderr"]).strip()
    devices = [line.strip() for line in text.splitlines() if line.strip() and not line.startswith("The following")]
    return {"ok": r["ok"], "devices": devices, "raw": text, "command": r["command"], "returncode": r["returncode"]}


def logic_analyzer_list_decoders(payload: dict[str, Any] | None = None) -> dict[str, Any]:
    payload = payload or {}
    sigrok = _which("sigrok-cli")
    if not sigrok:
        return {"ok": False, "error": "sigrok-cli not found"}
    r = _run([sigrok, "--list-supported"], timeout=int(payload.get("timeout", 30)))
    lines = r["stdout"].splitlines()
    decoders: list[str] = []
    in_section = False
    for line in lines:
        if line.strip() == "Supported protocol decoders:":
            in_section = True
            continue
        if in_section and line.startswith("Supported "):
            break
        if in_section and line.strip():
            decoders.append(line.strip())
    query = str(payload.get("query", "")).lower().strip()
    if query:
        decoders = [d for d in decoders if query in d.lower()]
    return {"ok": r["ok"], "decoders": decoders, "count": len(decoders), "stderr": r["stderr"], "command": r["command"]}


def logic_analyzer_capture(payload: dict[str, Any]) -> dict[str, Any]:
    sigrok = _which("sigrok-cli")
    if not sigrok:
        return {"ok": False, "error": "sigrok-cli not found"}
    driver = payload.get("driver")
    requested_output_format = str(payload.get("output_format", "srzip"))
    # Debian's sigrok package exposes the raw session writer as "srzip", not "sr".
    # Keep accepting "sr" as a user-facing alias because .sr is the normal filename extension.
    output_format = "srzip" if requested_output_format == "sr" else requested_output_format
    suffix = "sr" if output_format == "srzip" else output_format
    output = _safe_output_path(payload.get("output_file"), suffix)
    argv = [sigrok]
    if driver:
        argv += ["--driver", str(driver)]
    channels = payload.get("channels")
    if channels:
        argv += ["--channels", str(channels)]
    config = payload.get("config")
    if isinstance(config, dict):
        config_text = ",".join(f"{k}={v}" for k, v in config.items())
        if config_text:
            argv += ["--config", config_text]
    elif config:
        argv += ["--config", str(config)]
    if payload.get("triggers"):
        argv += ["--triggers", str(payload["triggers"])]
    if payload.get("wait_trigger"):
        argv += ["--wait-trigger"]
    if payload.get("time"):
        argv += ["--time", str(payload["time"])]
    else:
        argv += ["--samples", str(payload.get("samples", 1000))]
    argv += ["--output-file", str(output), "--output-format", str(output_format)]
    timeout = int(payload.get("timeout", 30))
    r = _run(argv, timeout=timeout)
    return {
        "ok": r["ok"] and output.exists(),
        "output_file": str(output),
        "size": output.stat().st_size if output.exists() else 0,
        "requested_output_format": requested_output_format,
        "effective_output_format": output_format,
        "command": r["command"],
        "command_shell": " ".join(shlex.quote(x) for x in r["command"]),
        "returncode": r["returncode"],
        "stdout": r["stdout"],
        "stderr": r["stderr"],
    }


def logic_analyzer_decode_file(payload: dict[str, Any]) -> dict[str, Any]:
    sigrok = _which("sigrok-cli")
    if not sigrok:
        return {"ok": False, "error": "sigrok-cli not found"}
    input_file = payload.get("input_file")
    decoder = payload.get("decoder")
    if not input_file or not decoder:
        return {"ok": False, "error": "input_file and decoder are required"}
    argv = [sigrok, "--input-file", str(Path(input_file).expanduser()), "--protocol-decoders", str(decoder)]
    annotations = payload.get("annotations")
    if annotations:
        argv += ["--protocol-decoder-annotations", str(annotations)]
    if payload.get("samplenum"):
        argv += ["--protocol-decoder-samplenum"]
    r = _run(argv, timeout=int(payload.get("timeout", 60)))
    return {"ok": r["ok"], "stdout": r["stdout"], "stderr": r["stderr"], "command": r["command"], "returncode": r["returncode"]}


def _sigrok_uart_token_to_text(token: str) -> str:
    token = token.rstrip("\r\n")
    if token.startswith("[") and token.endswith("]"):
        try:
            value = int(token[1:-1], 16)
        except ValueError:
            return token
        if value == 0x0D:
            return "\r"
        if value == 0x0A:
            return "\n"
        if 32 <= value < 127:
            return chr(value)
        return f"<{value:02X}>"
    return token


def logic_analyzer_uart_decode_file(payload: dict[str, Any]) -> dict[str, Any]:
    """Decode UART from a saved sigrok/srzip file with normalized 8N1 defaults."""
    sigrok = _which("sigrok-cli")
    if not sigrok:
        return {"ok": False, "error": "sigrok-cli not found"}
    input_file = payload.get("input_file")
    if not input_file:
        return {"ok": False, "error": "input_file is required"}
    channel = str(payload.get("channel", payload.get("rx", "0")))
    baudrate = int(payload.get("baudrate", 115200))
    data_bits = int(payload.get("data_bits", 8))
    parity = str(payload.get("parity", "none"))
    stop_bits = str(payload.get("stop_bits", "1"))
    data_format = str(payload.get("format", "ascii"))
    direction = str(payload.get("direction", "rx")).lower().strip()
    if direction not in {"rx", "tx"}:
        return {"ok": False, "error": "direction must be 'rx' or 'tx'"}
    decoder = (
        f"uart:{direction}={channel}:baudrate={baudrate}:data_bits={data_bits}:"
        f"parity={parity}:stop_bits={stop_bits}:format={data_format}"
    )
    annotation = f"uart={direction}-data"
    argv = [
        sigrok,
        "--input-file", str(Path(input_file).expanduser()),
        "--protocol-decoders", decoder,
        "--protocol-decoder-annotations", annotation,
    ]
    if payload.get("samplenum"):
        argv += ["--protocol-decoder-samplenum"]
    r = _run(argv, timeout=int(payload.get("timeout", 60)))
    # Preserve decoded ASCII spaces. sigrok emits them as lines like "uart-1:  ";
    # stripping the whole line would erase valid UART data bytes.
    lines = [line.rstrip("\n") for line in r["stdout"].splitlines() if line.strip()]
    tokens: list[str] = []
    for line in lines:
        if ":" in line:
            token = line.split(":", 1)[1]
            if token.startswith(" "):
                token = token[1:]
            tokens.append(token)
    text = "".join(_sigrok_uart_token_to_text(token) for token in tokens)
    return {
        "ok": r["ok"],
        "text": text,
        "tokens": tokens,
        "line_count": len(lines),
        "decoder": decoder,
        "annotation": annotation,
        "stdout": r["stdout"],
        "stderr": r["stderr"],
        "command": r["command"],
        "returncode": r["returncode"],
    }


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
        "note": "DSView is GUI-first; use sigrok-cli/MCP tools for scripted capture.",
    }


TOOL_REGISTRY = {
    "logic_analyzer_status": logic_analyzer_status,
    "logic_analyzer_scan": logic_analyzer_scan,
    "logic_analyzer_list_decoders": logic_analyzer_list_decoders,
    "logic_analyzer_capture": logic_analyzer_capture,
    "logic_analyzer_decode_file": logic_analyzer_decode_file,
    "logic_analyzer_uart_decode_file": logic_analyzer_uart_decode_file,
    "logic_analyzer_dsview_info": logic_analyzer_dsview_info,
}
