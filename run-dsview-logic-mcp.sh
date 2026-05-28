#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VENV="${DSVIEW_LOGIC_VENV:-$ROOT/.venv}"
if [[ ! -x "$VENV/bin/python" ]]; then
  echo "Missing MCP venv python: $VENV/bin/python" >&2
  echo "Create it with: python3 -m venv .venv && . .venv/bin/activate && pip install -e ." >&2
  exit 1
fi
export DSVIEW_LOGIC_MODE=mcp
exec "$VENV/bin/python" "$ROOT/server.py"
