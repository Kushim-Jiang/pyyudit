"""Exception hierarchy for pyyudit."""

from __future__ import annotations


class YuditError(Exception):
    """Base exception for all pyyudit errors."""


class LibraryNotFound(YuditError):
    """The native yudit_shaper library could not be found or loaded."""


class FontError(YuditError):
    """The font data is invalid or unsupported."""


class ShapingError(YuditError):
    """Shaping failed (unsupported script, empty text, etc.)."""
