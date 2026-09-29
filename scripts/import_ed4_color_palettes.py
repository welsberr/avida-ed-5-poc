#!/usr/bin/env python3
"""Generate the Avida-ED 5 C++ palettes from a supplied v4 colormap.js."""

from __future__ import annotations

import argparse
import re
from pathlib import Path


PALETTES = {
    "GNUPLOT2": "Gnuplot2cmap",
    "VIRIDIS": "ViridisCmap",
    "CUBEHELIX": "cubehelixCmap",
    "CATEGORICAL": "parentColorList",
}
COLOR = re.compile(r"rgb\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)|#([0-9a-fA-F]{6})")


def packed_color(match: re.Match[str]) -> int:
    if match.group(4):
        red, green, blue = (int(match.group(4)[offset : offset + 2], 16) for offset in (0, 2, 4))
    else:
        red, green, blue = (int(match.group(index)) for index in (1, 2, 3))
    return 0xFF000000 | (blue << 16) | (green << 8) | red


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path, help="Path to Avida-ED 4 colormap.js")
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()

    source = args.source.read_text(encoding="utf-8")
    source = re.sub(r"/\*.*?\*/", "", source, flags=re.DOTALL)
    source = re.sub(r"//[^\n]*", "", source)
    arrays: dict[str, list[int]] = {}
    for cpp_name, js_name in PALETTES.items():
        found = re.search(rf"av\.color\.{js_name}\s*=\s*\[(.*?)\];", source, re.DOTALL)
        if not found:
            raise SystemExit(f"Missing av.color.{js_name} in {args.source}")
        values = [packed_color(match) for match in COLOR.finditer(found.group(1))]
        if not values:
            raise SystemExit(f"No colors found for av.color.{js_name}")
        arrays[cpp_name] = values

    lines = [
        "#pragma once",
        "",
        "#include <array>",
        "#include <cstdint>",
        "",
        "// Color values retained from Avida-ED 4 colormap.js for tutorial continuity.",
        "// Original project copyright: Michigan State University; MIT license.",
        "namespace avida_web::legacy_colors {",
    ]
    for name, colors in arrays.items():
        lines.append(f"inline constexpr std::array<uint32_t, {len(colors)}> {name}{{{{")
        for offset in range(0, len(colors), 6):
            lines.append("  " + ", ".join(f"0x{color:08x}U" for color in colors[offset : offset + 6]) + ",")
        lines.append("}};")
        lines.append("")
    lines.append("} // namespace avida_web::legacy_colors")
    args.destination.parent.mkdir(parents=True, exist_ok=True)
    args.destination.write_text("\n".join(lines) + "\n", encoding="utf-8")
    for name, colors in arrays.items():
        print(f"{name}: {len(colors)} colors")


if __name__ == "__main__":
    main()
