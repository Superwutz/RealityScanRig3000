#!/usr/bin/env python3
"""Build and stage an OTA/web-installer release (cross-platform).

Does the build/stage half of tools/release.ps1 without any git actions:
  1. validate the new version against docs/manifest.json
  2. patch SCANRIG_FW_VERSION / SCANRIG_UI_VERSION in src/main.cpp
  3. sync the app.js cache-buster in web/index.html, regenerate web_assets.h
  4. pio run
  5. copy bootloader/partitions/boot_app0/firmware into docs/
  6. write docs/manifest.json and refresh docs/index.html from the template

Committing and tagging is left to the caller (release.ps1 locally, or
.github/workflows/release.yml).

Usage: python tools/release.py --version v0.7.0
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
ENV_NAME_DEFAULT = "esp32s3"


def fail(msg: str) -> None:
    print(f"error: {msg}", file=sys.stderr)
    sys.exit(1)


def parse_version(v: str) -> tuple[int, int, int]:
    m = re.fullmatch(r"v?(\d+)\.(\d+)\.(\d+)", v.strip())
    if not m:
        fail(f"version must look like v0.7.0 (got {v!r})")
    return (int(m.group(1)), int(m.group(2)), int(m.group(3)))


def patch_file(path: Path, pattern: str, replacement: str, count: int = 1) -> None:
    text = path.read_text(encoding="utf-8")
    new_text, n = re.subn(pattern, replacement, text, count=count)
    if n != count:
        fail(f"{path}: expected {count} match(es) for {pattern!r}, found {n}")
    path.write_text(new_text, encoding="utf-8")


def find_boot_app0() -> Path:
    roots = []
    if os.environ.get("PLATFORMIO_CORE_DIR"):
        roots.append(Path(os.environ["PLATFORMIO_CORE_DIR"]))
    roots.append(Path.home() / ".platformio")
    for core in roots:
        pkg = core / "packages" / "framework-arduinoespressif32" / "tools" / "partitions" / "boot_app0.bin"
        if pkg.is_file():
            return pkg
        pkgs = core / "packages"
        if pkgs.is_dir():
            for hit in pkgs.glob("framework-arduinoespressif32*/tools/partitions/boot_app0.bin"):
                return hit
    fail("boot_app0.bin not found in PlatformIO packages (build once first)")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--version", required=True, help="release version, e.g. v0.7.0")
    parser.add_argument("--env", default=ENV_NAME_DEFAULT)
    args = parser.parse_args()

    version = args.version if args.version.startswith("v") else f"v{args.version}"
    plain = version[1:]
    new_v = parse_version(version)

    manifest_path = REPO / "docs" / "manifest.json"
    current = json.loads(manifest_path.read_text(encoding="utf-8"))
    cur_v = parse_version(current["version"])
    if new_v <= cur_v:
        fail(f"version {version} must be greater than manifest version {current['version']}")

    main_cpp = REPO / "src" / "main.cpp"
    patch_file(main_cpp, r'(#define\s+SCANRIG_FW_VERSION\s+")[^"]+(")', rf"\g<1>{plain}\g<2>")
    patch_file(main_cpp, r'(#define\s+SCANRIG_UI_VERSION\s+")[^"]+(")', rf"\g<1>{plain}\g<2>")
    patch_file(REPO / "web" / "index.html", r'(\./app\.js\?v=)[^"]+(")', rf"\g<1>{plain}\g<2>")
    print(f"patched versions -> {plain}")

    subprocess.run([sys.executable, str(REPO / "tools" / "webassets.py"), "embed"], check=True)

    pio = shutil.which("pio") or shutil.which("platformio")
    if not pio:
        fail("PlatformIO not found on PATH")
    subprocess.run([pio, "run", "-e", args.env], cwd=REPO, check=True)

    build = REPO / ".pio" / "build" / args.env
    docs = REPO / "docs"
    for name, src in [
        ("bootloader.bin", build / "bootloader.bin"),
        ("partitions.bin", build / "partitions.bin"),
        ("boot_app0.bin", find_boot_app0()),
        ("firmware.bin", build / "firmware.bin"),
    ]:
        if not src.is_file():
            fail(f"missing build artifact: {src}")
        shutil.copyfile(src, docs / name)
        print(f"staged docs/{name} ({src.stat().st_size} bytes)")

    manifest = {
        "name": "RealityScanRig3000",
        "version": version,
        "new_install_prompt_erase": True,
        "builds": [
            {
                "chipFamily": "ESP32-S3",
                "parts": [
                    {"path": "bootloader.bin", "offset": 0},
                    {"path": "partitions.bin", "offset": 32768},
                    {"path": "boot_app0.bin", "offset": 57344},
                    {"path": "firmware.bin", "offset": 65536},
                ],
            }
        ],
    }
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    shutil.copyfile(REPO / "deploy" / "web-installer" / "index.html", docs / "index.html")
    print(f"staged docs/manifest.json ({version}) and docs/index.html")
    print("release staging complete - review, commit and tag")


if __name__ == "__main__":
    main()
