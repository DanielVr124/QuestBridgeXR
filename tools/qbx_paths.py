"""Shared project-path discovery for source and portable Windows builds."""
from __future__ import annotations
import sys
from pathlib import Path

def project_root(script_file: str | Path) -> Path:
    """Return the project root for a source script or a packaged tools/*.exe.

    Portable release layout places QuestBridgeStudio.exe at the root and the
    helper executables under tools/, so those binaries resolve one directory up.
    """
    if getattr(sys, "frozen", False):
        executable_dir = Path(sys.executable).resolve().parent
        if executable_dir.name.casefold() == "tools":
            return executable_dir.parent
        return executable_dir
    return Path(script_file).resolve().parents[1]
