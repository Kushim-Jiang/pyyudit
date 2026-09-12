"""Tests for pyyudit — pure-Python helpers (no native library required)."""

from __future__ import annotations

import pytest

import pyyudit
from pyyudit._font_meta import has_table, read_font_bytes, upem_from_ttf


class TestVersion:
    def test_version_string(self) -> None:
        assert isinstance(pyyudit.__version__, str)
        parts = pyyudit.__version__.split(".")
        assert len(parts) >= 2

    def test_version_tuple(self) -> None:
        assert isinstance(pyyudit.version_tuple, tuple)
        assert all(isinstance(v, int) for v in pyyudit.version_tuple)


class TestTypes:
    def test_glyph_record_fields(self) -> None:
        g = pyyudit.GlyphRecord(g=1, cl=0, dx=0, dy=0, ax=600, ay=0, flags=0)
        assert g.g == 1
        assert g.ax == 600

    def test_trace_stage_fields(self) -> None:
        stage = pyyudit.TraceStage(m="test", glyphs=(), depth=0, effective=True)
        assert stage.m == "test"
        assert stage.effective is True


class TestFontMeta:
    def test_read_font_bytes_from_bytes(self) -> None:
        data = b"\x00" * 100
        assert read_font_bytes(data) == data

    def test_read_font_bytes_from_path(self, tmp_path: object) -> None:
        from pathlib import Path

        p = Path(str(tmp_path)) / "test.ttf"
        p.write_bytes(b"\x00" * 100)
        assert len(read_font_bytes(p)) == 100

    def test_read_font_bytes_missing(self, tmp_path: object) -> None:
        from pathlib import Path

        with pytest.raises(FileNotFoundError):
            read_font_bytes(Path(str(tmp_path)) / "missing.ttf")

    def test_upem_default(self) -> None:
        """Without a valid head table, returns default 1000."""
        assert upem_from_ttf(b"\x00" * 100) == 1000

    def test_has_table_missing(self) -> None:
        assert not has_table(b"\x00" * 100, "head")


class TestLoader:
    def test_configure(self) -> None:
        pyyudit.configure(None)  # reset

    def test_library_info(self) -> None:
        info = pyyudit.library_info()
        assert "available" in info
        assert "path" in info


class TestErrorHierarchy:
    def test_inheritance(self) -> None:
        assert issubclass(pyyudit.LibraryNotFound, pyyudit.YuditError)
        assert issubclass(pyyudit.FontError, pyyudit.YuditError)
        assert issubclass(pyyudit.ShapingError, pyyudit.YuditError)
