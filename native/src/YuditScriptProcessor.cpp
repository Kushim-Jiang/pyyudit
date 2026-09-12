/**
 * YuditScriptProcessor.cpp �?Step-by-step shaping with per-lookup trace.
 *
 * Uses the global trace callback from yudit_trace.h to capture per-lookup
 * events inside getOTFFeature() and processGPOSFeature(). Each lookup
 * attempt and result is recorded.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "YuditScriptProcessor.h"
#include "stoolkit/SStringVector.h"
#include "yudit_trace.h"

/* ── Static trace callback ──────────────────────────────────────────────── */

/**
 * Per-lookup events collected during a single gsub()/gpos() call.
 */
struct TraceCollector {
    std::vector<LookupEvent> events;
};

static void
onTraceEventGlobal(
    YuditTraceEvent event, const char* table,
    unsigned int lookup_idx, const char* feature,
    int matched, void* user_data)
{
    TraceCollector* col = (TraceCollector*)user_data;
    if (!col) return;

    if (event == YUDIT_TRACE_LOOKUP_END) {
        LookupEvent ev;
        ev.table       = table;
        ev.lookup_idx  = lookup_idx;
        /* Copy feature tag bytes (may not be null-terminated) */
        if (feature) {
            ev.feature[0] = feature[0];
            ev.feature[1] = feature[1];
            ev.feature[2] = feature[2];
            ev.feature[3] = feature[3];
            ev.feature[4] = '\0';
        } else {
            ev.feature[0] = '\0';
        }
        ev.matched     = matched;
        col->events.push_back(ev);
    }
}

/* ── Snapshot helper ────────────────────────────────────────────────────── */

void
YuditScriptProcessor::snapshotGlyphs(std::vector<_YuditGlyphMini>& out)
{
    out.clear();
    const SV_GlyphIndex& gi = getGlyphs();
    const SV_INT& pos = getPositions();
    for (unsigned int i = 0; i < gi.size(); i++) {
        _YuditGlyphMini g;
        g.glyph_id  = gi[i];
        g.codepoint = 0;
        if (i < (unsigned int)pos.size()) {
            int32_t xy = pos[i];
            g.x = (int16_t)(xy & 0xffff);
            g.y = (int16_t)((xy >> 16) & 0xffff);
        } else {
            g.x = 0;
            g.y = 0;
        }
        g.width   = m_font_proxy ? m_font_proxy->gwidth(gi[i]) : 0;
        g.cluster = (int32_t)i;
        out.push_back(g);
    }
}

/* ── Main trace-capturing shaping pipeline ──────────────────────────────── */

void
YuditScriptProcessor::applyWithTrace(std::vector<TraceStageInfo>& stages)
{
    stages.clear();

    unsigned int i;
    SStringVector gsub_guide;
    SStringVector gpos_guide;

    m_in = m_orig;
    m_out.clear();
    m_out_type.clear();

    switch (m_script) {
    case SC_NONE: break;
    case SC_DEVANAGARI:
        gsub_guide = SStringVector("nukt,akhn,rphf,blwf,half,vatu,pres,abvs,blws,psts,haln");
        gpos_guide = SStringVector("abvm,blwm,dist");
        break;
    case SC_BENGALI:
        gsub_guide = SStringVector("nukt,akhn,rphf,blwf,half,pstf,vatu,pres,abvs,blws,psts");
        gpos_guide = SStringVector("abvm,blwm,dist");
        break;
    case SC_GURMUKHI:
        gsub_guide = SStringVector("nukt,blwf,pstf,vatu,pres,abvs,blws,psts,haln");
        gpos_guide = SStringVector("abvm,blwm,dist");
        break;
    case SC_GUJARATI:
        gsub_guide = SStringVector("nukt,akhn,rphf,blwf,half,pstf,vatu,pres,abvs,blws,psts,haln");
        gpos_guide = SStringVector("abvm,blwm,dist");
        break;
    case SC_ORIYA:
    case SC_KANNADA:
    case SC_MALAYALAM:
        gsub_guide = SStringVector("nukt,akhn,rphf,blwf,half,pstf,vatu,pres,blws,abvs,psts,haln");
        gpos_guide = SStringVector("abvm,blwm,dist");
        break;
    case SC_TAMIL:
        gsub_guide = SStringVector("akhn,half,pres,abvs,blws,psts,haln");
        gpos_guide = SStringVector("abvm,blwm,dist");
        break;
    case SC_TELUGU:
        gsub_guide = SStringVector("akhn,blwf,abvs,blws,psts,haln");
        gpos_guide = SStringVector("abvm,blwm,dist");
        break;
    case SC_THAI:
        gsub_guide = SStringVector("ccmp");
        gpos_guide = SStringVector("kern,mark,mkmk");
        break;
    case SC_LAO:
        gsub_guide = SStringVector("ccmp,blws,abvs");
        gpos_guide = SStringVector("kern,mark,mkmk");
        break;
    case SC_TIBETAN:
        gsub_guide = SStringVector("ccmp,blws,abvs");
        gpos_guide = SStringVector("blwm,abvm,kern");
        break;
    case SC_JAMO:
        gsub_guide = SStringVector("ccmp,ljmo,vjmo,tjmo");
        gpos_guide = SStringVector("");
        break;
    default:
    case SC_MAX:
        break;
    }

    TraceStageInfo stage;
    TraceCollector collector;

    /* Step 1: decompose */
    decompose();
    stage.msg = "decompose";
    stage.effective = true;
    stage.lookups.clear();
    snapshotGlyphs(stage.glyphs);
    stages.push_back(stage);

    /* Step 2: reorder */
    reorder();
    stage.msg = "reorder";
    stage.effective = true;
    stage.lookups.clear();
    snapshotGlyphs(stage.glyphs);
    stages.push_back(stage);

    /* Step 3: gindex */
    gindex();
    stage.msg = "gindex";
    stage.effective = true;
    stage.lookups.clear();
    snapshotGlyphs(stage.glyphs);
    stages.push_back(stage);

    /* Step 4: GSUB features �?with per-lookup trace */
    SV_GlyphIndex prev_out = m_out;
    for (i = 0; i < gsub_guide.size(); i++) {
        SString s = gsub_guide[i];
        if (s.size() != 4) continue;
        s.append((char)0);

        /* Set trace callback */
        collector.events.clear();
        yudit_set_trace_callback(onTraceEventGlobal, &collector);

        gsub(s.array());

        /* Unset trace callback */
        yudit_set_trace_callback(0, 0);

        /* Check if buffer changed */
        bool changed = (m_out.size() != prev_out.size());
        if (!changed) {
            for (unsigned int j = 0; j < m_out.size(); j++) {
                if (j < prev_out.size() && m_out[j] != prev_out[j]) {
                    changed = true;
                    break;
                }
            }
        }

        /* Create stage with per-lookup events */
        stage.msg = s.array(); /* e.g. "half" */
        stage.effective = changed;
        stage.lookups = collector.events;
        snapshotGlyphs(stage.glyphs);
        stages.push_back(stage);

        if (changed) {
            prev_out = m_out;
        }
    }

    /* Step 5: gsubclean */
    gsubclean();
    stage.msg = "clean";
    stage.effective = true;
    stage.lookups.clear();
    snapshotGlyphs(stage.glyphs);
    stages.push_back(stage);

    /* Step 6: GPOS features �?with per-lookup trace */
    gposInit();
    SV_INT prev_pos = m_positions;
    for (i = 0; i < gpos_guide.size(); i++) {
        SString s = gpos_guide[i];
        if (s.size() != 4) continue;
        s.append((char)0);

        /* Set trace callback */
        collector.events.clear();
        yudit_set_trace_callback(onTraceEventGlobal, &collector);

        gpos(s.array());

        /* Unset trace callback */
        yudit_set_trace_callback(0, 0);

        bool changed = (m_positions.size() != prev_pos.size());
        if (!changed) {
            for (unsigned int j = 0; j < m_positions.size(); j++) {
                if (j < prev_pos.size() && m_positions[j] != prev_pos[j]) {
                    changed = true;
                    break;
                }
            }
        }

        stage.msg = s.array();
        stage.effective = changed;
        stage.lookups = collector.events;
        snapshotGlyphs(stage.glyphs);
        stages.push_back(stage);

        if (changed) {
            prev_pos = m_positions;
        }
    }

    /* Step 7: gposFinal */
    gposFinal();
    stage.msg = "gpos_final";
    stage.effective = true;
    stage.lookups.clear();
    snapshotGlyphs(stage.glyphs);
    stages.push_back(stage);
}
