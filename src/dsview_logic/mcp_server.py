from __future__ import annotations

from mcp.server.fastmcp import FastMCP

from .config import SERVER_NAME
from .tool_impl import (
    logic_analyzer_capture,
    logic_analyzer_decode_file,
    logic_analyzer_dsview_info,
    logic_analyzer_list_decoders,
    logic_analyzer_scan,
    logic_analyzer_status,
    logic_analyzer_uart_decode_file,
)

mcp = FastMCP(name=SERVER_NAME)


@mcp.tool(name="logic_analyzer_status", description="Check local DSView/sigrok logic analyzer tool availability and versions")
def tool_logic_analyzer_status(payload: dict | None = None):
    return logic_analyzer_status(payload or {})


@mcp.tool(name="logic_analyzer_scan", description="Run sigrok-cli --scan to discover connected logic analyzers and instruments")
def tool_logic_analyzer_scan(payload: dict | None = None):
    return logic_analyzer_scan(payload or {})


@mcp.tool(name="logic_analyzer_list_decoders", description="List sigrok protocol decoders, optionally filtered by query")
def tool_logic_analyzer_list_decoders(payload: dict | None = None):
    return logic_analyzer_list_decoders(payload or {})


@mcp.tool(name="logic_analyzer_capture", description="Run a bounded sigrok-cli capture to a file; provide driver/config/channels/samples or time")
def tool_logic_analyzer_capture(payload: dict):
    return logic_analyzer_capture(payload)


@mcp.tool(name="logic_analyzer_decode_file", description="Decode a saved sigrok capture file with a protocol decoder")
def tool_logic_analyzer_decode_file(payload: dict):
    return logic_analyzer_decode_file(payload)


@mcp.tool(name="logic_analyzer_uart_decode_file", description="Decode UART from a saved sigrok/srzip file with normalized baud/channel/options")
def tool_logic_analyzer_uart_decode_file(payload: dict):
    return logic_analyzer_uart_decode_file(payload)


@mcp.tool(name="logic_analyzer_dsview_info", description="Show DSView GUI binary version/help; scripted capture uses sigrok-cli")
def tool_logic_analyzer_dsview_info(payload: dict | None = None):
    return logic_analyzer_dsview_info(payload or {})


def main():
    mcp.run(transport="stdio")


if __name__ == "__main__":
    main()
