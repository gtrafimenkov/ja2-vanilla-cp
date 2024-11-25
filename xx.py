#!/usr/bin/env python3
#
# Helper script to run different useful commands on the repository.

import os
import platform
import subprocess
import sys

USAGE = """Helper script.

Commands:
  build                 - build debug version
  build-debug           - build debug version
  build-release         - build release version
  format-modified       - format modified files using clang-format
  clean                 - cleanup repository from all unwanted files
"""

def get_modified_files():
    result = subprocess.run(["git", "status", "--porcelain"], capture_output=True, text=True)
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

def main():
    args = sys.argv[1:]
    if len(args) == 0:
        print(USAGE, file=sys.stderr)
        sys.exit(1)

    command = args[0]

    if command in ["build", "build-debug"]:

        if platform.system() == 'Windows':
            subprocess.run(["cmake", "-B", "build"])
            subprocess.run(["cmake", "--build", "build", "--parallel", "--config", "Debug"])
        else:
            subprocess.run(["cmake", "-B", "build", "-DCMAKE_BUILD_TYPE=Debug"])
            subprocess.run(["cmake", "--build", "build", "--parallel"])

    elif command == "build-release":

        if platform.system() == 'Windows':
            subprocess.run(["cmake", "-B", "build"])
            subprocess.run(["cmake", "--build", "build", "--parallel", "--config", "Release"])
        else:
            subprocess.run(["cmake", "-B", "build-release", "-DCMAKE_BUILD_TYPE=Release"])
            subprocess.run(["cmake", "--build", "build-release", "--parallel"])

    elif command == "clean":
        subprocess.run(["git", "clean", "-fdx"])
    elif command == "format-modified":
        modified_files = get_modified_files()
        source_files = filter_source_files(modified_files)
        format_files(source_files)
    else:
        print(f"Unknown command {command}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
