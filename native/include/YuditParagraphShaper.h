/**
 * YuditParagraphShaper.h — Paragraph-layer shaping front-end.
 *
 * Yudit does its Arabic/Syriac joining (and RTL ordering) in the *paragraph*
 * layer, not in SScriptProcessor:
 *
 *     SParagraph::expand()   → split()   : UCS4 → SGlyph (4 Unicode shapes each)
 *     SParagraph::reShape()  → setShape(): pick isolated/initial/medial/final
 *     SGlyph::getShapedChar(): the shaped codepoint
 *
 * SScriptProcessor only knows about Indic reordering and Thai/Lao/Tibetan/Jamo.
 * This file bridges the paragraph layer to the C API so that joining scripts
 * can be shaped correctly, while reusing SScriptProcessor's GPOS engine
 * (mark/mkmk/kern) for positioning.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef YUDIT_PARAGRAPH_SHAPER_H
#define YUDIT_PARAGRAPH_SHAPER_H

#include <cstdint>
#include <vector>

#include "YuditScriptProcessor.h"   /* TraceStageInfo, _YuditGlyphMini, LookupEvent */

class SFontLookup;

/**
 * One shaped glyph produced by the paragraph layer.
 *
 * x/y keep the same convention as the C API: x is the pen delta from the
 * previous glyph in output order (so `pen += dx` reproduces the exact
 * position), y is the mark-to-base offset.
 */
struct ParagraphGlyph {
    uint32_t glyph_id;        /* glyph index in the font */
    uint32_t codepoint;       /* shaped codepoint used to find the glyph */
    uint32_t orig_codepoint;  /* original (logical) codepoint */
    int32_t  x;               /* pen delta (visual order) */
    int32_t  y;               /* mark offset */
    int32_t  width;           /* advance width (>= 0) */
    int32_t  cluster;         /* index into the original text */
};

/** Result of one paragraph-layer shaping pass. */
struct ParagraphShapeResult {
    std::vector<ParagraphGlyph>  glyphs;   /* visual order */
    std::vector<TraceStageInfo>  stages;
    int                          width;    /* total advance */
    bool                         rtl;      /* base direction was RTL */
};

/**
 * True when the text contains a script whose shaping lives in the paragraph
 * layer (Arabic, Syriac, Hebrew, Thaana, N'Ko, Mandaic, Mongolian, …).
 * Those scripts must not be pushed through SScriptProcessor.
 */
bool yudit_uses_paragraph_path(const uint32_t* text, uint32_t length);

/**
 * Shape `text` with the paragraph layer.
 *
 * The returned glyphs are in visual order (reversed for RTL), which is what
 * HarfBuzz produces for an RTL run, and `x` is the pen delta between
 * consecutive output glyphs.
 *
 * @return false only on an internal failure; on success `out` is filled.
 */
bool yudit_shape_paragraph(SFontLookup* font,
                           const std::vector<uint32_t>& text,
                           ParagraphShapeResult& out);

#endif /* YUDIT_PARAGRAPH_SHAPER_H */
