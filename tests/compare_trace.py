"""Compare HarfBuzz trace vs pyyudit trace on Devanagari text.

Two goals:
1. Verify trace format matches babelsoft convention
2. Compare lookup count and feature sequence
"""

# pyright: reportAttributeAccessIssue=false, reportMissingImports=false

import pyharfrust2

import pyyudit

FONT_PATH = r"D:\Github\pyyudit\tests\fonts\NotoSansDevanagari-Regular.ttf"
TEXT = "\u0928\u092e\u0938\u094d\u0924\u0947"  # नमस्ते

with open(FONT_PATH, "rb") as f:
    font_data = f.read()

# ── HarfBuzz trace ────────────────────────────────────────────────────────
print("=" * 72)
print("HarfBuzz (pyharfrust2) trace:")
print("=" * 72)

hb_trace = pyharfrust2.shape_trace(font_data, TEXT, script="deva", language="hi")

hb_lookups = []
for i, stage in enumerate(hb_trace):
    eff = "*" if stage["effective"] else " "
    m = stage["m"]
    n = len(stage["glyphs"])
    # Parse lookup info from message: "start lookup N feature 'xxxx'"
    lookup_info = ""
    if "lookup" in m:
        hb_lookups.append(m)
        # Extract feature name
        if "feature" in m:
            feat = m.split("feature")[-1].strip().strip("'")
            lookup_info = f"  ← feature '{feat}'"

    print(f"  [{i:2d}] {eff} {m:50s} {n:4d} glyphs{lookup_info}")

print(f"\n  Total stages: {len(hb_trace)}")
print(f"  Total effective: {sum(1 for s in hb_trace if s['effective'])}")
print(f"  HB lookups (substitutions): {len(hb_lookups)}")
for lk in hb_lookups:
    print(f"    {lk}")

# ── pyyudit trace ─────────────────────────────────────────────────────────
print()
print("=" * 72)
print("pyyudit trace:")
print("=" * 72)

with pyyudit.YuditFont.from_path(FONT_PATH) as f:
    yt_result = f.shape_trace(TEXT)

yt_lookups = []
for i, stage in enumerate(yt_result.stages):
    eff = "*" if stage.effective else " "
    m = stage.m
    n = len(stage.glyphs)
    # In yudit, each gsub/gpos feature name IS the lookup
    if m in (
        "nukt",
        "akhn",
        "rphf",
        "blwf",
        "half",
        "pstf",
        "vatu",
        "pres",
        "abvs",
        "blws",
        "psts",
        "haln",
        "abvm",
        "blwm",
        "dist",
        "kern",
        "mark",
        "mkmk",
    ):
        yt_lookups.append(m)
        lookup_info = "  ← GSUB/GPOS feature"
    elif m == "clean":
        lookup_info = "  ← post-GSUB cleanup"
    elif m == "gpos_init":
        lookup_info = ""
    elif m == "gpos_final":
        lookup_info = "  ← final positioning"
    else:
        lookup_info = ""

    print(f"  [{i:2d}] {eff} {m:16s} {n:4d} glyphs{lookup_info}")

print(f"\n  Total stages: {len(yt_result.stages)}")
print(f"  Total effective: {sum(1 for s in yt_result.stages if s.effective)}")
print(f"  yudit lookups (features): {len(yt_lookups)}")
for lk in yt_lookups:
    print(f"    {lk}")

# ── Final result comparison ───────────────────────────────────────────────
print()
print("=" * 72)
print("RESULT COMPARISON:")
print("=" * 72)

hb_buf = pyharfrust2.shape_trace(font_data, TEXT, script="deva", language="hi")
# Get final glyphs from last effective stage
hb_final = hb_trace[-1]["glyphs"]
yt_final = yt_result.final_glyphs

print(f"  {'':4s}  {'HarfBuzz':>20s}    {'pyyudit':>20s}")
print(f"  {'':4s}  {'glyphs':>10s} {'advance':>10s}  {'glyphs':>10s} {'advance':>10s}")
hb_adv = sum(g["ax"] for g in hb_final)
yt_adv = yt_result.advance_x
print(f"  {'':4s}  {len(hb_final):>10d} {hb_adv:>10d}  {len(yt_final):>10d} {yt_adv:>10d}")

print()
print("  Per-glyph:")
max_g = max(len(hb_final), len(yt_final))
for i in range(max_g):
    hb_s = f"gid={hb_final[i]['g']:4d} adv={hb_final[i]['ax']:6d}" if i < len(hb_final) else "    (none)        "
    yt_s = f"gid={yt_final[i].g:4d} adv={yt_final[i].ax:6d}" if i < len(yt_final) else "    (none)        "
    match = "OK" if (i < len(hb_final) and i < len(yt_final) and hb_final[i]["g"] == yt_final[i].g) else "  "
    print(f"  [{i:2d}]  {hb_s}  {yt_s}  {match}")

print()
print("  Advance match:", "YES ✅" if hb_adv == yt_adv else f"NO ❌ ({hb_adv} vs {yt_adv})")
print(
    "  Glyph count match:",
    "YES ✅" if len(hb_final) == len(yt_final) else f"NO ❌ ({len(hb_final)} vs {len(yt_final)})",
)
