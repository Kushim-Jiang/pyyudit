"""Low-level ctypes binding to the yudit_shaper native library."""

from __future__ import annotations

import ctypes
from ctypes import (
    POINTER,
    Structure,
    c_char_p,
    c_int,
    c_int32,
    c_size_t,
    c_uint8,
    c_uint32,
    c_void_p,
)
from typing import Any, Callable

from ._loader import load

_loaded = load()
if _loaded is None:
    raise ImportError("yudit_shaper native library not found. See pyyudit._loader")

_lib: ctypes.CDLL = _loaded


def _fn(name: str, res: Any, *params: Any, **kwds: Any) -> None:
    """Declare the restype/argtypes for a native function."""
    f = getattr(_lib, name)
    f.restype = res
    f.argtypes = params
    errcheck = kwds.get("errcheck")
    if errcheck:
        f.errcheck = errcheck


def _check(err: int, func: Callable[..., Any], _args: tuple[Any, ...]) -> int:
    """Raise on negative error codes."""
    if err < 0:
        raise RuntimeError(f"{func.__name__} failed: error code {err}")
    return err


# ── Structures ─────────────────────────────────────────────────────────────


class _Glyph(Structure):
    _fields_ = [
        ("glyph_id", c_uint32),
        ("codepoint", c_uint32),
        ("x", c_int32),
        ("y", c_int32),
        ("width", c_int32),
        ("cluster", c_int32),
    ]


class _TraceStage(Structure):
    _fields_ = [
        ("message", c_char_p),
        ("glyphs", POINTER(_Glyph)),
        ("glyph_count", c_uint32),
        ("depth", c_int),
        ("effective", c_int),
    ]


# ── Function declarations ─────────────────────────────────────────────────

_fn("yudit_version", c_char_p)
_fn("yudit_error_string", c_char_p, c_int)

# Font
_fn("yudit_font_open", c_int, POINTER(c_uint8), c_size_t, c_int, POINTER(c_void_p), errcheck=_check)
_fn("yudit_font_close", None, c_void_p)
_fn("yudit_font_upem", c_int, c_void_p)
_fn("yudit_font_glyph_count", c_int, c_void_p)
_fn("yudit_font_name", c_char_p, c_void_p)

# Shaping
_fn("yudit_shape", c_int, c_void_p, POINTER(c_uint32), c_uint32, POINTER(c_void_p), errcheck=_check)
_fn("yudit_result_free", None, c_void_p)

# Result queries
_fn("yudit_result_glyph_count", c_uint32, c_void_p)
_fn("yudit_result_glyphs", POINTER(_Glyph), c_void_p)
_fn("yudit_result_width", c_int, c_void_p)
_fn("yudit_result_trace_count", c_uint32, c_void_p)
_fn("yudit_result_trace_stage", POINTER(_TraceStage), c_void_p, c_uint32)
