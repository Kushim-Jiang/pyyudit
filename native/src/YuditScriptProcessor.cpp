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

    LookupEvent ev;
    ev.event      = event;
    ev.table      = table;
    ev.lookup_idx = lookup_idx;
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

/* ── Snapshot helper ────────────────────────────────────────────────────── */

void
YuditScriptProcessor::snapshotGlyphs(std::vector<_YuditGlyphMini>& out)
{
    out.clear();

    /* Before gindex() runs, m_out is empty: the buffer still holds the
     * (decomposed, reordered) codepoints in m_in.  Report those, with the
     * codepoint in the glyph field - that is how the reference trace shows a
     * buffer that has not been mapped to glyphs yet. */
    if (getGlyphs().size() == 0) {
        for (unsigned int i = 0; i < m_in.size(); i++) {
            _YuditGlyphMini g;
            g.glyph_id  = (uint32_t)m_in[i];
            g.codepoint = (uint32_t)m_in[i];
            g.x = g.y = g.width = 0;
            g.cluster   = (int32_t)i;
            out.push_back(g);
        }
        return;
    }

    const SV_GlyphIndex& gi = getGlyphs();
    const SV_INT& pos = getPositions();
    int prev_x = 0;
    for (unsigned int i = 0; i < gi.size(); i++) {
        _YuditGlyphMini g;
        g.glyph_id  = gi[i];
        g.codepoint = 0;
        if (i < (unsigned int)pos.size()) {
            int32_t xy = pos[i];
            int abs_x = (int16_t)(xy & 0xffff);
            int mark_y = (int16_t)((xy >> 16) & 0xffff);
            g.x = abs_x - prev_x;  /* relative dx */
            g.y = mark_y;           /* mark-to-base dy */
            prev_x = abs_x;
        } else {
            g.x = 0;
            g.y = 0;
        }
        g.width   = m_font_proxy ? m_font_proxy->gwidth(gi[i]) : 0;
        g.cluster = (int32_t)i;
        out.push_back(g);
    }
}

/* ── Positioning finalization (fixed) ───────────────────────────────────── */

/*
 * Mirrors SScriptProcessor::gposFinal() with one fix: the base version uses
 * the raw glyph width when deciding whether a positioned glyph extends the
 * line.  Yudit reports a *negative* width for glyphs with a negative left
 * side bearing (Arial's "A", Verdana's "A", Arabic's FE91/FE92, ...), so
 * `x + w` never exceeds the cursor and the line stops growing - every
 * following glyph is then placed at the wrong position.  The magnitude is
 * what matters for the advance.
 */
void
YuditScriptProcessor::gposFinal()
{
    m_positions.clear();
    m_width = 0;
    if (m_out.size() == 0) return;

    /* store these for the first run */
    m_positions.append(0);
    m_xpos.replace(0, 0);
    m_ypos.replace(0, 0);

    int width = m_font->gwidth(m_out[0]);
    if (width < 0) width = -width;

    for (unsigned int i = 1; i < m_out.size(); i++) {
        int x = 0;
        int y = 0;
        if (m_xpos[i] != 0 || m_ypos[i] != 0) {
            /* we have a relative mark-to-base or mark-to-mark position */
            x = m_xpos[i] + m_xpos[m_pos_base_index[i]];
            y = m_ypos[i] + m_ypos[m_pos_base_index[i]];
            /* if it sticks out update our width */
            int w = m_font->gwidth(m_out[i]);
            if (w < 0) w = -w;
            if (x + w > width) width = x + w;
        } else {
            int w = m_font->gwidth(m_out[i]);
            if (w < 0) w = -w;
            if (w > 0) {
                x = width;
                y = 0;
                width += w;
            } else {
                x = width - w;
                y = 0;
            }
        }
        m_xpos.replace(i, x);
        m_ypos.replace(i, y);
        int xy = (y << 16) & 0xffff0000;
        xy = xy | (x & 0xffff);
        m_positions.append(xy);
    }
    m_width = width;
}

/* ── Main trace-capturing shaping pipeline ──────────────────────────────── */

/*
 * Did a GPOS feature change anything?
 *
 * SScriptProcessor::gpos() records *relative* offsets in m_xpos/m_ypos; the
 * packed absolute positions in m_positions are only produced later, by
 * gposFinal().  Comparing m_positions around a gpos() call therefore always
 * reports "unchanged" - the offsets are what moves.
 */
static bool
rel_positions_differ(const SV_INT& prev_x, const SV_INT& prev_y,
                     const SV_INT& cur_x,  const SV_INT& cur_y)
{
    if (prev_x.size() != cur_x.size() || prev_y.size() != cur_y.size()) {
        return true;
    }
    for (unsigned int k = 0; k < cur_x.size(); k++) {
        if (prev_x[k] != cur_x[k]) return true;
    }
    for (unsigned int k = 0; k < cur_y.size(); k++) {
        if (prev_y[k] != cur_y[k]) return true;
    }
    return false;
}

/* Did the glyph buffer change? */
static bool
glyph_run_changed(const SV_GlyphIndex& prev, const SV_GlyphIndex& cur)
{
    if (prev.size() != cur.size()) return true;
    for (unsigned int k = 0; k < cur.size(); k++) {
        if (prev[k] != cur[k]) return true;
    }
    return false;
}

/* Did the codepoint buffer change? */
static bool
char_run_changed(const SV_UCS4& prev, const SV_UCS4& cur)
{
    if (prev.size() != cur.size()) return true;
    for (unsigned int k = 0; k < cur.size(); k++) {
        if (prev[k] != cur[k]) return true;
    }
    return false;
}

void
YuditScriptProcessor::applyWithTrace(std::vector<TraceStageInfo>& stages){
    stages.clear();

    unsigned int i;
    SStringVector gsub_guide;
    SStringVector gpos_guide;

    m_in = m_orig;
    m_out.clear();
    m_out_type.clear();

    switch (m_script) {
    case SC_NONE:
        /* No complex-script reordering: plain cmap-driven scripts such as
         * Latin, Greek, Cyrillic, CJK and symbols.  Yudit defines no feature
         * set of its own for these, so use the same list it uses for every
         * other script it does not handle specially (see `default` below).
         * The OTF script tag is picked by the caller (latn / hani / kana /
         * hang), so features the font does not carry for that script simply
         * do not match. */
        gsub_guide = SStringVector("ccmp,isol,fina,medi,init,rlig,calt");
        gpos_guide = SStringVector("kern,mark,mkmk");
        m_in = m_orig;
        break;
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
    case SC_SINHALA:
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
        /* For unsupported scripts (including Arabic/Hebrew),
         * try common GSUB/GPOS features. Arabic joining forms
         * are handled by isol/fina/medi/init features in the font. */
        gsub_guide = SStringVector("ccmp,isol,fina,medi,init,rlig,calt");
        gpos_guide = SStringVector("kern,mark,mkmk");
        /* For unsupported scripts, m_in may not be set by put().
         * Copy m_orig to m_in so gindex/gsub can work. */
        m_in = m_orig;
        break;
    }

    TraceStageInfo stage;
    TraceCollector collector;

    /* Step 1: decompose */
    SV_UCS4 before_chars = m_in;
    decompose();
    stage.msg = "decompose";
    stage.effective = char_run_changed(before_chars, m_in);
    stage.lookups.clear();
    snapshotGlyphs(stage.glyphs);
    stages.push_back(stage);

    /* Step 2: reorder */
    before_chars = m_in;
    reorder();
    stage.msg = "reorder";
    stage.effective = char_run_changed(before_chars, m_in);
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

        /* No change with this script tag?  Try "dflt" once. */
        if (!glyph_run_changed(prev_out, m_out) && m_script != SC_NONE) {
            const char* saved = m_otfScript;
            m_otfScript = "dflt";
            gsub(s.array());
            m_otfScript = saved;
        }

        /* Unset trace callback */
        yudit_set_trace_callback(0, 0);

        bool changed = glyph_run_changed(prev_out, m_out);

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
    SV_GlyphIndex before_clean = m_out;
    gsubclean();
    stage.msg = "clean";
    stage.effective = glyph_run_changed(before_clean, m_out);
    stage.lookups.clear();
    snapshotGlyphs(stage.glyphs);
    stages.push_back(stage);

    /* Step 6: GPOS features �?with per-lookup trace */
    gposInit();
    for (i = 0; i < gpos_guide.size(); i++) {
        SString s = gpos_guide[i];
        if (s.size() != 4) continue;
        s.append((char)0);

        /* Set trace callback */
        collector.events.clear();
        yudit_set_trace_callback(onTraceEventGlobal, &collector);

        SV_INT prev_x = m_xpos;
        SV_INT prev_y = m_ypos;

        gpos(s.array());

        /* Unset trace callback */
        yudit_set_trace_callback(0, 0);

        bool changed = rel_positions_differ(prev_x, prev_y, m_xpos, m_ypos);

        stage.msg = s.array();
        stage.effective = changed;
        stage.lookups = collector.events;
        snapshotGlyphs(stage.glyphs);
        stages.push_back(stage);
    }

    /* Step 7: gposFinal */
    gposFinal();
    stage.msg = "gpos_final";
    stage.effective = true;
    stage.lookups.clear();
    snapshotGlyphs(stage.glyphs);
    stages.push_back(stage);
}
