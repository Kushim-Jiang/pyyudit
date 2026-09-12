/**
 * YuditParagraphShaper.cpp — See YuditParagraphShaper.h for the rationale.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "YuditParagraphShaper.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "stoolkit/SParagraph.h"
#include "stoolkit/SGlyph.h"
#include "stoolkit/SCharClass.h"
#include "stoolkit/SRendClass.h"
#include "stoolkit/SUniMap.h"
#include "stoolkit/STypes.h"
#include "swindow/SFontLookup.h"
#include "swindow/SScriptProcessor.h"
#include "yudit_trace.h"

/* ── small helpers ──────────────────────────────────────────────────────── */

static inline bool
in_range(uint32_t c, uint32_t lo, uint32_t hi)
{
    return c >= lo && c <= hi;
}

static inline bool
is_arabic(uint32_t c)
{
    return in_range(c, 0x0600, 0x06FF) || in_range(c, 0x0750, 0x077F)
        || in_range(c, 0x08A0, 0x08FF) || in_range(c, 0xFB50, 0xFDFF)
        || in_range(c, 0xFE70, 0xFEFF);
}

static inline bool
is_syriac(uint32_t c)
{
    return in_range(c, 0x0700, 0x074F) || in_range(c, 0x0860, 0x086F);
}

static inline bool
is_hebrew(uint32_t c)
{
    return in_range(c, 0x0590, 0x05FF) || in_range(c, 0xFB1D, 0xFB4F);
}

/* ── routing ────────────────────────────────────────────────────────────── */

bool
yudit_uses_paragraph_path(const uint32_t* text, uint32_t length)
{
    if (!text) return false;
    for (uint32_t i = 0; i < length; i++) {
        uint32_t c = text[i];
        if (is_hebrew(c) || is_arabic(c) || is_syriac(c)) return true;
        if (in_range(c, 0x0780, 0x07BF)) return true;  /* Thaana    */
        if (in_range(c, 0x07C0, 0x07FF)) return true;  /* N'Ko      */
        if (in_range(c, 0x0840, 0x085F)) return true;  /* Mandaic   */
        if (in_range(c, 0x1800, 0x18AF)) return true;  /* Mongolian */
    }
    return false;
}

/* 
 * UAX#9 P2/P3: the first strong character determines the base direction.
 * This mirrors what a caller of HarfBuzz would pass as the run direction.
 */
static bool
detect_rtl(const std::vector<uint32_t>& text)
{
    for (size_t i = 0; i < text.size(); i++) {
        SD_BiDiClass c = getBiDiClass((SS_UCS4)text[i]);
        if (c == SD_BC_L) return false;
        if (c == SD_BC_R || c == SD_BC_AL) return true;
    }
    return false;
}

/* OpenType script tag for the run (first matching script wins). */
static const char*
otf_script_for(const std::vector<uint32_t>& text)
{
    for (size_t i = 0; i < text.size(); i++) {
        uint32_t c = text[i];
        if (is_syriac(c))  return "syrc";
        if (in_range(c, 0x0780, 0x07BF)) return "thaa";
        if (in_range(c, 0x07C0, 0x07FF)) return "nko ";
        if (in_range(c, 0x0840, 0x085F)) return "mand";
        if (in_range(c, 0x1800, 0x18AF)) return "mong";
        if (is_hebrew(c))  return "hebr";
        if (is_arabic(c))  return "arab";
    }
    return "DFLT";
}

/* ── trace plumbing ─────────────────────────────────────────────────────── */

static void
push_stage(std::vector<TraceStageInfo>& stages, const char* msg,
           const std::vector<ParagraphGlyph>& glyphs, bool effective,
           const std::vector<LookupEvent>* lookups = 0)
{
    TraceStageInfo st;
    st.msg       = msg;
    st.effective = effective;
    if (lookups) st.lookups = *lookups;
    st.glyphs.resize(glyphs.size());
    for (size_t i = 0; i < glyphs.size(); i++) {
        st.glyphs[i].glyph_id  = glyphs[i].glyph_id;
        st.glyphs[i].codepoint = glyphs[i].codepoint;
        st.glyphs[i].x         = glyphs[i].x;
        st.glyphs[i].y         = glyphs[i].y;
        st.glyphs[i].width     = glyphs[i].width;
        st.glyphs[i].cluster   = glyphs[i].cluster;
    }
    stages.push_back(st);
}

/* Collects the per-lookup events of one GSUB or GPOS feature call. */
struct StageTraceCollector {
    std::vector<LookupEvent> events;
};

static void
onStageTraceEvent(YuditTraceEvent event, const char* table,
                  unsigned int lookup_idx, const char* feature,
                  int matched, void* user_data)
{
    StageTraceCollector* col = (StageTraceCollector*)user_data;
    if (!col) return;

    LookupEvent ev;
    ev.event      = event;
    ev.table      = table;
    ev.lookup_idx = lookup_idx;
    if (feature) {
        ev.feature[0] = feature[0];
        ev.feature[1] = feature[1];
        ev.feature[2] = feature[2];
        ev.feature[3] = feature[3];
        ev.feature[4] = '\0';
    } else {
        ev.feature[0] = '\0';
    }
    ev.matched = matched;
    col->events.push_back(ev);
}

/* ── main entry point ───────────────────────────────────────────────────── */

/*
 * Apply one OpenType substitution feature to a glyph list.
 *
 * SFontLookup::gsub() reports, for the run starting at the glyph it is
 * called with, how many glyphs are consumed (`cut`) and how many glyphs it
 * produces (`out`/`out_size`).  A ligature therefore merges a run of glyphs
 * into a single one; the merged glyph keeps the cluster (and codepoint) of
 * the first source glyph, which is what HarfBuzz does as well.
 */
static void
apply_gsub_feature(SFontLookup* font, const char* script, const char* feature,
                   std::vector<ParagraphGlyph>& glyphs)
{
    if (glyphs.size() < 2) return;

    std::vector<SS_GlyphIndex> in(glyphs.size());
    for (size_t i = 0; i < glyphs.size(); i++) {
        in[i] = (SS_GlyphIndex)glyphs[i].glyph_id;
    }
    std::vector<SS_GlyphIndex> out(glyphs.size() + 8);

    for (size_t i = 0; i + 1 < glyphs.size() && i + 1 < in.size(); ) {
        unsigned int start = 0;
        unsigned int out_size = 0;
        bool is_contextual = false;

        unsigned int cut = font->gsub(script, feature,
                                      &in[i], (unsigned int)(in.size() - i),
                                      &start, out.data(), &out_size,
                                      &is_contextual);

        if (cut == 0 || out_size == 0
            || (size_t)start + (size_t)cut > in.size() - i) {
            i++;
            continue;
        }

        size_t from = i + start;
        if (from + cut > glyphs.size()) { i++; continue; }

        ParagraphGlyph proto = glyphs[from];

        in.erase(in.begin() + from, in.begin() + from + cut);
        in.insert(in.begin() + from, out.begin(), out.begin() + out_size);

        glyphs.erase(glyphs.begin() + from, glyphs.begin() + from + cut);
        for (unsigned int k = 0; k < out_size; k++) {
            ParagraphGlyph g = proto;
            g.glyph_id = out[k];
            int w = font->gwidth(out[k]);
            g.width = (w < 0) ? -w : w;
            glyphs.insert(glyphs.begin() + from + k, g);
        }
        i++;
    }
}

bool
yudit_shape_paragraph(SFontLookup* font,
                      const std::vector<uint32_t>& text,
                      ParagraphShapeResult& out)
{
    out.glyphs.clear();
    out.stages.clear();
    out.width = 0;
    out.rtl   = false;

    if (!font || text.empty()) return true;

    const bool  rtl    = detect_rtl(text);
    const char* script = otf_script_for(text);
    out.rtl = rtl;

    /* ── stage: input ───────────────────────────────────────────────────── */
    {
        std::vector<ParagraphGlyph> in(text.size());
        for (size_t i = 0; i < text.size(); i++) {
            ParagraphGlyph g;
            g.glyph_id       = text[i];   /* codepoint, see push_codepoints */
            g.codepoint      = text[i];
            g.orig_codepoint = text[i];
            g.x = g.y = g.width = 0;
            g.cluster        = (int32_t)i;
            in[i] = g;
        }
        push_stage(out.stages, "input", in, true);
    }

    /* ── paragraph layer: split, compose, join ──────────────────────────── */
    SV_UCS4 buffer;
    for (size_t i = 0; i < text.size(); i++) {
        buffer.append((SS_UCS4)text[i]);
    }

    std::vector<ParagraphGlyph> logical;   /* logical order, shaped chars */
    std::vector<uint32_t>       split_cp;  /* original chars per glyph     */

    unsigned int cursor = 0;
    while (cursor < buffer.size()) {
        unsigned int para_start = cursor;
        SParagraph para(buffer, &cursor);

        /* size()/operator[] lazily run the private expand(), which performs
         * split() (composition + Unicode shape arrays) and then reShape()
         * (the Arabic/Syriac joining pass). */
        unsigned int consumed = 0;
        unsigned int n = para.size();
        for (unsigned int i = 0; i < n; i++) {
            const SGlyph& g = para[i];

            SS_UCS4 shaped = g.getShapedChar();
            SS_UCS4 orig   = g.getChar();
            if (orig == 0) orig = g.getFirstChar();
            if (orig == 0) orig = shaped;

            /* number of source characters this glyph consumed */
            unsigned int step = g.decompSize() + g.compSize();
            if (step == 0) step = 1;

            const int32_t cluster = (int32_t)(para_start + consumed);

            ParagraphGlyph pg;
            pg.glyph_id       = 0;
            pg.codepoint      = shaped;
            pg.orig_codepoint = orig;
            pg.x = pg.y = pg.width = 0;
            pg.cluster        = cluster;

            logical.push_back(pg);
            split_cp.push_back(orig);

            /* Combining marks ride along inside the base glyph.  Emit them as
             * their own glyphs so GPOS mark attachment can position them. */
            unsigned int csz = g.compSize();
            if (csz > 0) {
                const SS_UCS4* comp = g.getCompArray();
                for (unsigned int m = 0; m < csz; m++) {
                    if (comp[m] == 0) continue;
                    ParagraphGlyph mg;
                    mg.glyph_id       = 0;
                    mg.codepoint      = comp[m];
                    mg.orig_codepoint = comp[m];
                    mg.x = mg.y = mg.width = 0;
                    mg.cluster        = cluster;
                    logical.push_back(mg);
                    split_cp.push_back(comp[m]);
                }
            }
            consumed += step;
        }
    }

    if (logical.empty()) return true;

    /* ── stage: split (original characters clustered by the paragraph) ────
     * Until the glyph lookup runs the buffer holds codepoints, not glyph ids,
     * so the codepoint is what goes in the glyph field. */
    {
        std::vector<ParagraphGlyph> st(logical.size());
        for (size_t i = 0; i < logical.size(); i++) {
            ParagraphGlyph g = logical[i];
            g.codepoint = split_cp[i];
            g.glyph_id  = split_cp[i];
            st[i] = g;
        }
        push_stage(out.stages, "split", st, true);
    }

    /* ── stage: join (isolated/initial/medial/final selected) ───────────── */
    {
        std::vector<ParagraphGlyph> jn = logical;
        bool joined = false;
        for (size_t i = 0; i < jn.size(); i++) {
            jn[i].glyph_id = jn[i].codepoint;
            if (jn[i].codepoint != split_cp[i]) joined = true;
        }
        push_stage(out.stages, "join", jn, joined);
    }

    /* ── glyph lookup ───────────────────────────────────────────────────── */
    for (size_t i = 0; i < logical.size(); i++) {
        ParagraphGlyph& pg = logical[i];
        SS_GlyphIndex gid = font->gindex(pg.codepoint);
        if (gid == 0 && pg.orig_codepoint != pg.codepoint) {
            gid = font->gindex(pg.orig_codepoint);
        }
        if (gid == 0) {
            /* dotted circle — keeps one-to-one correspondence in the trace */
            gid = font->gindex(0x25cc);
        }
        pg.glyph_id = gid;
        int w = font->gwidth(gid);
        /* Yudit returns a negative width for glyphs with a negative left side
         * bearing (its "right aligned" marker).  For advance purposes the
         * magnitude is what matters. */
        pg.width = (w < 0) ? -w : w;
    }

    push_stage(out.stages, "gindex", logical, true);

    /* ── GSUB: ligatures and contextual forms over the joined glyphs ──────
     * The paragraph layer resolved the joining forms (isolated/initial/
     * medial/final) from Yudit's Unicode shape tables.  The remaining
     * OpenType substitutions - required ligatures such as lam-alef and the
     * Allah ligature, plus contextual alternates - are applied here, in the
     * order HarfBuzz uses for Arabic: joining forms first, then ligatures.
     */
    static const char* gsub_feats[] = { "rlig", "calt", "liga" };
    for (size_t f = 0; f < sizeof(gsub_feats) / sizeof(gsub_feats[0]); f++) {
        StageTraceCollector col;
        yudit_set_trace_callback(onStageTraceEvent, &col);
        apply_gsub_feature(font, script, gsub_feats[f], logical);
        yudit_set_trace_callback(0, 0);

        bool effective = false;
        for (size_t k = 0; k < col.events.size(); k++) {
            if (col.events[k].matched) { effective = true; break; }
        }
        push_stage(out.stages, gsub_feats[f], logical, effective, &col.events);
    }

    /* ── GPOS: reuse SScriptProcessor's positioning engine ──────────────── */
    YuditScriptProcessor sp(font);
    sp.m_script    = SScriptProcessor::SC_NONE;
    sp.m_otfScript = script;
    sp.m_in.clear();
    sp.m_out.clear();
    sp.m_out_type.clear();
    for (size_t i = 0; i < logical.size(); i++) {
        sp.m_out.append((SS_GlyphIndex)logical[i].glyph_id);
        sp.m_out_type.append(SRendClass::None);
    }
    sp.m_positions.clear();

    sp.gposInit();
    static const char* feats[] = { "kern", "mark", "mkmk" };
    for (size_t f = 0; f < sizeof(feats) / sizeof(feats[0]); f++) {
        StageTraceCollector col;
        yudit_set_trace_callback(onStageTraceEvent, &col);

        /* gpos() writes the relative offsets into m_xpos/m_ypos - the packed
         * positions in m_positions only appear when gposFinal() runs. */
        std::vector<int> prev_x(sp.m_xpos.size());
        std::vector<int> prev_y(sp.m_ypos.size());
        for (size_t k = 0; k < prev_x.size(); k++) prev_x[k] = sp.m_xpos[k];
        for (size_t k = 0; k < prev_y.size(); k++) prev_y[k] = sp.m_ypos[k];

        sp.gpos(feats[f]);
        yudit_set_trace_callback(0, 0);

        bool effective = false;
        if (prev_x.size() != (size_t)sp.m_xpos.size()
            || prev_y.size() != (size_t)sp.m_ypos.size()) {
            effective = true;
        } else {
            for (size_t k = 0; k < prev_x.size() && !effective; k++) {
                if (prev_x[k] != sp.m_xpos[k]) effective = true;
            }
            for (size_t k = 0; k < prev_y.size() && !effective; k++) {
                if (prev_y[k] != sp.m_ypos[k]) effective = true;
            }
        }
        push_stage(out.stages, feats[f], logical, effective, &col.events);
    }

    /* Snapshot the *relative* mark attachments before gposFinal() rewrites
     * m_xpos/m_ypos in place with absolute coordinates. */
    const size_t n = logical.size();
    std::vector<int>  rel_x(n, 0);
    std::vector<int>  rel_y(n, 0);
    std::vector<int>  attached_base(n, 0);
    std::vector<char> attached(n, 0);
    for (size_t i = 0; i < n; i++) {
        if (i < (size_t)sp.m_xpos.size())           rel_x[i] = sp.m_xpos[i];
        if (i < (size_t)sp.m_ypos.size())           rel_y[i] = sp.m_ypos[i];
        if (i < (size_t)sp.m_pos_base_index.size()) attached_base[i] = (int)sp.m_pos_base_index[i];
        attached[i] = (rel_x[i] != 0 || rel_y[i] != 0) ? 1 : 0;
    }

    sp.gposFinal();

    /* ── decode absolute positions ──────────────────────────────────────── */
    std::vector<int> adv(n, 0);
    std::vector<int> pen(n, 0);     /* advance cursor in logical order */
    std::vector<int> absy(n, 0);
    std::vector<int> slot(n, 0);    /* glyph that owns the advance slot */
    std::vector<int> dx0(n, 0);     /* offset from that slot's origin */

    int running = 0;
    for (size_t i = 0; i < n; i++) {
        adv[i] = logical[i].width;
        pen[i] = running;
        running += adv[i];

        int b = attached_base[i];
        if (b < 0 || b >= (int)i) b = (i > 0) ? (int)i - 1 : 0;
        slot[i] = attached[i] ? slot[(size_t)b] : (int)i;
        dx0[i]  = attached[i] ? rel_x[i] : 0;

        if (i < (size_t)sp.m_positions.size()) {
            absy[i] = (int16_t)((sp.m_positions[i] >> 16) & 0xffff);
        }
    }
    const int total = running;

    /* ── visual order (reverse the run for RTL, like HarfBuzz) ────────────
     *
     * A normal glyph sits at its own advance origin; a mark sits at the
     * origin of the slot it attaches to, plus its GPOS offset.  For an RTL
     * run the glyphs logically *after* a slot are the ones to its left, so
     * the visual origin is measured from the end of the run.
     */
    std::vector<ParagraphGlyph> visual(n);
    int prev_x = 0;
    for (size_t v = 0; v < n; v++) {
        size_t i = rtl ? (n - 1 - v) : v;
        size_t s = (size_t)slot[i];
        int origin = rtl ? (total - pen[s] - adv[s]) : pen[s];
        int x = origin + dx0[i];
        int y = absy[i];

        ParagraphGlyph g = logical[i];
        g.x = x - prev_x;
        g.y = y;
        prev_x = x;
        visual[v] = g;
    }

    if (rtl) {
        push_stage(out.stages, "reorder", visual, true);
    }

    out.glyphs = visual;
    out.width  = total;

    push_stage(out.stages, "shaped", visual, true);
    return true;
}
