from pathlib import Path
import os
import sys

ROOT = Path(__file__).resolve().parent
SRC = ROOT / "src"
if str(SRC) not in sys.path:
    sys.path.insert(0, str(SRC))

mode = os.environ.get("DSVIEW_LOGIC_MODE", "runner")
if mode == "mcp":
    from dsview_logic.mcp_server import main
else:
    from dsview_logic.server import main

if __name__ == "__main__":
    main()
