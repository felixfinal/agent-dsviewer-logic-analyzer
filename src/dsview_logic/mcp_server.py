from __future__ import annotations

from mcp.server.fastmcp import FastMCP

from .config import SERVER_NAME
from .tool_impl import (
    logic_analyzer_capture,
    logic_analyzer_decode_file,
    logic_analyzer_dsview_info,
    logic_analyzer_list_decoders,
    logic_analyzer_native_capture,
    logic_analyzer_native_decode_file,
    logic_analyzer_native_scan,
    logic_analyzer_scan,
    logic_analyzer_status,
    logic_analyzer_uart_decode_file,
)

mcp = FastMCP(name=SERVER_NAME)


@mcp.tool(name="logic_analyzer_status", description="Check local DSView dslogic-cli/libsigrok4DSL logic analyzer tool availability and versions")
def tool_logic_analyzer_status(payload: dict | None = None):
    return logic_analyzer_status(payload or {})


@mcp.tool(name="logic_analyzer_scan", description="Compatibility alias for native dslogic-cli scan")
def tool_logic_analyzer_scan(payload: dict | None = None):
    return logic_analyzer_scan(payload or {})


@mcp.tool(name="logic_analyzer_native_scan", description="Run dslogic-cli scan for native DSLogic/libsigrok4DSL devices")
def tool_logic_analyzer_native_scan(payload: dict | None = None):
    return logic_analyzer_native_scan(payload or {})


@mcp.tool(name="logic_analyzer_list_decoders", description="List native dslogic-cli built-in decoders, optionally filtered by query")
def tool_logic_analyzer_list_decoders(payload: dict | None = None):
    return logic_analyzer_list_decoders(payload or {})


@mcp.tool(name="logic_analyzer_capture", description="Compatibility alias for bounded native dslogic-cli capture")
def tool_logic_analyzer_capture(payload: dict):
    return logic_analyzer_capture(payload)


@mcp.tool(name="logic_analyzer_native_capture", description="Run a bounded native dslogic-cli capture, including DSLogic Plus 100MHz x3 stream mode")
def tool_logic_analyzer_native_capture(payload: dict):
    return logic_analyzer_native_capture(payload)


@mcp.tool(name="logic_analyzer_decode_file", description="Compatibility alias for native dslogic-cli raw binary decode")
def tool_logic_analyzer_decode_file(payload: dict):
    return logic_analyzer_decode_file(payload)


@mcp.tool(name="logic_analyzer_uart_decode_file", description="Compatibility alias for native dslogic-cli UART decode")
def tool_logic_analyzer_uart_decode_file(payload: dict):
    return logic_analyzer_uart_decode_file(payload)


@mcp.tool(name="logic_analyzer_native_decode_file", description="Decode a native dslogic-cli raw binary capture with built-in UART/SPI/I2C decoders")
def tool_logic_analyzer_native_decode_file(payload: dict):
    return logic_analyzer_native_decode_file(payload)


@mcp.tool(name="logic_analyzer_dsview_info", description="Show DSView GUI binary version/help; scripted capture uses dslogic-cli/libsigrok4DSL")
def tool_logic_analyzer_dsview_info(payload: dict | None = None):
    return logic_analyzer_dsview_info(payload or {})


def main():
    mcp.run(transport="stdio")


if __name__ == "__main__":
    main()
