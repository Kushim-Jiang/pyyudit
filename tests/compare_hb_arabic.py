"""Compare pyyudit vs HarfBuzz shaping on Arabic (joining + RTL) text.

Usage:
    python tests/compare_hb_arabic.py [font.ttf] ["text"]

Defaults to an Arabic-capable system font.  Set PYYUDIT_LIBRARY_PATH to point
at a locally built yudit_shaper.dll.
"""

# pyright: reportAttributeAccessIssue=false, reportMissingImports=false
from __future__ import annotations

import os
import sys

import uharfbuzz as hb

import pyyudit

DEFAULT_FONTS = [
    r"C:\Windows\Fonts\arial.ttf",
    r"C:\Windows\Fonts\segoeui.ttf",
    r"C:\Windows\Fonts\tahoma.ttf",
]


def pick_font() -> str:
    if len(sys.argv) > 1:
        return sys.argv[1]
    for path in DEFAULT_FONTS:
        if os.path.isfile(path):
            return path
    raise SystemExit("no Arabic-capable font found; pass one as argv[1]")


FONT_PATH = pick_font()
TEXT = sys.argv[2] if len(sys.argv) > 2 else "\u0628\u0628\u0628 \u0644\u0627 \u0628\u064e"


def fmt(cp: int) -> str:
    return f"U+{cp:04X}" if cp else "--"


def show(label: str, rows: list[dict]) -> None:
    print(f"  {label}")
    for i, r in enumerate(rows):
        print(
            f"    [{i:2d}] gid={r['g']:5d} {fmt(r['cp']):7s} cl={r['cl']:2d}"
            f"  ax={r['ax']:6d} dx={r['dx']:5d} dy={r['dy']:5d}"
        )


print("=" * 72)
print(f"Font: {FONT_PATH}")
print(f"Text: {TEXT}")
print(f"Codepoints: {' '.join(fmt(ord(c)) for c in TEXT)}")
print("=" * 72)

with open(FONT_PATH, "rb") as fh:
    font_data = fh.read()

# ── HarfBuzz ──────────────────────────────────────────────────────────────
hb_face = hb.Face(font_data)  # pyright: ignore[reportAttributeAccessIssue]
hb_font = hb.Font(hb_face)
buf = hb.Buffer()
buf.add_str(TEXT)
buf.direction = "rtl"
buf.script = "Arab"
buf.language = "ar"
hb.shape(hb_font, buf, {})

hb_rows = [
    {
        "g": info.codepoint,
        "cp": 0,
        "cl": info.cluster,
        "ax": pos.x_advance,
        "dx": pos.x_offset,
        "dy": pos.y_offset,
    }
    for info, pos in zip(buf.glyph_infos, buf.glyph_positions)
]

# ── pyyudit ───────────────────────────────────────────────────────────────
with pyyudit.YuditFont.from_bytes(font_data) as font:
    traced = font.shape_trace(TEXT)

yd_rows = [{"g": g.g, "cp": 0, "cl": g.cl, "ax": g.ax, "dx": g.dx, "dy": g.dy} for g in traced.final_glyphs]

print()
print("-" * 72)
print(f"HarfBuzz  ({len(hb_rows)} glyphs, advance {sum(r['ax'] for r in hb_rows)})")
print("-" * 72)
show("visual order (rtl):", hb_rows)

print()
print("-" * 72)
print(f"pyyudit   ({len(yd_rows)} glyphs, advance {traced.advance_x})")
print("-" * 72)
show("visual order:", yd_rows)

print()
print("-" * 72)
print("pyyudit trace")
print("-" * 72)
for stage in traced.stages:
    eff = "*" if stage.effective else " "
    print(f"  {eff} {stage.m:20s} {len(stage.glyphs):3d} glyphs")

# ── glyph id comparison ───────────────────────────────────────────────────
hb_ids = [r["g"] for r in hb_rows]
yd_ids = [r["g"] for r in yd_rows]
print()
print("-" * 72)
print(f"glyph ids   HB: {hb_ids}")
print(f"glyph ids yudit: {yd_ids}")
print(f"identical: {hb_ids == yd_ids}")
