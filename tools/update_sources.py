#!/usr/bin/env python3
#
# Script for updating source files.

import os


def include_text_on_top_if_substring_found(filename, substring, include_text):
    with open(filename, "r+", encoding="utf-8", newline="\n") as f:
        content = f.read()
        if substring in content:
            f.seek(0, 0)
            f.write(include_text + content)


def find_files(dir, ext):
    found = []
    for root, _, files in os.walk(dir):
        for file in files:
            if file.endswith(ext):
                found.append(os.path.join(root, file))
    return found


def find_substring_and_include_text(src_dir, extensions, substring, include_text):
    files = []
    for ext in extensions:
        files += find_files(src_dir, ext)
    for f in files:
        include_text_on_top_if_substring_found(f, substring, include_text)


def main():
    find_substring_and_include_text(
        "ja2",
        [".h", ".cc"],
        " ",
        "// This is not free software.\n// This file contains code derived from the code released under the terms\n// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.\n\n",
    )
    find_substring_and_include_text(
        "rustlib",
        [".rs"],
        " ",
        "// This is not free software.\n// This file may contain code derived from the code released under the terms\n// of Strategy First Inc. Source Code License Agreement. See SFI-SCLA.txt.\n\n",
    )


if __name__ == "__main__":
    main()
