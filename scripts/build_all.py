#!/usr/bin/env python3
"""
ULWAI AI Agent - Cross-Platform Build & Test Orchestrator
Platform-independent Python build script for Linux, Windows, and macOS.
"""

import os
import sys
import argparse
import subprocess
import multiprocessing
from pathlib import Path

def run_command(cmd, cwd=None):
    print(f"\n[EXEC] {' '.join(cmd)}")
    result = subprocess.run(cmd, cwd=cwd)
    if result.returncode != 0:
        print(f"[FAIL] Command failed with exit code {result.returncode}")
        sys.exit(result.returncode)

def main():
    parser = argparse.ArgumentParser(description="Build and test ULWAI AI Agent")
    parser.add_argument("--build-type", default="Release", choices=["Release", "Debug"], help="Build configuration")
    parser.add_argument("--no-gui", action="store_true", help="Build terminal CLI only (skip GUI)")
    parser.add_argument("--skip-tests", action="store_true", help="Skip running test suite after build")
    parser.add_argument("--clean", action="store_true", help="Clean build directory before compiling")
    args = parser.parse_args()

    project_root = Path(__file__).resolve().parent.parent
    build_dir = project_root / "build"

    version_file = project_root / "VERSION"
    version = version_file.read_text(encoding="utf-8").strip() if version_file.exists() else "2.0.0"

    print("==================================================")
    print(f" ULWAI AI Agent v{version} - Native C++20 Build System")
    print(f" Platform : {sys.platform}")
    print(f" Root     : {project_root}")
    print(f" Config   : {args.build_type}")
    print("==================================================")

    if args.clean and build_dir.exists():
        print("[CLEAN] Removing build directory...")
        import shutil
        shutil.rmtree(build_dir, ignore_errors=True)

    build_dir.mkdir(parents=True, exist_ok=True)

    # 1. CMake Configure
    cmake_args = [
        "cmake",
        "-S", str(project_root),
        "-B", str(build_dir),
        f"-DCMAKE_BUILD_TYPE={args.build_type}",
        f"-DBUILD_TESTS={'OFF' if args.skip_tests else 'ON'}",
        f"-DBUILD_GUI={'OFF' if args.no_gui else 'ON'}"
    ]
    run_command(cmake_args)

    # 2. CMake Build
    cpu_count = str(multiprocessing.cpu_count())
    build_cmd = [
        "cmake",
        "--build", str(build_dir),
        "--config", args.build_type,
        "--parallel", cpu_count
    ]
    run_command(build_cmd)

    # 3. Run Tests
    if not args.skip_tests:
        print("\n[TEST] Running comprehensive unit tests...")
        test_cmd = [
            "ctest",
            "--test-dir", str(build_dir),
            "--output-on-failure",
            "-C", args.build_type
        ]
        run_command(test_cmd)

    print("\n==================================================")
    print(f">>> SUCCESS: ULWAI AI Agent v{version} build complete! <<<")
    print(f" Executable: {build_dir / 'ulw-ai-agent'}")
    print("==================================================")

if __name__ == "__main__":
    main()
