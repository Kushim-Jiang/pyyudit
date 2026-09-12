"""One-shot shaping functions (the main user-facing API)."""

from __future__ import annotations

from ._font_meta import FontSource, read_font_bytes
from ._types import GlyphRecord, ShapedTrace

__all__ = ["shape", "shape_trace"]

_LIB_HINT = (
    "yudit_shaper native library not available. Build it with CMake "
    "(see README.md) and place the shared library in pyyudit/_lib/ "
    "or set PYYUDIT_LIBRARY_PATH."
)


def shape(
    font: FontSource,
    text: str,
    **kwargs: object,
) -> list[GlyphRecord]:
    """Shape text with a font and return positioned glyphs.

    This is a convenience wrapper around :class:`YuditFont`.

    :param font: font bytes or file path.
    :param text: Unicode text to shape.
    :returns: list of :class:`GlyphRecord` with positioning.
    """
    from ._font import YuditFont

    with YuditFont.from_bytes(read_font_bytes(font)) as f:
        result = f.shape(text)
        return list(result.glyphs)


def shape_trace(
    font: FontSource,
    text: str,
    **kwargs: object,
) -> ShapedTrace:
    """Shape text with a font and return the full trace.

    :param font: font bytes or file path.
    :param text: Unicode text to shape.
    :returns: :class:`ShapedTrace` with per-step stages.
    """
    from ._font import YuditFont

    with YuditFont.from_bytes(read_font_bytes(font)) as f:
        return f.shape_trace(text)
