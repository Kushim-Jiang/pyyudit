/**
 * yudit_shaper.cpp — C API wrapper around Yudit's shaping engine.
 *
 * Bridges Yudit's internal C++ classes (SFontTTF, SScriptProcessor, etc.)
 * to a clean C interface suitable for ctypes consumption from Python.
 *
 * Strategy: write font bytes to a temp file so SFile/SFileImage can mmap them,
 * then drive SScriptProcessor for shaping with trace capture at each step.
 *
 * Copyright (C) 1997-2023 Gaspar Sinai (Yudit original)
 * C wrapper Copyright (C) 2026 Kushim-Jiang
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "yudit_shaper.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <map>

/* ── Yudit internal headers ─────────────────────────────────────────────── */
#include "stoolkit/SString.h"
#include "stoolkit/SUniMap.h"
#include "stoolkit/SGlyph.h"
#include "stoolkit/SParagraph.h"
#include "stoolkit/SRendClass.h"
#include "stoolkit/SCluster.h"
#include "stoolkit/STypes.h"
#include "stoolkit/SBinVector.h"
#include "stoolkit/SStringVector.h"
#include "stoolkit/SIO.h"
#include "stoolkit/SExcept.h"
#include "stoolkit/SBiDi.h"
#include "swindow/SFontTTF.h"
#include "swindow/SFontLookup.h"
#include "YuditScriptProcessor.h"
#include "YuditParagraphShaper.h"

#ifdef USE_WINAPI
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#define SGETTEMP _mktemp
#else
#include <unistd.h>
#include <dlfcn.h>
#define SGETTEMP mktemp
#endif

/* ── Version ────────────────────────────────────────────────────────────── */

#define PYYUDIT_VERSION "0.2.0"

extern "C" const char*
yudit_version(void) { return PYYUDIT_VERSION; }

extern "C" const char*
yudit_error_string(YuditError err) {
    switch (err) {
        case YUDIT_OK:              return "OK";
        case YUDIT_ERR_NOMEM:       return "out of memory";
        case YUDIT_ERR_BAD_FONT:    return "invalid or unsupported font data";
        case YUDIT_ERR_EMPTY_TEXT:  return "empty text";
        case YUDIT_ERR_UNSUPPORTED: return "unsupported script";
        default:                    return "unknown error";
    }
}

/* ── Static initialization: set up Yudit data paths ─────────────────────── */

static bool g_initialized = false;

static void ensure_initialized() {
    if (g_initialized) return;
    g_initialized = true;

    /* Tell SUniMap where to find .my data files.
     * We bundle the maps next to the shared library. */
    std::string lib_dir;
#ifdef USE_WINAPI
    /* On Windows, get the directory of this DLL */
    char dll_path[MAX_PATH];
    HMODULE hMod;
    if (GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR)&ensure_initialized, &hMod)) {
        GetModuleFileNameA(hMod, dll_path, MAX_PATH);
        /* Strip filename to get directory */
        char* last_slash = strrchr(dll_path, '\\');
        if (last_slash) *last_slash = '\0';
        lib_dir = dll_path;
    }
#else
    /* On Linux/macOS, use the library's directory */
    Dl_info info;
    if (dladdr((void*)&ensure_initialized, &info) && info.dli_fname) {
        std::string lib_path = info.dli_fname;
        size_t last_slash = lib_path.find_last_of('/');
        if (last_slash != std::string::npos) {
            lib_dir = lib_path.substr(0, last_slash);
        }
    }
#endif

    /* Set the search path for SUniMap.
     *  - the wheel ships the maps in <lib>/data
     *  - an in-tree CMake build copies them to <lib>/../data
     * PYYUDIT_DATA_PATH overrides both if it is set. */
    SStringVector path;
    const char* env_data = getenv("PYYUDIT_DATA_PATH");
    if (env_data && env_data[0]) path.append(env_data);
    if (!lib_dir.empty()) {
        std::string d1 = lib_dir + "/data";
        std::string d2 = lib_dir + "/../data";
        path.append(d1.c_str());
        path.append(d2.c_str());
    }
    /* Also try the current directory */
    path.append(".");
    path.append("data");
    SUniMap::setPath(path);
}

/* ── Internal structures ────────────────────────────────────────────────── */

struct YuditFont {
    std::string    temp_path;   /* temp file with font data */
    SFontTTF*      font;        /* Yudit font object */
    int            upem_val;
    int            glyph_count_val;
    std::string    name_val;
    bool           ok;

    YuditFont() : font(0), upem_val(0), glyph_count_val(0), ok(false) {}
    ~YuditFont() {
        delete font;
        font = 0;
        /* remove temp file */
        if (!temp_path.empty()) {
            remove(temp_path.c_str());
        }
    }
};

struct TraceStep {
    std::string          message;
    std::vector<YuditGlyph> glyphs;
    int                  depth;
    int                  effective;
};

/* A trace step collected while shaping; same shape as the C API struct. */
using ClusterTrace = TraceStep;

struct YuditResult {
    std::vector<YuditGlyph> glyphs;
    int                      width;
    std::vector<TraceStep>   trace;
};

/* ── Font API ───────────────────────────────────────────────────────────── */

extern "C" YuditError
yudit_font_open(const uint8_t* data, size_t size,
                int face_index, YuditFont** out)
{
    ensure_initialized();

    if (!data || size < 12 || !out) return YUDIT_ERR_BAD_FONT;

    *out = 0;

    YuditFont* f = new (std::nothrow) YuditFont();
    if (!f) return YUDIT_ERR_NOMEM;

    /* Write font bytes to a temp file — SFile needs a path. */
    char tmpl[] = "pyyudit_XXXXXX";
#ifdef USE_WINAPI
    char tmppath[MAX_PATH];
    GetTempPathA(MAX_PATH, tmppath);
    strcat(tmppath, tmpl);
    _mktemp_s(tmppath, strlen(tmppath) + 1);
    int fd = -1;
    errno_t err = _sopen_s(&fd, tmppath, _O_CREAT | _O_BINARY | _O_RDWR, _SH_DENYNO, _S_IREAD | _S_IWRITE);
    if (err != 0 || fd < 0) {
        delete f;
        return YUDIT_ERR_NOMEM;
    }
    _write(fd, data, (unsigned int)size);
    _close(fd);
    f->temp_path = tmppath;
#else
    char tmppath[] = "/tmp/pyyudit_XXXXXX";
    int fd = mkstemp(tmppath);
    if (fd < 0) {
        delete f;
        return YUDIT_ERR_NOMEM;
    }
    write(fd, data, size);
    close(fd);
    f->temp_path = tmppath;
#endif

    /* Create SFile and SFontTTF */
    SString path(f->temp_path.c_str(), (int)f->temp_path.size());
    SString encoding;  /* auto-detect */
    SFile file(path);

    f->font = new SFontTTF(file, encoding);
    if (!f->font || !f->font->isOK()) {
        delete f;
        return YUDIT_ERR_BAD_FONT;
    }

    /* Extract upem from head table */
    f->upem_val = 1000; /* default */
    /* The font is now initialized; we can query basic metrics. */

    f->ok = true;
    *out = f;
    return YUDIT_OK;
}

extern "C" void
yudit_font_close(YuditFont* font) {
    delete font;
}

extern "C" int
yudit_font_upem(const YuditFont* font) {
    if (!font || !font->ok) return 1000;
    return font->upem_val;
}

extern "C" int
yudit_font_glyph_count(const YuditFont* font) {
    if (!font || !font->ok) return 0;
    return font->glyph_count_val;
}

extern "C" const char*
yudit_font_name(const YuditFont* font) {
    if (!font || !font->ok) return "";
    return font->name_val.c_str();
}

/*
 * OpenType script tag for characters that SScriptProcessor does not claim.
 *
 * These are plain cmap-driven scripts.  Latin/Greek/Cyrillic and symbols
 * share "latn"; CJK, Kana and Hangul have their own tags, and without the
 * right tag their GSUB/GPOS features cannot be located at all.
 */
static const char*
common_otf_script(SS_UCS4 c)
{
    if (c >= 0x3040 && c <= 0x30FF) return "kana";   /* Hiragana, Katakana */
    if (c >= 0x31F0 && c <= 0x31FF) return "kana";
    if (c >= 0xFF66 && c <= 0xFF9F) return "kana";   /* halfwidth Katakana */
    if ((c >= 0x1100 && c <= 0x11FF)                 /* Hangul Jamo */
        || (c >= 0xA960 && c <= 0xA97F)
        || (c >= 0xAC00 && c <= 0xD7AF)              /* Hangul syllables */
        || (c >= 0xD7B0 && c <= 0xD7FF)) return "hang";
    if ((c >= 0x2E80 && c <= 0x2EFF)                 /* CJK radicals */
        || (c >= 0x3000 && c <= 0x303F)              /* CJK punctuation */
        || (c >= 0x3400 && c <= 0x4DBF)              /* Ext A */
        || (c >= 0x4E00 && c <= 0x9FFF)              /* Unified */
        || (c >= 0xF900 && c <= 0xFAFF)              /* Compatibility */
        || (c >= 0xFE30 && c <= 0xFE4F)              /* CJK compat forms */
        || (c >= 0xFF00 && c <= 0xFFEF)              /* Half/fullwidth */
        || (c >= 0x20000 && c <= 0x3FFFF)) return "hani";
    return "latn";
}

/* ── Shaping API ────────────────────────────────────────────────────────── */

/*
 * Characters that SScriptProcessor::put() claims for a complex-script shaper.
 * They must not be merged into a "plain" run.
 */
static inline bool
is_complex_script_char(uint32_t c)
{
    return (c >= 0x0900 && c <= 0x0DFF)   /* Indic, incl. Sinhala */
        || (c >= 0x0E00 && c <= 0x0FFF)   /* Thai, Lao, Tibetan    */
        || (c >= 0x1100 && c <= 0x11FF);  /* Hangul Jamo           */
}

/*
 * Length of the maximal "plain" run at `start`, i.e. characters that no
 * complex-script shaper claims (Latin, Greek, Cyrillic, CJK, symbols).
 *
 * These are shaped as one run so that run-level OpenType features -
 * ligatures, contextual alternates and, most importantly, pair kerning -
 * can apply.  Feeding them one character at a time, as the original
 * fallback did, makes every one of those features impossible.
 */
static unsigned int
plain_run_length(const std::vector<SS_UCS4>& text, unsigned int start)
{
    unsigned int i = start + 1;
    for (; i < (unsigned int)text.size(); i++) {
        uint32_t c = text[i];
        if (is_complex_script_char(c)) break;
        if (c == 0x000D || c == 0x000A || c == 0x000C
            || c == 0x2028 || c == 0x2029) break;
    }
    return (i > start) ? (i - start) : 1;
}

/* ── trace conversion ───────────────────────────────────────────────────── */

static void
copy_stage_glyphs(TraceStep& ts, const std::vector<_YuditGlyphMini>& src)
{
    ts.glyphs.resize(src.size());
    for (size_t k = 0; k < src.size(); k++) {
        ts.glyphs[k].glyph_id  = src[k].glyph_id;
        ts.glyphs[k].codepoint = src[k].codepoint;
        ts.glyphs[k].x         = src[k].x;
        ts.glyphs[k].y         = src[k].y;
        ts.glyphs[k].width     = src[k].width;
        ts.glyphs[k].cluster   = src[k].cluster;
    }
}

/*
 * Flatten shaping stages into the result trace.
 *
 * Each lookup a stage ran becomes three steps, using the message vocabulary
 * of the reference trace:
 *
 *     start lookup 114 feature 'half'
 *     skipped lookup 114 feature 'half' because no glyph matches
 *     end lookup 114 feature 'half'
 *
 * `skipped` is emitted only for a lookup that no glyph matched.  `effective`
 * is set on the `end` step, and only when that lookup produced a result, so a
 * lookup that did nothing can be told apart from one that changed the buffer.
 *
 * The glyph snapshot is the state after the whole feature ran: the engine does
 * not hand out intermediate buffers per lookup, so the per-lookup steps all
 * share the feature's snapshot.
 *
 * The stage itself is kept as a leading step (Yudit's own unit of work is one
 * gsub()/gpos() call for a whole feature), which also carries the aggregate
 * `effective` flag.
 */
static void
append_trace_stages(std::vector<TraceStep>& dst,
                    const std::vector<TraceStageInfo>& src)
{
    dst.reserve(dst.size() + src.size());
    for (size_t i = 0; i < src.size(); i++) {
        const TraceStageInfo& stg = src[i];

        TraceStep stage_step;
        stage_step.message   = stg.msg;
        stage_step.depth     = 0;
        stage_step.effective = stg.effective ? 1 : 0;
        copy_stage_glyphs(stage_step, stg.glyphs);

        if (stg.lookups.empty()) {
            dst.push_back(stage_step);
            continue;
        }

        dst.push_back(stage_step);

        /* Yudit retries a feature at every position in the run, so the engine
         * reports the same lookup several times.  The reference trace lists a
         * lookup once per feature, so collapse the repeats: report the first
         * occurrence, and treat the lookup as matched when any attempt did. */
        std::vector<unsigned int> order;      /* unique lookup indices */
        std::vector<int>          any_matched;
        for (size_t li = 0; li < stg.lookups.size(); li++) {
            const LookupEvent& ev = stg.lookups[li];
            size_t slot = order.size();
            for (size_t k = 0; k < order.size(); k++) {
                if (order[k] == ev.lookup_idx) { slot = k; break; }
            }
            if (slot == order.size()) {
                order.push_back(ev.lookup_idx);
                any_matched.push_back(0);
            }
            if (ev.event == YUDIT_TRACE_LOOKUP_END && ev.matched) {
                any_matched[slot] = 1;
            }
        }

        char feature[5];
        feature[0] = stg.lookups[0].feature[0];
        feature[1] = stg.lookups[0].feature[1];
        feature[2] = stg.lookups[0].feature[2];
        feature[3] = stg.lookups[0].feature[3];
        feature[4] = '\0';

        for (size_t k = 0; k < order.size(); k++) {
            const unsigned int idx     = order[k];
            const int          matched = any_matched[k];

            char start_msg[96];
            snprintf(start_msg, sizeof(start_msg),
                     "start lookup %u feature '%s'", idx, feature);

            TraceStep ts;
            ts.message   = start_msg;
            ts.depth     = 0;
            ts.effective = 0;
            ts.glyphs    = stage_step.glyphs;
            dst.push_back(ts);

            if (!matched) {
                char skip_msg[96];
                snprintf(skip_msg, sizeof(skip_msg),
                         "skipped lookup %u feature '%s'"
                         " because no glyph matches", idx, feature);
                TraceStep sk = ts;
                sk.message = skip_msg;
                dst.push_back(sk);
            }

            char end_msg[96];
            snprintf(end_msg, sizeof(end_msg),
                     "end lookup %u feature '%s'", idx, feature);
            TraceStep en = ts;
            en.message   = end_msg;
            en.effective = matched;
            dst.push_back(en);
        }
    }
}

extern "C" YuditError
yudit_shape(const YuditFont* font,
            const uint32_t* text, uint32_t length,
            YuditResult** out)
{
    if (!font || !font->ok)  return YUDIT_ERR_BAD_FONT;
    if (!text || length == 0) return YUDIT_ERR_EMPTY_TEXT;
    if (!out) return YUDIT_ERR_BAD_FONT;

    *out = 0;

    YuditResult* result = new (std::nothrow) YuditResult();
    if (!result) return YUDIT_ERR_NOMEM;

    SFontLookup* flookup = font->font;

    /* We need to process text in clusters. The processor's put() method
     * handles one cluster at a time (one script run). For simplicity,
     * feed the entire string as one cluster. */
    std::vector<SS_UCS4> ucs4_text(length);
    for (uint32_t i = 0; i < length; i++) {
        ucs4_text[i] = (SS_UCS4)text[i];
    }

    /* Process clusters — Yudit processes one script cluster at a time.
     * We walk through the text, letting put() consume as many characters
     * as it recognises as belonging to the same script. */
    unsigned int pos = 0;
    std::vector<SS_UCS4> all_orig_chars;
    std::vector<SS_GlyphIndex> all_glyphs;
    std::vector<int> all_positions;

    /* Per-cluster trace: collect all step snapshots across all clusters */
    std::vector<ClusterTrace> all_trace;

    /* Input snapshot */
    {
        ClusterTrace input_stage;
        input_stage.message   = "input";
        input_stage.depth     = 0;
        input_stage.effective = true;
        for (uint32_t i = 0; i < length; i++) {
            YuditGlyph g;
            /* Before the glyph lookup the buffer holds codepoints, so the
             * codepoint goes in the glyph field - the same convention the
             * reference trace uses for its pre-GSUB stages. */
            g.glyph_id  = text[i];
            g.codepoint = text[i];
            g.x = 0; g.y = 0; g.width = 0; g.cluster = (int32_t)i;
            input_stage.glyphs.push_back(g);
        }
        all_trace.push_back(input_stage);
    }

    /* ── Paragraph-layer path (Arabic / Syriac / Hebrew / RTL) ─────────
     *
     * Joining scripts are *not* handled by SScriptProcessor.  Yudit does
     * Arabic/Syriac joining in SParagraph::reShape() -> SGlyph::setShape(),
     * which turns each character into the right Unicode joining form before
     * the glyph is looked up in the font.  Route those runs there.
     */
    if (yudit_uses_paragraph_path(text, length)) {
        ParagraphShapeResult pr;
        std::vector<uint32_t> plain(text, text + length);

        if (yudit_shape_paragraph(flookup, plain, pr)) {
            result->width = pr.width;
            result->glyphs.reserve(pr.glyphs.size());
            for (size_t i = 0; i < pr.glyphs.size(); i++) {
                YuditGlyph g;
                g.glyph_id  = pr.glyphs[i].glyph_id;
                g.codepoint = pr.glyphs[i].codepoint;
                g.x         = pr.glyphs[i].x;
                g.y         = pr.glyphs[i].y;
                g.width     = pr.glyphs[i].width;
                g.cluster   = pr.glyphs[i].cluster;
                result->glyphs.push_back(g);
            }
            append_trace_stages(result->trace, pr.stages);
            *out = result;
            return YUDIT_OK;
        }
        /* fall through to the SScriptProcessor path if it did not work */
    }

    /* ── BiDi reordering ─────────────────────────────────────────────── */
    /* Detect if text contains RTL characters (Arabic, Hebrew, etc.) */
    bool has_rtl = false;
    bool has_ltr = false;
    for (uint32_t i = 0; i < length; i++) {
        SS_UCS4 ch = text[i];
        /* Arabic range */
        if (ch >= 0x0600 && ch <= 0x06FF) { has_rtl = true; continue; }
        if (ch >= 0x0750 && ch <= 0x077F) { has_rtl = true; continue; }
        if (ch >= 0x08A0 && ch <= 0x08FF) { has_rtl = true; continue; }
        if (ch >= 0xFB50 && ch <= 0xFDFF) { has_rtl = true; continue; }
        if (ch >= 0xFE70 && ch <= 0xFEFF) { has_rtl = true; continue; }
        /* Hebrew range */
        if (ch >= 0x0590 && ch <= 0x05FF) { has_rtl = true; continue; }
        if (ch >= 0xFB1D && ch <= 0xFB4F) { has_rtl = true; continue; }
        /* Syriac */
        if (ch >= 0x0700 && ch <= 0x074F) { has_rtl = true; continue; }
        /* Thaana */
        if (ch >= 0x0780 && ch <= 0x07BF) { has_rtl = true; continue; }
        /* CJK and other LTR */
        if (ch >= 0x0041 && ch <= 0x005A) { has_ltr = true; } /* A-Z */
        if (ch >= 0x0061 && ch <= 0x007A) { has_ltr = true; } /* a-z */
        if (ch >= 0x0900 && ch <= 0x0DFF) { has_ltr = true; } /* Indic */
        if (ch >= 0x0E00 && ch <= 0x0E7F) { has_ltr = true; } /* Thai */
    }

    if (has_rtl && !has_ltr) {
        /* Pure RTL: reverse the codepoint order for visual rendering */
        std::vector<SS_UCS4> reordered(length);
        for (uint32_t i = 0; i < length; i++) {
            reordered[i] = ucs4_text[length - 1 - i];
        }
        ucs4_text = reordered;
        /* Emit bidi trace stage */
        ClusterTrace bidi_stage;
        bidi_stage.message   = "bidi";
        bidi_stage.depth     = 0;
        bidi_stage.effective = true;
        for (uint32_t i = 0; i < length; i++) {
            YuditGlyph g;
            g.glyph_id  = 0;
            g.codepoint = ucs4_text[i];
            g.x = 0; g.y = 0; g.width = 0;
            g.cluster = (int32_t)(length - 1 - i);
            bidi_stage.glyphs.push_back(g);
        }
        all_trace.push_back(bidi_stage);
    } else if (has_rtl && has_ltr) {
        /* Mixed: simple prepend-RTL approach (not full UAX#9, but functional) */
        /* For now, just emit a note - full BiDi is complex */
        ClusterTrace bidi_stage;
        bidi_stage.message   = "bidi (mixed, simplified)";
        bidi_stage.depth     = 0;
        bidi_stage.effective = false;
        all_trace.push_back(bidi_stage);
    }

    while (pos < length) {
        /* Create a fresh script processor for each cluster */
        YuditScriptProcessor sp(flookup);

        unsigned int consumed = sp.put(ucs4_text.data() + pos,
                                       length - pos,
                                       pos == 0);

        if (consumed == 0) {
            /* Characters no complex-script shaper claims (Latin, Greek,
             * Cyrillic, CJK, symbols).  Shape the whole run at once so that
             * run-level OpenType features - ligatures, contextual alternates
             * and pair kerning - can apply. */
            consumed = plain_run_length(ucs4_text, pos);

            YuditScriptProcessor sp2(flookup);
            sp2.m_otfScript = common_otf_script(ucs4_text[pos]);
            sp2.m_orig.clear();
            sp2.m_out.clear();
            sp2.m_out_type.clear();
            for (unsigned int ci = 0; ci < consumed && (pos + ci) < length; ci++) {
                sp2.m_orig.append(ucs4_text[pos + ci]);
            }
            sp2.m_in = sp2.m_orig;
            sp2.m_is_begin = (pos == 0);

            /* Apply GSUB/GPOS with trace */
            std::vector<TraceStageInfo> stages;
            sp2.applyWithTrace(stages);

            /* Convert stages to trace */
            for (size_t si = 0; si < stages.size(); si++) {
                const TraceStageInfo& stg = stages[si];
                ClusterTrace ct;
                ct.message   = stg.msg;
                ct.depth     = 0;
                ct.effective = stg.effective;
                ct.glyphs.resize(stg.glyphs.size());
                for (size_t k = 0; k < stg.glyphs.size(); k++) {
                    ct.glyphs[k].glyph_id  = stg.glyphs[k].glyph_id;
                    ct.glyphs[k].codepoint = stg.glyphs[k].codepoint;
                    ct.glyphs[k].x         = stg.glyphs[k].x;
                    ct.glyphs[k].y         = stg.glyphs[k].y;
                    ct.glyphs[k].width     = stg.glyphs[k].width;
                    ct.glyphs[k].cluster   = stg.glyphs[k].cluster;
                }
                all_trace.push_back(ct);
            }

            /* Store original chars */
            for (unsigned int ci = 0; ci < consumed; ci++) {
                all_orig_chars.push_back(ucs4_text[pos + ci]);
            }

            /* Get final results */
            const SV_GlyphIndex& glyphs = sp2.getGlyphs();
            const SV_INT& positions = sp2.getPositions();
            for (unsigned int i = 0; i < glyphs.size(); i++) {
                all_glyphs.push_back(glyphs[i]);
                if (i < (unsigned int)positions.size()) {
                    all_positions.push_back(positions[i]);
                } else {
                    all_positions.push_back(0);
                }
            }
            pos += consumed;
            continue;
        }

        /* Store original chars for this cluster */
        for (unsigned int i = 0; i < consumed; i++) {
            all_orig_chars.push_back(ucs4_text[pos + i]);
        }

        /* Apply shaping step-by-step with per-lookup trace capture */
        std::vector<TraceStageInfo> stages;
        sp.applyWithTrace(stages);

        /* Convert to C API trace entries (same shape as the paragraph path) */
        append_trace_stages(all_trace, stages);

        /* Get final results for this cluster */
        const SV_GlyphIndex& glyphs = sp.getGlyphs();
        const SV_INT& positions = sp.getPositions();

        for (unsigned int i = 0; i < glyphs.size(); i++) {
            all_glyphs.push_back(glyphs[i]);
            if (i < (unsigned int)positions.size()) {
                all_positions.push_back(positions[i]);
            } else {
                all_positions.push_back(0);
            }
        }

        pos += consumed;
    }

    /* Build final result glyphs.
     *
     * Yudit records an absolute pen position for every glyph plus a y offset
     * for marks.  The reference trace format wants the offset from the pen
     * position instead, so `pen += ax` and `draw at pen + dx` reproduce the
     * layout: dx is measured against the accumulated advance, not against the
     * previous glyph's absolute x.
     *
     * The advance is the magnitude of Yudit's width.  gwidth() returns a
     * negative number for glyphs with a negative left side bearing, which is
     * Yudit's marker for "align to the end of the previous character"; its own
     * positioning code takes the magnitude before advancing by it.
     */
    result->glyphs.reserve(all_glyphs.size());
    int pen = 0;
    for (unsigned int i = 0; i < all_glyphs.size(); i++) {
        YuditGlyph g;
        g.glyph_id  = all_glyphs[i];
        g.codepoint = (i < all_orig_chars.size()) ? all_orig_chars[i] : 0;

        int w = flookup->gwidth(all_glyphs[i]);
        g.width = (w < 0) ? -w : w;

        if (i < all_positions.size()) {
            int32_t xy = all_positions[i];
            int abs_x  = (int16_t)(xy & 0xffff);
            int mark_y = (int16_t)((xy >> 16) & 0xffff);
            g.x = abs_x - pen;   /* offset from the pen position */
            g.y = mark_y;        /* mark-to-base offset */
        } else {
            g.x = 0;
            g.y = 0;
        }

        g.cluster = (int32_t)i;
        pen += g.width;
        result->glyphs.push_back(g);
    }

    /* The run's advance is the sum of the reported advances, so a consumer
     * that adds them up gets the same number. */
    result->width = pen;

    /* Final snapshot */
    {
        ClusterTrace final_stage;
        final_stage.message   = "shaped";
        final_stage.depth     = 0;
        final_stage.effective = true;
        final_stage.glyphs    = result->glyphs;
        all_trace.push_back(final_stage);
    }

    /* Convert to TraceStep vector */
    result->trace.reserve(all_trace.size());
    for (size_t i = 0; i < all_trace.size(); i++) {
        TraceStep ts;
        ts.message   = all_trace[i].message;
        ts.glyphs    = all_trace[i].glyphs;
        ts.depth     = all_trace[i].depth;
        ts.effective = all_trace[i].effective ? 1 : 0;
        result->trace.push_back(ts);
    }

    *out = result;
    return YUDIT_OK;
}

extern "C" void
yudit_result_free(YuditResult* result) {
    delete result;
}

/* ── Result queries ─────────────────────────────────────────────────────── */

extern "C" uint32_t
yudit_result_glyph_count(const YuditResult* result) {
    if (!result) return 0;
    return (uint32_t)result->glyphs.size();
}

extern "C" const YuditGlyph*
yudit_result_glyphs(const YuditResult* result) {
    if (!result || result->glyphs.empty()) return 0;
    return result->glyphs.data();
}

extern "C" int
yudit_result_width(const YuditResult* result) {
    if (!result) return 0;
    return result->width;
}

extern "C" uint32_t
yudit_result_trace_count(const YuditResult* result) {
    if (!result) return 0;
    return (uint32_t)result->trace.size();
}

extern "C" const YuditTraceStage*
yudit_result_trace_stage(const YuditResult* result, uint32_t index) {
    if (!result || index >= result->trace.size()) return 0;
    const TraceStep& step = result->trace[index];

    /* Thread-local static for returning to caller.
     * This is NOT thread-safe but matches the simple C API pattern. */
    static YuditTraceStage s_stage;
    static std::vector<YuditGlyph> s_glyphs;

    s_glyphs = step.glyphs;
    s_stage.message      = step.message.c_str();
    s_stage.glyphs       = s_glyphs.data();
    s_stage.glyph_count  = (uint32_t)s_glyphs.size();
    s_stage.depth        = step.depth;
    s_stage.effective    = step.effective;

    return &s_stage;
}
