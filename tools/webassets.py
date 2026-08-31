#!/usr/bin/env python3
"""Sync the web UI sources in web/ with the embedded header src/web_assets.h.

The tracked source of truth is web/index.html, web/style.css, web/app.js.
`embed` regenerates src/web_assets.h from them; `extract` goes the other way
(useful for inspecting an old header revision). `check` verifies the header
matches the sources (used by CI).

Usage:
  python tools/webassets.py embed
  python tools/webassets.py extract [--out-dir DIR]
  python tools/webassets.py check
"""

import argparse
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
HEADER = REPO_ROOT / "src" / "web_assets.h"
WEB_DIR = REPO_ROOT / "web"

ASSETS = [
    ("INDEX_HTML", "index.html"),
    ("STYLE_CSS", "style.css"),
    ("APP_JS", "app.js"),
]

BYTES_PER_LINE = 19


def render_header(blobs):
    lines = [
        "// Auto-generated from web/index.html, web/style.css, web/app.js.",
        "// Regenerate with: python tools/webassets.py embed  (do not edit manually)",
        "#pragma once",
        "",
    ]
    for name, _ in ASSETS:
        data = blobs[name]
        lines.append(f"static const uint8_t {name}[] PROGMEM = {{")
        for i in range(0, len(data), BYTES_PER_LINE):
            chunk = data[i : i + BYTES_PER_LINE]
            lines.append("  " + " ".join(f"0x{b:02X}," for b in chunk))
        lines.append("};")
        lines.append(f"static const size_t {name}_LEN = {len(data)};")
        lines.append("")
    return "\n".join(lines)


def read_sources():
    blobs = {}
    for name, filename in ASSETS:
        path = WEB_DIR / filename
        if not path.is_file():
            sys.exit(f"error: missing source file {path}")
        blobs[name] = path.read_bytes()
    return blobs


def parse_header():
    if not HEADER.is_file():
        sys.exit(f"error: missing header {HEADER}")
    text = HEADER.read_text()
    blobs = {}
    for name, _ in ASSETS:
        m = re.search(
            rf"const uint8_t {name}\[\] PROGMEM = \{{(.*?)\}};", text, re.S
        )
        if not m:
            sys.exit(f"error: array {name} not found in {HEADER}")
        data = bytes(int(x, 16) for x in re.findall(r"0x([0-9a-fA-F]{2})", m.group(1)))
        lm = re.search(rf"const size_t {name}_LEN = (\d+);", text)
        if not lm or int(lm.group(1)) != len(data):
            sys.exit(f"error: length mismatch for {name} in {HEADER}")
        blobs[name] = data
    return blobs


def cmd_embed(_args):
    blobs = read_sources()
    HEADER.write_text(render_header(blobs))
    total = sum(len(b) for b in blobs.values())
    print(f"wrote {HEADER} ({total} asset bytes)")


def cmd_extract(args):
    out_dir = Path(args.out_dir) if args.out_dir else WEB_DIR
    out_dir.mkdir(parents=True, exist_ok=True)
    blobs = parse_header()
    for name, filename in ASSETS:
        path = out_dir / filename
        path.write_bytes(blobs[name])
        print(f"wrote {path} ({len(blobs[name])} bytes)")


def cmd_check(_args):
    src = read_sources()
    hdr = parse_header()
    stale = [name for name, _ in ASSETS if src[name] != hdr[name]]
    if stale:
        sys.exit(
            "error: src/web_assets.h is stale for: "
            + ", ".join(stale)
            + "\nrun: python tools/webassets.py embed"
        )
    print("src/web_assets.h matches web/ sources")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("embed").set_defaults(func=cmd_embed)
    p_extract = sub.add_parser("extract")
    p_extract.add_argument("--out-dir", default=None)
    p_extract.set_defaults(func=cmd_extract)
    sub.add_parser("check").set_defaults(func=cmd_check)
    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
