#!/usr/bin/env python3
"""Generate a C++ pinyin map from pinyin-data."""

from __future__ import annotations

import argparse
import datetime as dt
import re
import unicodedata
from pathlib import Path
from typing import Dict, Iterable, List

LINE_RE = re.compile(r"^U\+([0-9A-F]+):\s*([^#]+)")


def normalize_py(raw: str) -> str:
    py = raw.strip().split(",", 1)[0].strip()
    py = unicodedata.normalize("NFD", py)

    # Remove tone/diacritic marks and keep plain letters only.
    stripped = []
    for ch in py:
        if unicodedata.category(ch) == "Mn":
            continue
        stripped.append(ch)

    out = "".join(stripped).upper()
    out = out.replace("Ü", "V")
    out = "".join(ch for ch in out if "A" <= ch <= "Z")
    return out


def parse_pinyin_data(path: Path) -> Dict[int, str]:
    result: Dict[int, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        m = LINE_RE.match(line)
        if not m:
            continue
        cp = int(m.group(1), 16)
        py = normalize_py(m.group(2))
        if py:
            result[cp] = py
    return result


def read_required_chars(path: Path) -> List[int]:
    cps: List[int] = []
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        if len(line) != 1:
            raise ValueError(f"Each non-comment line must contain exactly 1 character: {line!r}")
        cps.append(ord(line))
    return sorted(set(cps))


def all_chars(pinyin_by_cp: Dict[int, str]) -> List[int]:
    return sorted(pinyin_by_cp.keys())


def make_header(cps: Iterable[int], pinyin_by_cp: Dict[int, str], source_rel: str) -> str:
    now = dt.datetime.now(dt.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    lines: List[str] = []
    lines.append("#pragma once")
    lines.append("")
    lines.append("#include <array>")
    lines.append("#include <cstdint>")
    lines.append("")
    lines.append("namespace phonefinder {")
    lines.append("")
    lines.append("struct PinyinToken {")
    lines.append("  const char* initials;")
    lines.append("  const char* full;")
    lines.append("};")
    lines.append("")
    lines.append("struct PinyinMapEntry {")
    lines.append("  char32_t cp;")
    lines.append("  PinyinToken token;")
    lines.append("};")
    lines.append("")
    lines.append(f"// Generated from {source_rel} at {now}")
    rows = []
    for cp in cps:
        full = pinyin_by_cp.get(cp)
        if not full:
            raise ValueError(f"Missing pinyin-data entry for U+{cp:04X}")
        ini = full[0]
        rows.append((cp, ini, full))

    lines.append(f"inline constexpr std::array<PinyinMapEntry, {len(rows)}> kPinyinMapData = {{{{")
    for cp, ini, full in rows:
        lines.append(f"    {{0x{cp:04X}, {{\"{ini}\", \"{full}\"}}}},")
    lines.append("}};")
    lines.append("")
    lines.append("}  // namespace phonefinder")
    lines.append("")
    return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--pinyin-data", required=True, type=Path)
    parser.add_argument("--required-chars", type=Path)
    parser.add_argument("--out", required=True, type=Path)
    args = parser.parse_args()

    pinyin_by_cp = parse_pinyin_data(args.pinyin_data)
    cps: List[int]
    if args.required_chars is not None:
        cps = read_required_chars(args.required_chars)
    else:
        cps = all_chars(pinyin_by_cp)

    text = make_header(cps, pinyin_by_cp, str(args.pinyin_data))
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(text, encoding="utf-8")


if __name__ == "__main__":
    main()
