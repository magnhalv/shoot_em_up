#!/usr/bin/env python3
"""
Generate compile_commands.json for the shoot_em_up project.

Usage: python scripts/gen_compile_commands.py [repo_dir]
  repo_dir: path to repo root (default: parent of this script's directory)

Output: <repo_dir>/compile_commands.json
"""

import json
import sys
from pathlib import Path


def fwd(p: Path) -> str:
    return str(p).replace("\\", "/")


def make_entry(file: Path, build_dir: Path, flags: list[str]) -> dict:
    args = ["cl.exe"] + flags + ["/c", fwd(file)]
    return {
        "directory": fwd(build_dir),
        "arguments": args,
        "file": fwd(file),
    }


def main():
    if len(sys.argv) > 1:
        repo = Path(sys.argv[1]).resolve()
    else:
        repo = Path(__file__).resolve().parent.parent

    src = repo / "src"
    tests_dir = repo / "tests"
    glad_src = repo / "shared_deps" / "glad" / "src"
    build_dir = repo / "build"

    common_flags = [
        "/nologo", "/utf-8", "/Od", "/MTd",
        "/fp:fast", "/fp:except-", "/GR-", "/EHa-",
        "/Z7", "/Oi", "/WX", "/W4",
        "/wd4201", "/wd4100", "/wd4189", "/wd4505", "/wd4127", "/wd4190",
        "/D_CRT_SECURE_NO_WARNINGS", "/FC",
        "/std:c++20", "/arch:AVX2", "/Zc:preprocessor",
        "/DHOMEMADE_DEBUG=1", "/DHOMEMADE_SLOW=1", "/DHOMEMADE_WIN32=1",
    ]

    base_includes = [f"/I{fwd(src)}", f"/I{fwd(repo / 'include')}"]
    glad_include = f"/I{fwd(repo / 'shared_deps' / 'glad' / 'include')}"

    test_flags = common_flags + [
        "/DENGINE_TEST",
        "/DDOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS",
        "/wd4611", "/wd4702",
    ]

    entries = []

    for cpp_file in src.rglob("*.cpp"):
        includes = base_includes
        if "opengl" in cpp_file.name.lower():
            includes = base_includes + [glad_include]
        entries.append(make_entry(
            cpp_file, build_dir, common_flags + includes))

    for cpp_file in tests_dir.rglob("*.cpp"):
        entries.append(make_entry(cpp_file, build_dir,
                       test_flags + base_includes))

    for c_file in glad_src.glob("*.c"):
        entries.append(make_entry(c_file, build_dir,
                       common_flags + base_includes + [glad_include]))

    out = repo / "compile_commands.json"
    with open(out, "w") as f:
        json.dump(entries, f, indent=2)

    print(f"Wrote {out} ({len(entries)} entries)")


if __name__ == "__main__":
    main()
