/**
 * yudit_shaper.h — C API for Yudit's Unicode shaping engine.
 *
 * Wraps the script-processing pipeline (Indic reordering, Arabic joining,
 * Thai/Lao/Tibetan/Jamo, OpenType GSUB/GPOS) from the Yudit editor into a
 * headless, cross-platform shaping library with per-step trace output.
 *
 * Copyright (C) 1997-2023 Gaspar Sinai (Yudit original)
 * C wrapper Copyright (C) 2026 Kushim-Jiang
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef YUDIT_SHAPER_H
#define YUDIT_SHAPER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── export/import macros ───────────────────────────────────────────────── */

#if defined(_WIN32) || defined(_WIN64)
  #ifdef YUDIT_BUILDING_DLL
    #define YUDIT_API __declspec(dllexport)
  #else
    #define YUDIT_API __declspec(dllimport)
  #endif
#else
  #define YUDIT_API __attribute__((visibility("default")))
#endif

/* ── opaque handles ─────────────────────────────────────────────────────── */

/** A font loaded from raw TrueType/OpenType bytes. */
typedef struct YuditFont YuditFont;

/** Result of a shaping operation (glyphs + positions + trace). */
typedef struct YuditResult YuditResult;

/* ── error codes ────────────────────────────────────────────────────────── */

typedef enum {
    YUDIT_OK              =  0,
    YUDIT_ERR_NOMEM       = -1,
    YUDIT_ERR_BAD_FONT    = -2,
    YUDIT_ERR_EMPTY_TEXT  = -3,
    YUDIT_ERR_UNSUPPORTED = -4,
} YuditError;

/* ── glyph record ───────────────────────────────────────────────────────── */

/**
 * A single shaped glyph with positioning.
 *
 * x, y are packed:  y in the upper 16 bits, x in the lower 16 bits
 * (matching Yudit's internal m_positions encoding).
 * Use yudit_glyph_x() / yudit_glyph_y() to unpack.
 */
typedef struct {
    uint32_t glyph_id;     /* glyph index in the font  */
    uint32_t codepoint;    /* original Unicode codepoint */
    int32_t  x;            /* horizontal offset (font units) */
    int32_t  y;            /* vertical offset   (font units) */
    int32_t  width;        /* advance width      (font units) */
    int32_t  cluster;      /* cluster index (0-based run offset) */
} YuditGlyph;

/* ── trace stage ────────────────────────────────────────────────────────── */

/**
 * A single shaping step (decompose, reorder, gsub feature, gpos feature, etc.).
 *
 * Each stage captures the full glyph buffer at that point, so the caller can
 * show "before / after" snapshots for any step.
 */
typedef struct {
    const char*    message;   /* e.g. "decompose", "reorder", "half", "abvm" */
    const YuditGlyph* glyphs; /* glyph buffer snapshot at this stage */
    uint32_t       glyph_count;
    int            depth;     /* nesting depth (0 = top-level) */
    int            effective; /* 1 if the buffer changed from the previous stage */
} YuditTraceStage;

/* ── font API ───────────────────────────────────────────────────────────── */

/**
 * Create a font handle from raw TrueType/OpenType font data.
 *
 * @param data   pointer to the font file bytes (copied internally).
 * @param size   number of bytes.
 * @param face_index  face index within the font file (usually 0).
 * @param out    receives the font handle on success.
 * @return YUDIT_OK or a negative error code.
 */
YUDIT_API YuditError yudit_font_open(const uint8_t* data, size_t size,
                           int face_index, YuditFont** out);

/** Release a font handle. */
YUDIT_API void yudit_font_close(YuditFont* font);

/** Return the font's units-per-em. */
YUDIT_API int yudit_font_upem(const YuditFont* font);

/** Return the number of glyphs in the font. */
YUDIT_API int yudit_font_glyph_count(const YuditFont* font);

/** Return the font name (valid until the font is closed). */
YUDIT_API const char* yudit_font_name(const YuditFont* font);

/* ── shaping API ────────────────────────────────────────────────────────── */

/**
 * Shape a Unicode text string with the given font.
 *
 * @param font       a font handle.
 * @param text       UTF-32LE encoded text (codepoints, NOT bytes).
 * @param length     number of codepoints.
 * @param out        receives the result on success.
 * @return YUDIT_OK or a negative error code.
 */
YUDIT_API YuditError yudit_shape(const YuditFont* font,
                       const uint32_t* text, uint32_t length,
                       YuditResult** out);

/** Free a shaping result. */
YUDIT_API void yudit_result_free(YuditResult* result);

/* ── result queries ─────────────────────────────────────────────────────── */

/** Number of output glyphs. */
YUDIT_API uint32_t yudit_result_glyph_count(const YuditResult* result);

/** Pointer to the output glyph array (valid until yudit_result_free). */
YUDIT_API const YuditGlyph* yudit_result_glyphs(const YuditResult* result);

/** Total advance width of the shaped run (font units). */
YUDIT_API int yudit_result_width(const YuditResult* result);

/** Number of trace stages recorded. */
YUDIT_API uint32_t yudit_result_trace_count(const YuditResult* result);

/** Get a specific trace stage. Returns NULL if index is out of range. */
YUDIT_API const YuditTraceStage* yudit_result_trace_stage(const YuditResult* result,
                                                 uint32_t index);

/* ── convenience ────────────────────────────────────────────────────────── */

/** Return a human-readable string for an error code. */
YUDIT_API const char* yudit_error_string(YuditError err);

/** Return the library version string. */
YUDIT_API const char* yudit_version(void);

#ifdef __cplusplus
}
#endif

#endif /* YUDIT_SHAPER_H */
