"""Quick test of per-lookup trace with fixed feature tags."""

# pyright: reportAttributeAccessIssue=false

import re

import pyyudit

f = pyyudit.YuditFont.from_path(r"tests/fonts/NotoSansDevanagari-Regular.ttf")
result = f.shape_trace("\u0928\u092e\u0938\u094d\u0924\u0947")
f.close()

for i, s in enumerate(result.stages[:40]):
    eff = "*" if s.effective else " "
    print(f"  [{i:2d}] {eff} {s.m:50s}  {len(s.glyphs)} glyphs")

print(f"\n  Total stages: {len(result.stages)}")
lk_count = sum(1 for s in result.stages if "lookup" in s.m)
print(f"  Lookup stages: {lk_count}")
feats = set()
for s in result.stages:
    m = re.search(r"feature '(.+?)'", s.m)
    if m:
        feats.add(m.group(1))
print(f"  Features seen: {sorted(feats)}")
