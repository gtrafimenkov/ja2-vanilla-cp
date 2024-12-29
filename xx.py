#!/usr/bin/env python3
#
# Helper script to run different useful commands on the repository.

import os
import platform
import subprocess
import sys
import shutil

USAGE = """Helper script to run one or more predefiend commands.

Usage:
  python xx.py COMMAND [COMMAND...]

Commands:
  build                 - build debug version
  build-debug           - build debug version
  build-release         - build release version
  format-modified       - format modified files using clang-format
  copy-data             - find and copy game data to the debug build localtion
  clean                 - cleanup repository from all unwanted files
  run                   - run debug build

Examples:
  python xx.py build copy-data run
"""


def get_modified_files():
    result = subprocess.run(
        ["git", "status", "--porcelain"], capture_output=True, text=True
    )
    modified_files = []
    for line in result.stdout.splitlines():
        flags, file_path = line.split()[:2]
        if "M" in flags:
            modified_files.append(file_path)
    return modified_files


def filter_source_files(files):
    source_extensions = [".h", ".c", ".cpp", ".cc"]
    return [file for file in files if os.path.splitext(file)[1] in source_extensions]


def format_files(files):
    if len(files) > 0:
        print(f"Formatting {len(files)} files ...", file=sys.stderr)
        subprocess.run(["clang-format", "-i", "--style=file"] + files)
    else:
        print("No files to format", file=sys.stderr)


def find_ja2_data_files():
    try_dirs = [
        r"C:\Program Files (x86)\Steam\steamapps\common\Jagged Alliance 2 Gold\Data",
        "~/.local/share/Steam/steamapps/common/ja2_wildfire/JA2Classic/Game/Data",
    ]
    for d in try_dirs:
        d = os.path.expanduser(d)
        if os.path.isdir(d):
            return d
    return None


def copy_ja2_data_files(dest_dir):
    if os.path.isdir(dest_dir):
        print("Data already copied", file=sys.stderr)
    else:
        source = find_ja2_data_files()
        if source is None:
            print("Cannot find data files for JA2", file=sys.stderr)
        else:
            shutil.copytree(source, dest_dir)


def get_debug_build_location():
    if platform.system() == "Windows":
        return "build/Debug"
    return "build"


def get_debug_build_exe():
    if platform.system() == "Windows":
        return "build/Debug/ja2vcp.exe"
    return "build/ja2vcp"


def test_debug():
    if platform.system() == "Windows":
        subprocess.run(["build/Debug/ja2vcp.exe", "--unittests"])
    else:
        subprocess.run(["./build/ja2vcp", "--unittests"])


def test_release():
    if platform.system() == "Windows":
        subprocess.run(["build/Release/ja2vcp.exe", "--unittests"])
    else:
        subprocess.run(["./build-release/ja2vcp", "--unittests"])


def run_command(command):
    if command in ["build", "build-debug"]:
        if platform.system() == "Windows":
            subprocess.run(["cmake", "-B", "build"])
            subprocess.run(
                ["cmake", "--build", "build", "--parallel", "--config", "Debug"]
            )
        else:
            subprocess.run(["cmake", "-B", "build", "-DCMAKE_BUILD_TYPE=Debug"])
            subprocess.run(["cmake", "--build", "build", "--parallel"])

    elif command == "build-release":
        if platform.system() == "Windows":
            subprocess.run(["cmake", "-B", "build"])
            subprocess.run(
                ["cmake", "--build", "build", "--parallel", "--config", "Release"]
            )
        else:
            subprocess.run(
                ["cmake", "-B", "build-release", "-DCMAKE_BUILD_TYPE=Release"]
            )
            subprocess.run(["cmake", "--build", "build-release", "--parallel"])

    elif command == "clean":
        subprocess.run(["git", "clean", "-fdx"])

    elif command == "copy-data":
        dest_dir = os.path.join(get_debug_build_location(), "data")
        copy_ja2_data_files(dest_dir)

    elif command == "format-modified":
        modified_files = get_modified_files()
        source_files = filter_source_files(modified_files)
        format_files(source_files)

    elif command == "run":
        subprocess.run([get_debug_build_exe()])

    elif command == "test-debug":
        test_debug()

    elif command == "test-release":
        test_release()

    else:
        print(f"Unknown command {command}", file=sys.stderr)
        sys.exit(1)


def main():
    args = sys.argv[1:]
    if len(args) == 0:
        print(USAGE, file=sys.stderr)
        sys.exit(1)

    commands = args
    for command in commands:
        run_command(command)


if __name__ == "__main__":
    main()
