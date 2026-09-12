"""Pure-Python font metadata helpers (no native library required)."""

from __future__ import annotations

import struct
from pathlib import Path
from typing import Union

__all__ = ["FontSource", "has_table", "read_font_bytes", "upem_from_ttf"]


FontSource = Union[bytes, bytearray, str, Path]


def read_font_bytes(source: FontSource) -> bytes:
    """Read font data from bytes or a file path."""
    if isinstance(source, (bytes, bytearray)):
        return bytes(source)
    path = Path(source)
    if not path.is_file():
        raise FileNotFoundError(f"Font file not found: {path}")
    return path.read_bytes()


def _read_u16(data: bytes, offset: int) -> int:
    return struct.unpack_from(">H", data, offset)[0]


def _read_u32(data: bytes, offset: int) -> int:
    return struct.unpack_from(">I", data, offset)[0]


def upem_from_ttf(font_bytes: bytes) -> int:
    """Read units-per-em from the head table of a TrueType/OpenType font."""
    try:
        num_tables = _read_u16(font_bytes, 4)
        for i in range(num_tables):
            offset = 12 + i * 16
            tag = font_bytes[offset : offset + 4]
            table_offset = _read_u32(font_bytes, offset + 8)
            if tag == b"head" and table_offset + 18 + 2 <= len(font_bytes):
                return _read_u16(font_bytes, table_offset + 18)
    except (struct.error, IndexError):
        pass
    return 1000  # default


def has_table(font_bytes: bytes, tag: str) -> bool:
    """Check if a font contains a specific OpenType table."""
    tag_bytes = tag.encode("ascii")[:4]
    try:
        num_tables = _read_u16(font_bytes, 4)
        for i in range(num_tables):
            offset = 12 + i * 16
            t = font_bytes[offset : offset + 4]
            if t == tag_bytes:
                return True
    except (struct.error, IndexError):
        pass
    return False


def is_graphite_font(font_bytes: bytes) -> bool:
    """Check if a font has Graphite tables (silf/sill/glat/gloc)."""
    return any(has_table(font_bytes, t) for t in ("silf", "sill"))


def glyph_count_from_ttf(font_bytes: bytes) -> int:
    """Read the number of glyphs from the maxp table."""
    try:
        num_tables = _read_u16(font_bytes, 4)
        for i in range(num_tables):
            offset = 12 + i * 16
            tag = font_bytes[offset : offset + 4]
            table_offset = _read_u32(font_bytes, offset + 8)
            if tag == b"maxp" and table_offset + 6 <= len(font_bytes):
                return _read_u16(font_bytes, table_offset + 4)
    except (struct.error, IndexError):
        pass
    return 0
