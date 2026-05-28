from pathlib import Path
import os

SERVER_NAME = "dsview-logic"
WORKSPACE_ROOT = Path(
    os.environ.get("DSVIEW_LOGIC_WORKSPACE")
    or os.environ.get("OPENCLAW_WORKSPACE")
    or Path.cwd()
).expanduser().resolve()
DEFAULT_OUTPUT_DIR = Path(
    os.environ.get("DSVIEW_LOGIC_OUTPUT_DIR")
    or WORKSPACE_ROOT / "deliverables" / "logic-analyzer"
).expanduser().resolve()
