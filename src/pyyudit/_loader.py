"""Cross-platform discovery and loading of the yudit_shaper native library.

Search order (first hit wins):

1. An explicit path passed to :func:`configure`.
2. The ``PYYUDIT_LIBRARY_PATH`` environment variable.
3. A library bundled inside the wheel at ``pyyudit/_lib/``.
4. A system-installed library (``libyudit_shaper.so`` / ``yudit_shaper.dll``).
"""

from __future__ import annotations

import ctypes
import ctypes.util
import os
import sys
from collections.abc import Iterator
from pathlib import Path

__all__ = [
    "configure",
    "is_available",
    "library_info",
    "library_path",
    "load",
]

if os.name == "nt":
    _PLATFORM_NAMES: tuple[str, ...] = ("yudit_shaper.dll",)
elif sys.platform == "darwin":
    _PLATFORM_NAMES = ("libyudit_shaper.dylib",)
else:
    _PLATFORM_NAMES = ("libyudit_shaper.so", "libyudit_shaper.so.0")

_PACKAGE_DIR = Path(__file__).resolve().parent
_LIB_DIR = _PACKAGE_DIR / "_lib"

# Explicit override set via configure(); None means "auto-discover".
_configured_path: Path | None = None
_resolved_path: Path | None = None
_cached_library: ctypes.CDLL | None = None


def _platform_candidates(directory: Path) -> Iterator[Path]:
    for name in _PLATFORM_NAMES:
        yield directory / name


def _candidate_paths() -> Iterator[Path]:
    seen: set[Path] = set()

    def _offer(path: Path) -> Iterator[Path]:
        if path not in seen:
            seen.add(path)
            yield path

    # 1. explicit runtime configuration
    if _configured_path is not None:
        if _configured_path.is_dir():
            for p in _platform_candidates(_configured_path):
                yield from _offer(p)
        else:
            yield from _offer(_configured_path)

    # 2. environment variable
    env = os.environ.get("PYYUDIT_LIBRARY_PATH")
    if env:
        env_path = Path(env)
        if env_path.is_dir():
            for p in _platform_candidates(env_path):
                yield from _offer(p)
        else:
            yield from _offer(env_path)

    # 3. wheel-local _lib/ directory
    if _LIB_DIR.is_dir():
        for p in _platform_candidates(_LIB_DIR):
            yield from _offer(p)

    # 4. system library
    for name in _PLATFORM_NAMES:
        found = ctypes.util.find_library(name.rsplit(".", 1)[0])
        if found:
            yield from _offer(Path(found))


def resolve() -> Path | None:
    global _resolved_path
    if _resolved_path is not None:
        return _resolved_path
    for path in _candidate_paths():
        if path.is_file():
            _resolved_path = path
            return path
    return None


def load() -> ctypes.CDLL | None:
    global _cached_library
    if _cached_library is not None:
        return _cached_library
    path = resolve()
    if path is None:
        return None
    try:
        if os.name == "nt" and path.is_file():
            os.add_dll_directory(str(path.parent))
        _cached_library = ctypes.CDLL(str(path))
        return _cached_library
    except OSError:
        return None


def configure(path: str | Path | None = None) -> None:
    global _configured_path, _resolved_path, _cached_library
    _configured_path = Path(path) if path else None
    _resolved_path = None
    _cached_library = None


def is_available() -> bool:
    return load() is not None


def library_path() -> Path | None:
    return resolve()


def library_info() -> dict[str, object]:
    path = resolve()
    lib = load()
    return {
        "available": lib is not None,
        "path": str(path) if path else None,
        "search_order": [
            "configure()",
            "PYYUDIT_LIBRARY_PATH",
            "pyyudit/_lib/",
            "system library",
        ],
    }
