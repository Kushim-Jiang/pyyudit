"""Basic pyyudit usage examples."""

from __future__ import annotations

import pyyudit


def main() -> None:
    # ── Check availability ─────────────────────────────────────────────
    print(f"pyyudit version: {pyyudit.__version__}")
    print(f"Native library:  {'available' if pyyudit.is_available() else 'NOT available'}")
    print(f"Library info:    {pyyudit.library_info()}")

    if not pyyudit.is_available():
        print("\nNative library not found. Pure-Python helpers still work:")
        null_bytes = b"\x00" * 100
        print(f"  upem_from_ttf:  {pyyudit.upem_from_ttf(null_bytes)}")
        return

    # ── One-shot shaping ───────────────────────────────────────────────
    # Replace with a real font path
    import sys

    if len(sys.argv) < 3:
        print("\nUsage: python examples/basic.py <font.ttf> <text>")
        print("Example: python examples/basic.py NotoSansDevanagari.ttf नमस्ते")
        return

    font_path = sys.argv[1]
    text = sys.argv[2]

    print(f"\nShaping '{text}' with {font_path}:")

    # Simple shaping
    glyphs = pyyudit.shape(font_path, text)
    print(f"\n  Simple: {len(glyphs)} glyphs")
    for g in glyphs:
        print(f"    gid={g.g:4d}  cl={g.cl:2d}  adv={g.ax:6d}  off=({g.dx},{g.dy})")

    # Shaping with trace
    print("\n  Trace:")
    result = pyyudit.shape_trace(font_path, text)
    for i, stage in enumerate(result.stages):
        marker = "*" if stage.effective else " "
        print(f"    [{i:2d}] {marker} {stage.m:12s}  {len(stage.glyphs):4d} glyphs  depth={stage.depth}")

    print(f"\n  Total advance: {result.advance_x} units")
    print(f"  Final glyphs:  {len(result.final_glyphs)}")

    # ── Reusable font object ───────────────────────────────────────────
    print("\n  Reusable font object:")
    with pyyudit.YuditFont.from_path(font_path) as f:
        print(f"    Name:     {f.name}")  # pyright: ignore[reportAttributeAccessIssue]
        print(f"    upem:     {f.upem}")  # pyright: ignore[reportAttributeAccessIssue]
        print(f"    glyphs:   {f.glyph_count}")  # pyright: ignore[reportAttributeAccessIssue]

        r = f.shape(text)  # pyright: ignore[reportAttributeAccessIssue]
        print(f"    shaped:   {len(r.glyphs)} glyphs, advance={r.advance_x}")


if __name__ == "__main__":
    main()
