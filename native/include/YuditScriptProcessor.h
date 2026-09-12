/**
 * YuditScriptProcessor.h — Subclass of SScriptProcessor that exposes
 * protected methods for per-step trace capture.
 *
 * SScriptProcessor::apply() does everything in one call. By subclassing
 * and using `using` declarations, we can call each step individually
 * and snapshot the glyph buffer between steps.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef YUDIT_SCRIPT_PROCESSOR_H
#define YUDIT_SCRIPT_PROCESSOR_H

#include <vector>
#include <string>
#include "swindow/SScriptProcessor.h"
#include "swindow/SFontLookup.h"
#include "yudit_trace.h"

/**
 * Minimal glyph record (mirrors YuditGlyph from yudit_shaper.h
 * to avoid circular includes).
 */
struct _YuditGlyphMini {
    uint32_t glyph_id;
    uint32_t codepoint;
    int32_t  x;
    int32_t  y;
    int32_t  width;
    int32_t  cluster;
};

/**
 * A single per-lookup trace event collected during shaping.
 */
struct LookupEvent {
    const char* table;       /* "GSUB" or "GPOS" */
    unsigned int lookup_idx;
    char feature[5];         /* 4-char feature tag, null-terminated */
    int matched;             /* 1 if lookup produced a result */
};

/**
 * A trace stage — a named step in the shaping pipeline
 * with an optional list of per-lookup events and a glyph snapshot.
 */
struct TraceStageInfo {
    std::string          msg;          /* e.g. "decompose", "half", "abvm" */
    std::vector<_YuditGlyphMini> glyphs;   /* glyph buffer snapshot */
    std::vector<LookupEvent> lookups;  /* per-lookup events in this stage */
    bool                 effective;
};

class YuditScriptProcessor : public SScriptProcessor {
public:
    /* Expose protected methods */
    using SScriptProcessor::decompose;
    using SScriptProcessor::reorder;
    using SScriptProcessor::gindex;
    using SScriptProcessor::gsub;
    using SScriptProcessor::gsubclean;
    using SScriptProcessor::gposInit;
    using SScriptProcessor::gpos;
    using SScriptProcessor::gposFinal;

    /* Expose protected data */
    using SScriptProcessor::m_in;
    using SScriptProcessor::m_orig;
    using SScriptProcessor::m_out;
    using SScriptProcessor::m_width;
    using SScriptProcessor::m_positions;
    using SScriptProcessor::m_out_type;
    using SScriptProcessor::m_is_begin;
    using SScriptProcessor::m_script;
    using SScriptProcessor::m_otfScript;
    using SScriptProcessor::m_reorder_guide;

    YuditScriptProcessor(SFontLookup* font) : SScriptProcessor(font), m_font_proxy(font) {}

    void applyWithTrace(std::vector<TraceStageInfo>& stages);

private:
    SFontLookup* m_font_proxy;

    /* Trace callback infrastructure */
    static void onTraceEvent(YuditTraceEvent event, const char* table,
                             unsigned int lookup_idx, const char* feature,
                             int matched, void* user_data);

    void snapshotGlyphs(std::vector<_YuditGlyphMini>& out);
};

#endif /* YUDIT_SCRIPT_PROCESSOR_H */
