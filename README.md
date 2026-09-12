# pyyudit

Cross-platform Python binding to [Yudit](https://www.yudit.org/)'s Unicode shaping engine, with per-step trace output.

## What is this?

`pyyudit` extracts the shaping engine from the Yudit Unicode editor into a standalone shared library and wraps it with Python ctypes bindings. It provides:

- **Indic script shaping** — Devanagari, Bengali, Gujarati, Gurmukhi, Oriya, Telugu, Kannada, Malayalam, Sinhala
- **Arabic / Syriac joining** — isolated, initial, medial and final forms, plus required ligatures (lam-alef, Allah)
- **Hebrew, Thaana, N'Ko, Mandaic, Mongolian** — right-to-left ordering and joining
- **Thai, Lao, Tibetan** text processing
- **Hangul Jamo** processed through `ccmp`/`ljmo`/`vjmo`/`tjmo` (no Unicode precomposition)
- **OpenType GSUB/GPOS** feature application, including `Extension` lookups (GSUB type 7 / GPOS type 9) used by Arial, Segoe UI, Noto, …
- **BiDi** (bidirectional text) base-direction detection and RTL visual ordering
- **Per-step trace** — see every decompose → reorder → gsub → gpos step

### How Yudit shapes text

Yudit has two shaping layers, and `pyyudit` routes text through the right one:

| Layer | Handles | Where |
|-------|---------|-------|
| Paragraph layer (`SParagraph` + `SGlyph::setShape`) | Arabic/Syriac joining, Hebrew/RTL ordering, Unicode presentation forms, lam-alef ligature | `YuditParagraphShaper` |
| Script processor (`SScriptProcessor`) | Indic reordering (Devanagari … Malayalam, Sinhala), Thai, Lao, Tibetan, Jamo + their GSUB/GPOS | `YuditScriptProcessor` |

Both paths share the same OpenType engine.  Mark positioning (`mark`, `mkmk`) and
required/contextual ligatures (`rlig`, `calt`) are what Yudit implements; pair
kerning (`kern`) is not one of them - see [Limitations](#limitations).

## Architecture

```
pyyudit (Python)
├── YuditFont          — load font from bytes/path
│   ├── shape()        — one-shot shaping
│   └── shape_trace()  — shaping with full trace
├── _binding.py        — ctypes declarations for C API
├── _loader.py         — cross-platform native lib discovery
└── _lib/              — bundled native library (DLL/SO/dylib)

yudit_shaper (C/C++)
├── yudit_shaper.h/cpp — C API wrapper
├── YuditParagraphShaper.h/cpp — paragraph layer (Arabic/Syriac joining, RTL)
├── YuditScriptProcessor.h/cpp — script processor trace (Indic, Thai, Lao, …)
├── stoolkit/          — Yudit core (BiDi, Unicode, glyphs, paragraphs)
├── swindow/           — Font handling (SFontTTF) + SScriptProcessor
└── CMakeLists.txt     — build system
```

## Installation

### From source (with native build)

```bash
# Prerequisites: CMake 3.15+, C++ compiler
git clone https://github.com/Kushim-Jiang/pyyudit.git
cd pyyudit

# Download Yudit source (for the shaping engine)
curl -L https://www.yudit.org/download/yudit-3.1.0.tar.gz | tar xz

# Build the native library
python scripts/build_native.py

# Install the Python package
pip install -e ".[dev]"
```

### Cross-platform wheel

```bash
# Build wheel (includes the native library)
pip install hatchling build
python -m build
pip install dist/pyyudit-0.2.0-py3-none-any.whl
```

## Usage

```python
import pyyudit

# One-shot shaping
font_bytes = open("NotoSansDevanagari.ttf", "rb").read()
glyphs = pyyudit.shape(font_bytes, "नमस्ते")
for g in glyphs:
    print(f"gid={g.g} cluster={g.cl} adv={g.ax}")

# With full trace
with pyyudit.YuditFont.from_path("NotoSansDevanagari.ttf") as f:
    result = f.shape_trace("नमस्ते")
    for stage in result.stages:
        print(f"{stage.m}: {len(stage.glyphs)} glyphs")
        if stage.effective:
            print("  (buffer changed)")

# Reusable font object
font = pyyudit.YuditFont.from_path("NotoSansArabic.ttf")
result = font.shape("مرحبا")
print(f"Advance: {result.advance_x} units")
font.close()
```

## Trace stages

The trace captures every step of Yudit's shaping pipeline.  Which stages you
see depends on the script:

| Stage | Path | Description |
|-------|------|-------------|
| `input` | both | Original codepoints before shaping |
| `decompose` | Indic | Unicode decomposition (vowel splits etc.) |
| `reorder` | Indic | Indic consonant/vowel reordering |
| `split` | joining | Paragraph split, composition and clustering |
| `join` | joining | Joining form selected (`isolated`/`initial`/`medial`/`final`) |
| `gindex` | both | Glyph index lookup from cmap |
| `half` | Indic | Half-form substitution (GSUB) |
| `pres` | Indic | Pre-base substitution (GSUB) |
| `abvs` | Indic | Above-base substitution (GSUB) |
| `blws` | Indic | Below-base substitution (GSUB) |
| `psts` | Indic | Post-base substitution (GSUB) |
| `haln` | Indic | Halant/virama substitution (GSUB) |
| `ccmp` | plain + joining | Glyph composition/decomposition (GSUB) |
| `rlig` | plain + joining | Required ligatures — lam-alef, Allah (GSUB) |
| `calt` | plain + joining | Contextual alternates (GSUB) |
| `liga` | joining | Standard ligatures (GSUB) |
| `abvm` | Indic | Above-base mark positioning (GPOS) |
| `blwm` | Indic | Below-base mark positioning (GPOS) |
| `mark` | plain + joining | Mark-to-base positioning (GPOS) |
| `mkmk` | plain + joining | Mark-to-mark positioning (GPOS) |
| `kern` | plain + joining | Pair kerning (GPOS type 2) - Yudit's handler only inspects coverage, so this stage never reports a position |
| `clean` | Indic | Post-GSUB cleanup |
| `gpos_final` | Indic | Final positioning |
| `reorder` | joining | Right-to-left visual ordering (emitted when the run is RTL) |
| `shaped` | both | Final shaped glyph buffer |

Each stage carries the full glyph buffer at that point, so `stage.glyphs`
is a snapshot you can diff against the previous stage.  `stage.effective`
is `True` when that step actually changed the buffer — a substitute that found
nothing, or a reorder that was already in order, reports `False`.

Every GSUB/GPOS feature stage is followed by one group of steps per lookup the
feature ran, using the same message vocabulary as the reference trace:

```
start lookup 114 feature 'half'
skipped lookup 114 feature 'half' because no glyph matches
end lookup 114 feature 'half'
```

`skipped` appears only when no glyph matched.  Each `start`/`end` pair holds the
glyph snapshot as it is after the whole feature ran — the engine does not expose
a buffer per lookup, so the per-lookup steps share the feature's snapshot.
`effective` sits on the `end` step and marks the lookup that changed the buffer.
Yudit retries a feature at every position in the run; the repeats are collapsed
so each lookup is listed once per feature.

> `g` holds the **Unicode codepoint** in the stages before glyph lookup
> (`input`, `decompose`, `reorder`, `split`, `join`) and the **glyph index**
> afterwards, which is how the reference trace reports an unmapped buffer.
> `cl` is the index into the original text.

> In the joining path the glyphs are returned in **visual order** (reversed for
> RTL runs, like HarfBuzz) and `dx` is the pen delta between consecutive output
> glyphs, so `pen += dx` reproduces the exact positions.

`tests/compare_trace.py` lays the trace next to a HarfBuzz trace of the same
text, so the lookup count, the feature sequence and the resulting buffer can be
compared step by step.

## Pure-Python helpers

These work without the native library:

```python
from pyyudit import upem_from_ttf, has_table, read_font_bytes

data = read_font_bytes("font.ttf")
upem = upem_from_ttf(data)
has_gsub = has_table(data, "GSUB")
```

## Configuration

```python
# Set explicit library path
pyyudit.configure("/path/to/libyudit_shaper.so")

# Or via environment variable
# export PYYUDIT_LIBRARY_PATH=/path/to/libyudit_shaper.so

# Check availability
print(pyyudit.is_available())
print(pyyudit.library_info())
```

## Supported scripts

Yudit's shaping engine supports these scripts (with OpenType features):

| Script | Tag | GSUB features | GPOS features |
|--------|-----|---------------|---------------|
| Plain (Latin, Greek, Cyrillic, CJK, symbols) | `latn` `hani` `kana` `hang` | ccmp isol fina medi init rlig calt | kern mark mkmk |
| Devanagari | `deva` | nukt akhn rphf blwf half vatu pres abvs blws psts haln | abvm blwm dist || Bengali | `beng` | init nukt akhn rphf blwf half pstf vatu pres abvs blws psts | abvm blwm dist |
| Gujarati | `gujr` | nukt akhn rphf blwf half pstf vatu pres abvs blws psts haln | abvm blwm dist |
| Gurmukhi | `guru` | nukt blwf pstf vatu pres abvs blws psts haln | abvm blwm dist |
| Oriya | `orya` | nukt akhn rphf blwf half pstf vatu pres blws abvs psts haln | abvm blwm dist |
| Tamil | `taml` | akhn half pres abvs blws psts haln | abvm blwm dist |
| Telugu | `telu` | akhn blwf abvs blws psts haln | abvm blwm dist |
| Kannada | `knda` | nukt akhn rphf blwf half pstf vatu pres blws abvs psts haln | abvm blwm dist |
| Malayalam | `mlym` | nukt akhn rphf blwf half pstf vatu pres blws abvs psts haln | abvm blwm dist |
| Sinhala | `sinh` | nukt akhn rphf blwf half pstf vatu pres blws abvs psts haln | abvm blwm dist |
| Thai | `thai` | ccmp | kern mark mkmk |
| Lao | `lao ` | ccmp blws abvs | kern mark mkmk |
| Tibetan | `tibt` | ccmp blws abvs | blwm abvm kern |
| Jamo | `jamo` | ccmp ljmo vjmo tjmo | — |

Joining / right-to-left scripts go through the paragraph layer instead:

| Script | Tag | Direction | Shaping |
|--------|-----|-----------|---------|
| Arabic | `arab` | RTL | joining forms via Unicode shape tables, then `rlig` `calt` `liga` |
| Syriac | `syrc` | RTL | joining forms, incl. the Alaph end-of-word rules |
| Hebrew | `hebr` | RTL | RTL ordering (no joining) |
| Thaana | `thaa` | RTL | RTL ordering + joining |
| N'Ko | `nko ` | RTL | RTL ordering + joining |
| Mandaic | `mand` | RTL | RTL ordering + joining |
| Mongolian | `mong` | LTR | joining forms |

### Kerning

Yudit does not kern.  Pair adjustment (`kern`, GPOS lookup type 2) exists as a
function in the engine but only checks coverage and always returns false, and the
legacy `kern` table code sits behind `#if 0`.  The `kern` stage therefore shows up
in the trace as *not effective* for every font.  This is reported as-is rather
than implemented: the point of the trace is to show what Yudit does.

### Limitations

- Pair kerning (`kern`) and legacy `kern` tables are absent, as described above.
- GPOS **cursive attachment** (`curs`, lookup type 3) is not implemented. Fonts that
  encode Arabic joining adjustments there (Dubai, for example) will miss those deltas.
- Chained contextual positioning (lookup type 8) is only partially implemented.
- Hangul jamo are not precomposed: Yudit runs `ljmo`/`vjmo`/`tjmo` and leaves the
  jamo as separate glyphs when the font does not carry those features.
- Reordering is per run: the base direction is detected with UAX#9 P2/P3 and the run is
  reversed for RTL. Multi-run BiDi is the caller's job, as it is with HarfBuzz.
- `face_index` on `yudit_font_open` is currently ignored and font collections (TTC) are
  not supported.

## License

- **pyyudit Python code**: MIT
- **Yudit shaping engine**: GPL-2.0-or-later (Gaspar Sinai)
- See [COPYING.TXT](yudit-3.1.0/COPYING.TXT) for details.
