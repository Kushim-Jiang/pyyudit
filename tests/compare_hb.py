"""Compare pyyudit vs HarfBuzz shaping on Devanagari (Indic) text."""

# pyright: reportAttributeAccessIssue=false, reportMissingImports=false
import uharfbuzz as hb

import pyyudit

FONT_PATH = r"D:\Github\pyyudit\tests\fonts\NotoSansDevanagari-Regular.ttf"
TEXT = "\u0928\u092e\u0938\u094d\u0924\u0947"  # नमस्ते

print("=" * 60)
print(f"Font: {FONT_PATH}")
print(f"Text: {TEXT}")
cps = [f"U+{ord(c):04X}" for c in TEXT]
print(f"Codepoints: {cps}")
print()

# ── HarfBuzz shaping ──────────────────────────────────────────────────────
print("-" * 60)
print("HarfBuzz shaping:")
print("-" * 60)

with open(FONT_PATH, "rb") as f:
    font_data = f.read()

face = hb.Face(font_data)  # pyright: ignore[reportAttributeAccessIssue]
font = hb.Font(face)
buf = hb.Buffer()
buf.add_str(TEXT)
buf.guess_segment_properties()

hb.shape(font, buf, {})

infos = buf.glyph_infos
positions = buf.glyph_positions

hb_glyphs = []
for i, (info, pos) in enumerate(zip(infos, positions)):
    hb_glyphs.append(
        {
            "g": info.codepoint,
            "cl": info.cluster,
            "ax": pos.x_advance,
            "ay": pos.y_advance,
            "dx": pos.x_offset,
            "dy": pos.y_offset,
        }
    )
    print(
        f"  [{i:2d}] gid={info.codepoint:4d}  cl={info.cluster:2d}"
        f"  adv=({pos.x_advance:6d},{pos.y_advance:4d})"
        f"  off=({pos.x_offset:5d},{pos.y_offset:5d})"
    )

print(f"Total glyphs: {len(hb_glyphs)}")

# ── HarfBuzz with trace ───────────────────────────────────────────────────
print()
print("-" * 60)
print("HarfBuzz trace:")
print("-" * 60)

# Use pyharfrust2 if available for trace, else skip
try:
    import pyharfrust2

    hb_traces = pyharfrust2.shape_trace(font_data, TEXT, script="deva", language="hi")
    for stage in hb_traces:
        eff = "*" if stage["effective"] else " "
        print(f"  {eff} {stage['m']:40s}  {len(stage['glyphs']):4d} glyphs")
except Exception as e:
    print(f"  (trace unavailable: {e})")
    # Fallback: just shape without trace
    buf2 = hb.Buffer()
    buf2.add_str(TEXT)
    buf2.guess_segment_properties()
    hb.shape(font, buf2, {})
    print(f"  (shaped without trace — {len(buf2.glyph_infos)} glyphs)")

# ── pyyudit shaping ───────────────────────────────────────────────────────
print()
print("-" * 60)
print("pyyudit shaping:")
print("-" * 60)

with pyyudit.YuditFont.from_path(FONT_PATH) as f:
    result = f.shape_trace(TEXT)

    print(f"Stages: {len(result.stages)}")
    for i, s in enumerate(result.stages):
        eff = "*" if s.effective else " "
        print(f"  [{i:2d}] {eff} {s.m:16s}  {len(s.glyphs):4d} glyphs")
        # Show glyph details for key stages
        show = s.m in ("input", "shaped", "reorder", "gindex", "decompose")
        show = show or s.m in (
            "half",
            "pres",
            "abvs",
            "blws",
            "haln",
            "psts",
            "clean",
            "gpos_final",
        )
        if show:
            for _j, g in enumerate(s.glyphs):
                cp = f"U+{g.cl:04X}" if g.cl < 0x10000 else f"U+{g.cl:X}"
                print(f"        gid={g.g:4d} cp={cp:8s} adv={g.ax:6d} off=({g.dx:5d},{g.dy:5d})")

    print()
    print("Final glyphs:")
    for i, g in enumerate(result.final_glyphs):
        cp = f"U+{g.cl:04X}" if g.cl < 0x10000 else f"U+{g.cl:X}"
        print(f"  [{i:2d}] gid={g.g:4d} cp={cp:8s}" f" adv=({g.ax:6d},{g.ay:4d}) off=({g.dx:5d},{g.dy:5d})")
    print(f"Total glyphs: {len(result.final_glyphs)}")
    print(f"Total advance: {result.advance_x}")

# ── Side-by-side comparison ───────────────────────────────────────────────
print()
print("=" * 60)
print("COMPARISON:")
print("=" * 60)
print(f"  {'':4s}  {'HarfBuzz':>20s}    {'pyyudit':>20s}")
print(f"  {'':4s}  {'glyphs':>10s} {'advance':>10s}  {'glyphs':>10s} {'advance':>10s}")
hb_adv = sum(g["ax"] for g in hb_glyphs)
yt_adv = result.advance_x
print(f"  {'':4s}  {len(hb_glyphs):>10d} {hb_adv:>10d}" f"  {len(result.final_glyphs):>10d} {yt_adv:>10d}")
print()
print("  Per-glyph comparison:")
max_g = max(len(hb_glyphs), len(result.final_glyphs))
for i in range(max_g):
    hb_s = ""
    yt_s = ""
    if i < len(hb_glyphs):
        g = hb_glyphs[i]
        hb_s = f"gid={g['g']:4d} adv={g['ax']:6d} off=({g['dx']:5d},{g['dy']:5d})"
    else:
        hb_s = "    (none)           "
    if i < len(result.final_glyphs):
        g = result.final_glyphs[i]
        yt_s = f"gid={g.g:4d} adv={g.ax:6d} off=({g.dx:5d},{g.dy:5d})"
    else:
        yt_s = "    (none)           "
    match = (
        "OK"
        if (i < len(hb_glyphs) and i < len(result.final_glyphs) and hb_glyphs[i]["g"] == result.final_glyphs[i].g)
        else "!!"
    )
    print(f"  [{i:2d}]  {hb_s}  {yt_s}  {match}")
