#!/usr/bin/env python3
"""Reconstruct the adapted VD-STAR source from the original released artifact."""
from pathlib import Path
import re
import shutil
import subprocess
import sys

ORDER = ['B-001-query-buffer-bounds-and-lifetime.patch', 'D-003-sorted-neighbor-delete-consistency.patch', 'B-002-large-vertex-nonlast-delete.patch', 'B-003-dtmanager-swap-delete-index-consistency.patch', 'D-001-core-threshold-degree-boundary.patch', 'D-002-complete-query-semantics.patch', 'D-005-affordability-reset-scale.patch', 'D-004-failure-probability-initialization.patch', 'B-005-owned-object-lifecycle.patch', 'C-001-explicit-approximation-seed.patch', 'D-006-small-edge-similarity-maintenance.patch']

def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: replay_patches.py UPSTREAM_DIRECTORY NEW_OUTPUT_DIRECTORY")
    source = Path(sys.argv[1]).resolve(strict=True)
    destination = Path(sys.argv[2]).resolve()
    if destination.exists():
        raise FileExistsError(destination)
    destination.mkdir(parents=True)
    for dirname in ("graph", "dt", "MyLib", "Tessil_robin_map"):
        for path in (source / dirname).iterdir():
            if path.is_file() and (path.suffix in {".cpp", ".h"} or path.name == "LICENSE"):
                target = destination / dirname / path.name
                target.parent.mkdir(exist_ok=True)
                shutil.copyfile(path, target)
    shutil.copyfile(source / "main.cpp", destination / "main.cpp")
    header = (destination / "graph/Jaccard.h").read_text()
    declarations = re.search(r"static long long (\w+);\s*static const int (\w+) = 500000;", header)
    if not declarations:
        raise ValueError("unexpected upstream sampling-counter declarations")
    replacements = dict(zip(declarations.groups(), ("group_num", "group_size")))
    for path in (destination / "graph").glob("Jaccard.*"):
        text = path.read_text()
        for old, new in replacements.items():
            text = re.sub(r"\b" + re.escape(old) + r"\b", new, text)
        path.write_bytes(text.encode())
    subprocess.run(["git", "init", "--quiet", str(destination)], check=True)
    patches = Path(__file__).resolve().parent / "patches"
    for name in ORDER:
        subprocess.run(["git", "-C", str(destination), "apply", str(patches / name)], check=True)
    for path in destination.rglob("*"):
        if path.is_file() and path.suffix in {".cpp", ".h"} and ".git" not in path.parts:
            path.write_bytes(path.read_bytes().replace(b"\r\n", b"\n"))
    print("Reconstructed adapted algorithm source:", destination)

if __name__ == "__main__":
    main()
