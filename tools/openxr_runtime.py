#!/usr/bin/env python3
"""Inspect installed OpenXR runtimes and launch one app with a per-process runtime override.

This utility never edits the machine-wide active-runtime registry value. On Windows,
XR_RUNTIME_JSON is consumed by the Khronos OpenXR loader and can override the global
runtime selection for a child application launched by this tool.
"""
from __future__ import annotations

import argparse
import json
import os
import pathlib
import shutil
import subprocess
import sys
from dataclasses import dataclass
from typing import Iterable, Mapping, Sequence


@dataclass(frozen=True)
class RuntimeInfo:
    name: str
    manifest: pathlib.Path
    source: str
    library_path: str = ""
    error: str = ""


def read_manifest(path: str | pathlib.Path, source: str = "custom") -> RuntimeInfo:
    """Read and validate the structure of an OpenXR runtime manifest."""
    manifest_path = pathlib.Path(path).expanduser().resolve()
    try:
        content = json.loads(manifest_path.read_text(encoding="utf-8-sig"))
    except (OSError, json.JSONDecodeError) as exc:
        return RuntimeInfo(manifest_path.name, manifest_path, source, error=str(exc))

    runtime = content.get("runtime") if isinstance(content, dict) else None
    if not isinstance(runtime, dict):
        return RuntimeInfo(manifest_path.name, manifest_path, source,
                           error="Manifest must contain a 'runtime' object")
    library = runtime.get("library_path", "")
    name = runtime.get("name") or manifest_path.stem
    if not isinstance(library, str) or not library.strip():
        return RuntimeInfo(str(name), manifest_path, source, error="Runtime manifest has no library_path")
    return RuntimeInfo(str(name), manifest_path, source, library.strip())


def registry_runtime_manifest() -> pathlib.Path | None:
    """Return the registered Windows active-runtime manifest without changing it."""
    if os.name != "nt":
        return None
    try:
        import winreg  # type: ignore[import-not-found]
    except ImportError:
        return None

    keys = (
        (winreg.HKEY_LOCAL_MACHINE, r"SOFTWARE\Khronos\OpenXR\1"),
        (winreg.HKEY_LOCAL_MACHINE, r"SOFTWARE\WOW6432Node\Khronos\OpenXR\1"),
    )
    for root, key_path in keys:
        try:
            with winreg.OpenKey(root, key_path) as key:
                value, _kind = winreg.QueryValueEx(key, "ActiveRuntime")
            if isinstance(value, str) and value.strip():
                return pathlib.Path(value.strip()).expanduser()
        except OSError:
            continue
    return None


def known_manifest_candidates(env: Mapping[str, str] | None = None) -> list[tuple[str, pathlib.Path]]:
    """Return likely Meta and SteamVR manifest locations; only existing ones are kept."""
    env = os.environ if env is None else env
    roots: list[pathlib.Path] = []
    for key in ("ProgramFiles", "ProgramFiles(x86)", "ProgramW6432"):
        value = env.get(key)
        if value:
            root = pathlib.Path(value)
            if root not in roots:
                roots.append(root)

    paths: list[tuple[str, pathlib.Path]] = []
    for root in roots:
        paths.extend((
            ("Meta Quest Link", root / "Oculus" / "Support" / "oculus-runtime" / "oculus_openxr_64.json"),
            ("Meta Quest Link (32-bit)", root / "Oculus" / "Support" / "oculus-runtime" / "oculus_openxr_32.json"),
            ("SteamVR", root / "Steam" / "steamapps" / "common" / "SteamVR" / "steamxr_win64.json"),
        ))
    return [(label, path) for label, path in paths if path.is_file()]


def discover_runtimes(env: Mapping[str, str] | None = None,
                      exists=pathlib.Path.is_file,
                      registry_path: pathlib.Path | None = None) -> list[RuntimeInfo]:
    """Discover environment override, Windows registry runtime, and common installs."""
    env = os.environ if env is None else env
    candidates: list[tuple[str, pathlib.Path, str]] = []
    override = (env.get("XR_RUNTIME_JSON") or "").strip()
    if override:
        candidates.append(("Current process XR_RUNTIME_JSON", pathlib.Path(override).expanduser(), "environment"))
    if registry_path is None:
        registry_path = registry_runtime_manifest()
    if registry_path is not None:
        candidates.append(("Windows registered active runtime", pathlib.Path(registry_path).expanduser(), "registry"))
    for name, path in known_manifest_candidates(env):
        candidates.append((name, path, "detected install"))

    seen: set[str] = set()
    out: list[RuntimeInfo] = []
    for label, path, source in candidates:
        key = str(path.expanduser().resolve(strict=False)).casefold()
        if key in seen:
            continue
        seen.add(key)
        if not exists(path):
            out.append(RuntimeInfo(label, path, source, error="Manifest file not found"))
        else:
            out.append(read_manifest(path, source))
    return out



def bundled_qbx_runtime_manifest() -> pathlib.Path:
    """Find the packaged QuestBridge runtime manifest beside its DLL.

    The DLL is opt-in only: this helper never writes the Windows active-runtime registry.
    """
    roots: list[pathlib.Path] = []
    if getattr(sys, "frozen", False):
        exe_dir = pathlib.Path(sys.executable).resolve().parent
        roots.extend((exe_dir.parent, exe_dir))
    else:
        roots.append(pathlib.Path(__file__).resolve().parent.parent)
    checked: set[str] = set()
    for root in roots:
        for candidate in (
            root / "runtime" / "questbridge_openxr_runtime.json",
            root / "runtime" / "manifest" / "questbridge_openxr_runtime.json",
        ):
            key = str(candidate.resolve(strict=False)).casefold()
            if key in checked:
                continue
            checked.add(key)
            if candidate.is_file():
                info = read_manifest(candidate, source="QuestBridge experimental runtime")
                if info.error:
                    raise ValueError(f"QuestBridge runtime manifest is invalid: {info.error}")
                library = pathlib.Path(info.library_path)
                if not library.is_absolute():
                    library = candidate.parent / library
                if not library.is_file():
                    raise FileNotFoundError(
                        f"QuestBridge runtime DLL not found beside its manifest: {library}. "
                        "Build the Windows runtime first; the source archive alone is not an installed runtime."
                    )
                return candidate.resolve()
    raise FileNotFoundError(
        "QuestBridge runtime is not built/packaged. Expected runtime/questbridge_openxr_runtime.json "
        "and qbx_openxr_runtime.dll in the portable app folder."
    )

def resolve_runtime_manifest(mode: str, discovered: Sequence[RuntimeInfo], custom_manifest: str | None = None) -> pathlib.Path:
    """Resolve active/meta/steamvr/custom mode to a valid runtime manifest path."""
    mode = mode.casefold()
    if mode == "qbx":
        return bundled_qbx_runtime_manifest()

    if mode == "custom":
        if not custom_manifest:
            raise ValueError("--manifest is required when --runtime custom is selected")
        info = read_manifest(custom_manifest)
        if info.error:
            raise ValueError(f"Invalid runtime manifest: {info.error}")
        return info.manifest

    if mode == "active":
        # The environment override wins because that is what the OpenXR loader sees.
        candidates = [item for item in discovered if item.source == "environment"]
        candidates += [item for item in discovered if item.source == "registry"]
        if not candidates:
            raise ValueError("No active runtime manifest was discovered. Install/select a runtime or use --runtime meta.")
        chosen = candidates[0]
    elif mode == "meta":
        candidates = [item for item in discovered
                      if item.source == "detected install"
                      and "oculus-runtime" in str(item.manifest).casefold()
                      and "32.json" not in item.manifest.name.casefold()]
        if not candidates:
            raise ValueError("Meta Quest Link 64-bit runtime manifest was not found in the standard install paths.")
        chosen = candidates[0]
    elif mode == "steamvr":
        candidates = [item for item in discovered
                      if item.source == "detected install"
                      and "steamvr" in str(item.manifest).casefold()
                      and "steamxr_win64.json" in item.manifest.name.casefold()]
        if not candidates:
            raise ValueError("SteamVR runtime manifest was not found in the standard install paths.")
        chosen = candidates[0]
    else:
        raise ValueError(f"Unknown runtime mode: {mode}")

    if chosen.error:
        raise ValueError(f"{chosen.name}: {chosen.error} ({chosen.manifest})")
    return chosen.manifest.resolve()


def launch_with_runtime(app: str | pathlib.Path, app_args: Sequence[str], manifest: str | pathlib.Path,
                        *, popen=subprocess.Popen) -> subprocess.Popen:
    """Launch an app with a process-local XR_RUNTIME_JSON override; no global mutation."""
    app_path = pathlib.Path(app).expanduser()
    if not app_path.is_file():
        resolved = shutil.which(str(app))
        if resolved is None:
            raise FileNotFoundError(f"Application executable not found: {app}")
        app_path = pathlib.Path(resolved)
    info = read_manifest(manifest)
    if info.error:
        raise ValueError(f"Invalid runtime manifest: {info.error}")
    env = os.environ.copy()
    env["XR_RUNTIME_JSON"] = str(info.manifest)
    return popen([str(app_path.resolve()), *app_args], cwd=str(app_path.resolve().parent), env=env)


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--list", action="store_true", help="list discovered runtime manifests")
    parser.add_argument("--runtime", choices=("active", "meta", "steamvr", "qbx", "custom"), default="active",
                        help="runtime to choose for a child app (qbx is experimental; no Windows registry changes)")
    parser.add_argument("--manifest", help="custom runtime JSON manifest (used with --runtime custom)")
    parser.add_argument("--app", help="application EXE to launch")
    parser.add_argument("app_args", nargs=argparse.REMAINDER, help="arguments for --app; separate with --")
    args = parser.parse_args(argv)

    runtimes = discover_runtimes()
    if args.list or not args.app:
        print("QuestBridge OpenXR runtime discovery (read-only)")
        print("=" * 48)
        if not runtimes:
            print("No OpenXR runtime manifests found in the environment or common install locations.")
        for item in runtimes:
            state = f"ERROR: {item.error}" if item.error else f"library_path={item.library_path}"
            print(f"- {item.name}\n  source: {item.source}\n  manifest: {item.manifest}\n  {state}")
        print("\nNo machine-wide runtime setting was changed.")
        if not args.app:
            return 0

    try:
        manifest = resolve_runtime_manifest(args.runtime, runtimes, args.manifest)
        if not args.app:
            print(f"Selected manifest: {manifest}")
            return 0
        app_args = list(args.app_args)
        if app_args and app_args[0] == "--":
            app_args = app_args[1:]
        child = launch_with_runtime(args.app, app_args, manifest)
        print(f"Launched {args.app} with XR_RUNTIME_JSON={manifest} (PID {child.pid}).")
        return 0
    except (OSError, ValueError, subprocess.SubprocessError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
