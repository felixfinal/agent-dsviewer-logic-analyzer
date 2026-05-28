from __future__ import annotations

import argparse
import json
from .config import SERVER_NAME
from .tool_impl import TOOL_REGISTRY


def main() -> None:
    parser = argparse.ArgumentParser(description=f"{SERVER_NAME} runner")
    parser.add_argument("tool", nargs="?")
    parser.add_argument("--input")
    parser.add_argument("--list-tools", action="store_true")
    args = parser.parse_args()
    if args.list_tools:
        print(json.dumps({"server": SERVER_NAME, "tools": sorted(TOOL_REGISTRY.keys())}, ensure_ascii=False, indent=2))
        return
    if not args.tool:
        print(f"{SERVER_NAME} ready")
        return
    payload = json.loads(args.input) if args.input else {}
    func = TOOL_REGISTRY.get(args.tool)
    if func is None:
        raise SystemExit(f"Unknown tool: {args.tool}")
    print(json.dumps(func(payload), ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
