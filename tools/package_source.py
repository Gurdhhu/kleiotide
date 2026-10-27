#!/usr/bin/env python3
"""Create a source ZIP without inheriting host clocks or local time zones."""

import argparse
from pathlib import Path
import re
import zipfile


def main():
    source = Path(__file__).resolve().parents[1]
    version = re.search(r"project\(Kleiotide VERSION ([\d.]+)",
                        (source / "CMakeLists.txt").read_text()).group(1)
    release = version
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path, help="Destination ZIP file")
    args = parser.parse_args()
    output = args.output.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    files = []
    for path in sorted(source.rglob("*")):
        relative = path.relative_to(source)
        if any(part.startswith(".") or part.startswith("build")
               or part == "__pycache__" for part in relative.parts):
            continue
        if (not path.is_file() or path.is_symlink() or path.resolve() == output
                or path.name.endswith((".user", ".pyc", ".zip"))):
            continue
        files.append((path, relative))
    with zipfile.ZipFile(output, "w", compression=zipfile.ZIP_DEFLATED,
                         compresslevel=9) as archive:
        for path, relative in files:
            # ZIP dates have no time zone. A fixed historical date prevents
            # extracted CMake inputs from appearing newer than build.ninja.
            info = zipfile.ZipInfo(f"Kleiotide_{release}/{relative.as_posix()}",
                                   date_time=(2000, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.create_system = 3
            info.external_attr = 0o100644 << 16
            archive.writestr(info, path.read_bytes(), compresslevel=9)
    print(f"Packaged {len(files)} source files: {output}")


if __name__ == "__main__":
    main()
