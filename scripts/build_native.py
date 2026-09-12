"""Build script for the yudit_shaper native library.

Usage:
    python scripts/build_native.py              # auto-detect generator
    python scripts/build_native.py --clean       # clean build
    python scripts/build_native.py --release     # Release build

This script:
1. Configures CMake with the Yudit source directory
2. Builds the shared library (DLL/SO/dylib)
3. Copies the result to src/pyyudit/_lib/ for wheel bundling
"""

from __future__ import annotations

import argparse
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BUILD_DIR = ROOT / "build"
LIB_DIR = ROOT / "src" / "pyyudit" / "_lib"
YUDIT_SRC = ROOT / "native" / "vendor" / "yudit"


def find_cmake() -> str:
    """Find the cmake executable."""
    cmake = shutil.which("cmake")
    if cmake:
        return cmake
    raise RuntimeError("cmake not found. Install CMake (https://cmake.org)")


def find_generator() -> str:
    """Pick a sensible default CMake generator."""
    if platform.system() == "Windows":
        # Try to find Visual Studio
        return "Visual Studio 17 2022"
    return "Unix Makefiles"


def build(release: bool = True, clean: bool = False) -> Path:
    """Build the native library and return the path to the output."""
    cmake = find_cmake()

    if clean and BUILD_DIR.exists():
        shutil.rmtree(BUILD_DIR)
    BUILD_DIR.mkdir(parents=True, exist_ok=True)

    config = "Release" if release else "Debug"
    gen = find_generator()

    # Configure
    cmd = [
        cmake,
        "-S",
        str(ROOT),
        "-B",
        str(BUILD_DIR),
        f"-DCMAKE_BUILD_TYPE={config}",
        f"-DYUDIT_SRC_DIR={YUDIT_SRC}",
    ]
    if gen:
        cmd.extend(["-G", gen])

    print(f"Configuring: {' '.join(cmd)}")
    subprocess.check_call(cmd)

    # Build
    build_cmd = [cmake, "--build", str(BUILD_DIR), "--config", config]
    print(f"Building: {' '.join(build_cmd)}")
    subprocess.check_call(build_cmd)

    # Find the output
    if platform.system() == "Windows":
        candidates = list(BUILD_DIR.glob("**/yudit_shaper.dll"))
    elif platform.system() == "Darwin":
        candidates = list(BUILD_DIR.glob("**/libyudit_shaper.dylib"))
    else:
        candidates = list(BUILD_DIR.glob("**/libyudit_shaper.so*"))
        # Filter out symlinks and versioned files
        candidates = [c for c in candidates if not c.is_symlink()]

    if not candidates:
        raise RuntimeError("Build succeeded but output library not found")

    lib_path = candidates[0]
    print(f"Built: {lib_path}")

    # Copy to _lib/
    LIB_DIR.mkdir(parents=True, exist_ok=True)
    dest = LIB_DIR / lib_path.name
    shutil.copy2(lib_path, dest)
    print(f"Copied to: {dest}")

    return dest


def main() -> None:
    parser = argparse.ArgumentParser(description="Build yudit_shaper native library")
    parser.add_argument("--clean", action="store_true", help="Clean build directory first")
    parser.add_argument("--debug", action="store_true", help="Debug build instead of Release")
    args = parser.parse_args()

    if not YUDIT_SRC.is_dir():
        print(f"ERROR: Yudit source not found at {YUDIT_SRC}")
        print("Download from https://www.yudit.org/download/yudit-3.1.0.tar.gz")
        sys.exit(1)

    build(release=not args.debug, clean=args.clean)


if __name__ == "__main__":
    main()
