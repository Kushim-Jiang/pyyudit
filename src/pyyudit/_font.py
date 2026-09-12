"""YuditFont — a loaded font ready for shaping."""

from __future__ import annotations

import ctypes
from pathlib import Path
from typing import Any

from ._binding import _lib
from ._errors import FontError, LibraryNotFound
from ._font_meta import read_font_bytes
from ._types import GlyphRecord, ShapedText, ShapedTrace, TraceStage

__all__ = ["YuditFont"]


class YuditFont:
    """A TrueType/OpenType font loaded by Yudit's shaping engine.

    Usage::

        font = YuditFont.from_path("NotoSansDevanagari.ttf")
        result = font.shape_trace("नमस्ते")
        for stage in result.stages:
            print(stage.m, len(stage.glyphs))
    """

    def __init__(self, data: bytes, *, face_index: int = 0) -> None:
        if not _lib:
            raise LibraryNotFound("yudit_shaper native library not available.")

        self._handle = ctypes.c_void_p()
        buf = (ctypes.c_uint8 * len(data))(*data)
        try:
            _lib.yudit_font_open(buf, len(data), face_index, ctypes.byref(self._handle))
        except RuntimeError as exc:
            raise FontError(str(exc)) from exc

        if not self._handle:
            raise FontError("yudit_font_open returned NULL")

        self._data = data  # prevent GC of the buffer

    @classmethod
    def from_path(cls, path: str | Path) -> YuditFont:
        """Load a font from a file path."""
        return cls(read_font_bytes(path))

    @classmethod
    def from_bytes(cls, data: bytes) -> YuditFont:
        """Load a font from raw bytes."""
        return cls(data)

    def close(self) -> None:
        """Release the native font handle."""
        if self._handle:
            _lib.yudit_font_close(self._handle)
            self._handle = ctypes.c_void_p()

    def __enter__(self) -> YuditFont:
        return self

    def __exit__(self, *exc: object) -> None:
        self.close()

    @property
    def upem(self) -> int:
        """Units per em."""
        return _lib.yudit_font_upem(self._handle)

    @property
    def glyph_count(self) -> int:
        """Number of glyphs in the font."""
        return _lib.yudit_font_glyph_count(self._handle)

    @property
    def name(self) -> str:
        """Font name."""
        n = _lib.yudit_font_name(self._handle)
        return n.decode("utf-8", errors="replace") if n else ""

    # ── Shaping ────────────────────────────────────────────────────────────

    def shape(self, text: str) -> ShapedText:
        """Shape text and return positioned glyphs (no trace)."""
        result = self.shape_trace(text)
        return ShapedText(
            glyphs=result.final_glyphs,
            advance_x=result.advance_x,
            advance_y=result.advance_y,
            text=text,
            engine="yudit",
        )

    def shape_trace(self, text: str) -> ShapedTrace:
        """Shape text and return positioned glyphs with per-step trace."""
        if not text:
            from ._types import ShapedTrace

            return ShapedTrace(
                stages=(),
                final_glyphs=(),
                advance_x=0,
                advance_y=0,
                text=text,
                engine="yudit",
            )

        # Encode text as UTF-32
        encoded = text.encode("utf-32-le")
        n_codepoints = len(encoded) // 4
        # Convert bytes to c_uint32 array via cast
        import array

        arr = array.array("I", encoded)
        buf = (ctypes.c_uint32 * n_codepoints)(*arr)

        result_handle = ctypes.c_void_p()
        try:
            _lib.yudit_shape(self._handle, buf, n_codepoints, ctypes.byref(result_handle))
        except RuntimeError as exc:
            from ._errors import ShapingError

            raise ShapingError(str(exc)) from exc

        try:
            return self._extract_result(result_handle, text)
        finally:
            _lib.yudit_result_free(result_handle)

    def _extract_result(self, handle: ctypes.c_void_p, text: str) -> ShapedTrace:
        """Extract glyphs and trace from a native result handle."""
        # Final glyphs
        glyph_count = _lib.yudit_result_glyph_count(handle)
        glyphs_ptr = _lib.yudit_result_glyphs(handle)
        final_glyphs = self._parse_glyphs(glyphs_ptr, glyph_count)

        # Trace stages
        trace_count = _lib.yudit_result_trace_count(handle)
        stages: list[TraceStage] = []
        for i in range(trace_count):
            stage_ptr = _lib.yudit_result_trace_stage(handle, i)
            if not stage_ptr:
                continue
            stage = stage_ptr.contents
            msg = stage.message.decode("utf-8", errors="replace") if stage.message else ""
            stage_glyphs = self._parse_glyphs(stage.glyphs, stage.glyph_count)
            stages.append(
                TraceStage(
                    m=msg,
                    glyphs=tuple(stage_glyphs),
                    depth=stage.depth,
                    effective=bool(stage.effective),
                )
            )

        advance_x = _lib.yudit_result_width(handle)

        return ShapedTrace(
            stages=tuple(stages),
            final_glyphs=tuple(final_glyphs),
            advance_x=advance_x,
            advance_y=0,
            text=text,
            engine="yudit",
        )

    @staticmethod
    def _parse_glyphs(ptr: Any, count: int) -> list[GlyphRecord]:
        """Parse a native glyph array into Python GlyphRecords."""
        glyphs: list[GlyphRecord] = []
        for i in range(count):
            g = ptr[i]
            glyphs.append(
                GlyphRecord(
                    g=g.glyph_id,
                    cl=g.cluster,
                    dx=g.x,
                    dy=g.y,
                    ax=g.width,
                    ay=0,
                    flags=0,
                )
            )
        return glyphs
