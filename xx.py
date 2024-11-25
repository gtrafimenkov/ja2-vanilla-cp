#!/usr/bin/env python3
#
# Helper script to run different useful commands on the repository.

import subprocess
import os
import sys

USAGE = """Helper script.

Commands:
  format-modified       - format modified files using clang-format
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

    if command == "format-modified":
        modified_files = get_modified_files()
        source_files = filter_source_files(modified_files)
        format_files(source_files)
    else:
        print(f"Unknown command {command}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
