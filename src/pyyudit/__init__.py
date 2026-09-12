"""pyyudit — cross-platform Python binding to Yudit's Unicode shaping engine.

``pyyudit`` wraps Yudit's standalone shaping engine for complex scripts
(Indic, Arabic, Thai, Lao, Tibetan, Jamo) into a cross-platform Python wheel
with per-step trace output.

Typical usage::

    import pyyudit

    font_bytes = open("NotoSansDevanagari.ttf", "rb").read()
    glyphs = pyyudit.shape(font_bytes, "नमस्ते")
    for g in glyphs:
        print(g.gid, g.cl, g.ax)

    with pyyudit.YuditFont.from_path("NotoSansDevanagari.ttf") as f:
        traced = f.shape_trace("नमस्ते")
        for stage in traced.stages:
            print(stage.m, len(stage.glyphs))

Pure-Python helpers (:func:`upem_from_ttf`, :func:`has_table`) work even
when no native library is installed.
"""

from __future__ import annotations

from typing import TYPE_CHECKING

from ._errors import FontError, LibraryNotFound, ShapingError, YuditError
from ._font_meta import FontSource, has_table, read_font_bytes, upem_from_ttf
from ._loader import configure, is_available, library_info, library_path
from ._types import GlyphRecord, ShapedText, ShapedTrace, TraceStage
from ._version import __version__, version_tuple

if TYPE_CHECKING:
    from ._font import YuditFont
    from ._shaper import shape, shape_trace

__all__ = [
    "FontError",
    "FontSource",
    "GlyphRecord",
    "LibraryNotFound",
    "ShapedText",
    "ShapedTrace",
    "ShapingError",
    "TraceStage",
    "YuditError",
    "YuditFont",
    "__version__",
    "configure",
    "has_table",
    "is_available",
    "library_info",
    "library_path",
    "read_font_bytes",
    "upem_from_ttf",
    "version_tuple",
]

_LIB_HINT = (
    "yudit_shaper native library not available. Build it with CMake "
    "(see README.md) and place the shared library in pyyudit/_lib/ "
    "or set PYYUDIT_LIBRARY_PATH."
)


if is_available():
    from ._font import YuditFont
    from ._shaper import shape, shape_trace

    __all__.extend(["shape", "shape_trace"])
else:  # pragma: no cover

    class _UnavailableFont:
        """Placeholder when the native library is unavailable."""

        def __init__(self, *a: object, **kw: object) -> None:
            raise LibraryNotFound(_LIB_HINT)

        def __enter__(self) -> _UnavailableFont:
            return self

        def __exit__(self, *exc: object) -> None:
            return None

        @classmethod
        def from_bytes(cls, data: bytes | bytearray = b"") -> _UnavailableFont:
            return cls(data)

        @classmethod
        def from_path(cls, path: object = ".") -> _UnavailableFont:
            return cls(path)

    YuditFont = _UnavailableFont  # type: ignore[misc, assignment]

    def shape(font: FontSource, text: str, **kwargs: object) -> list[GlyphRecord]:  # type: ignore[no-redef]
        raise LibraryNotFound(_LIB_HINT)

    def shape_trace(font: FontSource, text: str, **kwargs: object) -> ShapedTrace:  # type: ignore[no-redef]
        raise LibraryNotFound(_LIB_HINT)
