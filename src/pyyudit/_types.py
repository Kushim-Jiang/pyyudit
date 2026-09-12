"""Type definitions for pyyudit."""

from __future__ import annotations

import os
from typing import NamedTuple, Union

# Type aliases
StrPath = Union[str, os.PathLike[str]]


class GlyphRecord(NamedTuple):
    """A single shaped glyph with positioning."""

    g: int  # glyph index in font
    cl: int  # cluster index (0-based)
    dx: int  # x offset (font units)
    dy: int  # y offset (font units)
    ax: int  # x advance (font units)
    ay: int  # y advance (usually 0)
    flags: int  # reserved


class TraceStage(NamedTuple):
    """A single shaping step in the trace pipeline."""

    m: str  # message, e.g. "decompose", "half", "abvm"
    glyphs: tuple[GlyphRecord, ...]  # buffer snapshot at this stage
    depth: int  # nesting depth (0 = top-level)
    effective: bool  # did the buffer change from previous stage?


class ShapedText(NamedTuple):
    """Result of a shaping operation (without trace)."""

    glyphs: tuple[GlyphRecord, ...]
    advance_x: int
    advance_y: int
    text: str
    engine: str


class ShapedTrace(NamedTuple):
    """Result of a shaping operation with full trace."""

    stages: tuple[TraceStage, ...]
    final_glyphs: tuple[GlyphRecord, ...]
    advance_x: int
    advance_y: int
    text: str
    engine: str
