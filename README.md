# pyyudit

Cross-platform Python binding to [Yudit](https://www.yudit.org/)'s Unicode shaping engine, with per-step trace output.

## What is this?

`pyyudit` extracts the shaping engine from the Yudit Unicode editor into a standalone shared library and wraps it with Python ctypes bindings. It provides:

- **Indic script shaping** — Devanagari, Bengali, Gujarati, Gurmukhi, Oriya, Telugu, Kannada, Malayalam, Sinhala
- **Arabic/Syriac joining** 
- **Thai, Lao, Tibetan** text processing
- **Hangul Jamo** composition
- **OpenType GSUB/GPOS** feature application
- **Full BiDi** (bidirectional text) support
- **Per-step trace** — see every decompose → reorder → gsub → gpos step

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
pip install dist/pyyudit-0.1.0-py3-none-any.whl
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

The trace captures every step of Yudit's shaping pipeline:

| Stage | Description |
|-------|-------------|
| `input` | Original codepoints before shaping |
| `decompose` | Unicode decomposition (vowel splits etc.) |
| `reorder` | Indic consonant/vowel reordering |
| `gindex` | Glyph index lookup from cmap |
| `half` | Half-form substitution (GSUB) |
| `pres` | Pre-base substitution (GSUB) |
| `abvs` | Above-base substitution (GSUB) |
| `blws` | Below-base substitution (GSUB) |
| `psts` | Post-base substitution (GSUB) |
| `haln` | Halant/virama substitution (GSUB) |
| `abvm` | Above-base mark positioning (GPOS) |
| `blwm` | Below-base mark positioning (GPOS) |
| `clean` | Post-GSUB cleanup |
| `final` | Final positioning |
| `shaped` | Final shaped glyph buffer |

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
| Devanagari | `deva` | nukt akhn rphf blwf half vatu pres abvs blws psts haln | abvm blwm dist |
| Bengali | `beng` | init nukt akhn rphf blwf half pstf vatu pres abvs blws psts | abvm blwm dist |
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

## License

- **pyyudit Python code**: MIT
- **Yudit shaping engine**: GPL-2.0-or-later (Gaspar Sinai)
- See [COPYING.TXT](yudit-3.1.0/COPYING.TXT) for details.
