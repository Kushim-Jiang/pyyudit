"""Tests for pyyudit native shaping (requires compiled library + test fonts)."""

# pyright: reportAttributeAccessIssue=false, reportCallIssue=false

from __future__ import annotations

import os

import pytest

import pyyudit

# Mark all tests in this file as requiring the native library
pytestmark = pytest.mark.native


def _has_native() -> bool:
    return pyyudit.is_available()


def _find_test_font() -> bytes | None:
    """Try to find a test font in common locations."""
    candidates = [
        # Common locations
        os.path.join(os.path.dirname(__file__), "..", "tests", "fonts"),
        os.path.join(os.path.dirname(__file__), "..", "examples"),
    ]
    # Also check FONTDIR env
    font_dir = os.environ.get("PYYUDIT_FONT_DIR", "")
    if font_dir:
        candidates.insert(0, font_dir)

    for d in candidates:
        if not os.path.isdir(d):
            continue
        for f in os.listdir(d):
            if f.lower().endswith((".ttf", ".otf")):
                path = os.path.join(d, f)
                with open(path, "rb") as fh:
                    return fh.read()
    return None


_skip_reason = "native library not available"


@pytest.mark.skipif(not _has_native(), reason=_skip_reason)
class TestYuditFont:
    def test_open_close(self) -> None:
        font_data = _find_test_font()
        if font_data is None:
            pytest.skip("no test font found")
        with pyyudit.YuditFont.from_bytes(font_data) as f:
            assert f.upem > 0

    def test_name(self) -> None:
        font_data = _find_test_font()
        if font_data is None:
            pytest.skip("no test font found")
        with pyyudit.YuditFont.from_bytes(font_data) as f:
            assert isinstance(f.name, str)


@pytest.mark.skipif(not _has_native(), reason=_skip_reason)
class TestShaping:
    def test_shape_latin(self) -> None:
        font_data = _find_test_font()
        if font_data is None:
            pytest.skip("no test font found")
        with pyyudit.YuditFont.from_bytes(font_data) as f:
            result = f.shape("Hello")
            assert len(result.glyphs) > 0
            assert result.advance_x > 0

    def test_shape_trace(self) -> None:
        font_data = _find_test_font()
        if font_data is None:
            pytest.skip("no test font found")
        with pyyudit.YuditFont.from_bytes(font_data) as f:
            result = f.shape_trace("Hello")
            assert len(result.stages) > 0
            assert result.stages[0].m == "input"
            assert result.stages[-1].m == "shaped"

    def test_shape_empty(self) -> None:
        font_data = _find_test_font()
        if font_data is None:
            pytest.skip("no test font found")
        with pyyudit.YuditFont.from_bytes(font_data) as f:
            result = f.shape("")
            assert len(result.glyphs) == 0
