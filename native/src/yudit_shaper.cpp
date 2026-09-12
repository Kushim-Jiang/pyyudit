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
#include "swindow/SFontTTF.h"
#include "swindow/SFontLookup.h"
#include "YuditScriptProcessor.h"

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

#define PYYUDIT_VERSION "0.1.2"

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
     * We bundle indic.my next to the shared library. */
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
    } else {
        dll_path[0] = '\0';
    }
    std::string data_dir = std::string(dll_path) + "\\data";
#else
    /* On Linux/macOS, use the library's directory */
    Dl_info info;
    std::string data_dir;
    if (dladdr((void*)&ensure_initialized, &info) && info.dli_fname) {
        std::string lib_path = info.dli_fname;
        size_t last_slash = lib_path.find_last_of('/');
        if (last_slash != std::string::npos) {
            data_dir = lib_path.substr(0, last_slash) + "/../data";
        }
    }
#endif

    /* Set the search path for SUniMap */
    SStringVector path;
    path.append(data_dir.c_str());
    /* Also try the current directory */
    path.append(".");
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

/* ── Glyph snapshot for trace ───────────────────────────────────────────── */

static std::vector<YuditGlyph>
snapshot_glyphs(const SV_GlyphIndex& gi,
                const SV_INT& positions,
                SFontLookup* font,
                const SS_UCS4* orig_chars,
                unsigned int count)
{
    std::vector<YuditGlyph> result;
    result.reserve(gi.size());

    for (unsigned int i = 0; i < gi.size(); i++) {
        YuditGlyph g;
        g.glyph_id  = gi[i];
        g.codepoint = (i < count && orig_chars) ? orig_chars[i] : 0;

        if (i < (unsigned int)positions.size()) {
            int32_t xy = positions[i];
            g.x = (int16_t)(xy & 0xffff);
            g.y = (int16_t)((xy >> 16) & 0xffff);
        } else {
            g.x = 0;
            g.y = 0;
        }

        g.width = font ? font->gwidth(gi[i]) : 0;
        g.cluster = (int32_t)i;

        result.push_back(g);
    }
    return result;
}

/* ── Shaping API ────────────────────────────────────────────────────────── */

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
    int total_width = 0;

    /* Per-cluster trace: collect all step snapshots across all clusters */
    struct ClusterTrace {
        std::string                  message;
        std::vector<YuditGlyph>     glyphs;
        int                         depth;
        bool                        effective;
    };
    std::vector<ClusterTrace> all_trace;

    /* Input snapshot */
    {
        ClusterTrace input_stage;
        input_stage.message   = "input";
        input_stage.depth     = 0;
        input_stage.effective = true;
        for (uint32_t i = 0; i < length; i++) {
            YuditGlyph g;
            g.glyph_id  = 0;
            g.codepoint = text[i];
            g.x = 0; g.y = 0; g.width = 0; g.cluster = (int32_t)i;
            input_stage.glyphs.push_back(g);
        }
        all_trace.push_back(input_stage);
    }

    while (pos < length) {
        /* Create a fresh script processor for each cluster */
        YuditScriptProcessor sp(flookup);

        unsigned int consumed = sp.put(ucs4_text.data() + pos,
                                       length - pos,
                                       pos == 0);

        if (consumed == 0) {
            /* Script not supported — advance by one codepoint */
            consumed = 1;
            SS_UCS4 ch = ucs4_text[pos];
            SS_GlyphIndex gid = flookup->gindex(ch);
            all_glyphs.push_back(gid);
            all_positions.push_back(0);
            all_orig_chars.push_back(ch);
            total_width += flookup->gwidth(gid);
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

        /* Convert TraceStageInfo to C API TraceStep entries.
         * For each stage that has per-lookup events, emit one
         * sub-stage per lookup — this matches HarfBuzz's per-lookup trace. */
        for (size_t si = 0; si < stages.size(); si++) {
            const TraceStageInfo& stg = stages[si];

            if (stg.lookups.empty()) {
                /* No per-lookup events — emit a single stage */
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
            } else {
                /* Emit one sub-stage per lookup event */
                for (size_t li = 0; li < stg.lookups.size(); li++) {
                    const LookupEvent& ev = stg.lookups[li];
                    ClusterTrace ct;
                    /* Build message: "lookup NNN feature 'xxxx' [matched]" */
                    char msg[64];
                    snprintf(msg, sizeof(msg), "lookup %u feature '%s'%s",
                             ev.lookup_idx,
                             ev.feature,
                             ev.matched ? " matched" : "");
                    ct.message   = msg;
                    ct.depth     = 0;
                    ct.effective = (li == stg.lookups.size() - 1) ? stg.effective : false;
                    /* Use the stage's glyph snapshot for all sub-stages */
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
            }
        }

        /* Get final results for this cluster */
        const SV_GlyphIndex& glyphs = sp.getGlyphs();
        const SV_INT& positions = sp.getPositions();
        int width = sp.getWidth();

        for (unsigned int i = 0; i < glyphs.size(); i++) {
            all_glyphs.push_back(glyphs[i]);
            if (i < (unsigned int)positions.size()) {
                all_positions.push_back(positions[i]);
            } else {
                all_positions.push_back(0);
            }
        }
        total_width += width;

        pos += consumed;
    }

    /* Build final result glyphs */
    result->width = total_width;
    result->glyphs.reserve(all_glyphs.size());
    int prev_x = 0;
    for (unsigned int i = 0; i < all_glyphs.size(); i++) {
        YuditGlyph g;
        g.glyph_id  = all_glyphs[i];
        g.codepoint = (i < all_orig_chars.size()) ? all_orig_chars[i] : 0;

        if (i < all_positions.size()) {
            int32_t xy = all_positions[i];
            int abs_x = (int16_t)(xy & 0xffff);
            int mark_y = (int16_t)((xy >> 16) & 0xffff);
            g.x = abs_x - prev_x;  /* relative dx */
            g.y = mark_y;           /* mark-to-base dy */
            prev_x = abs_x;
        } else {
            g.x = 0;
            g.y = 0;
        }

        g.width = flookup->gwidth(all_glyphs[i]);
        g.cluster = (int32_t)i;
        result->glyphs.push_back(g);
    }

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
