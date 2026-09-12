/**
 * yudit_gui_stubs.cpp — Stubs for GUI-only Yudit classes.
 *
 * Only SFontImpl, SFontNative, SAwt, and SFontFB are needed.
 * SColor/SPen/SImage/SEvent* are compiled from their real .cpp files.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "swindow/SFontImpl.h"
#include "swindow/SFontNative.h"
#include "swindow/SFontFB.h"
#include "swindow/SAwt.h"

/* ═══════════════════════════════════════════════════════════════════════════
 *  SFontImpl — abstract base for font delegation
 * ═══════════════════════════════════════════════════════════════════════════ */

SFontImpl::SFontImpl(const SString& n, const SString& e) : SObject(), name(n), encoding(e) {}
SFontImpl::SFontImpl(const SFontImpl& f) : SObject(), name(f.name), encoding(f.encoding) {}
SFontImpl SFontImpl::operator=(const SFontImpl& f) { name=f.name; encoding=f.encoding; return *this; }
SFontImpl::~SFontImpl() {}
void SFontImpl::setPath(const SStringVector&) {}
void SFontImpl::guessPath() {}
const SStringVector& SFontImpl::getPath() { static SStringVector p; return p; }
SObject* SFontImpl::clone() const { return 0; }
void SFontImpl::scale(double, double) {}
bool SFontImpl::draw(SCanvas*, const SPen&, const SS_Matrix2D&, SS_UCS4, bool, bool, bool) { return false; }
bool SFontImpl::width(SS_UCS4, double*) { return false; }
double SFontImpl::width() const { return 0; }
double SFontImpl::ascent() const { return 0; }
double SFontImpl::descent() const { return 0; }
double SFontImpl::gap() const { return 0; }
bool SFontImpl::isTTF() const { return false; }
bool SFontImpl::isLeftAligned(SS_UCS4) const { return false; }
void SFontImpl::setAttributes(const SProperties&) {}
bool SFontImpl::needSoftMirror(SS_UCS4, bool) const { return false; }
void SFontImpl::setBase(SS_UCS4) {}
void SFontImpl::createSaneXLFD() {}

/* ═══════════════════════════════════════════════════════════════════════════
 *  SFontNative — X11 bitmap fonts (not used in shaping)
 * ═══════════════════════════════════════════════════════════════════════════ */

SFontNative::SFontNative() {}
SFontNative::~SFontNative() {}
bool SFontNative::draw(const SString&, SCanvas*, const SPen&, const SS_Matrix2D&, SS_UCS4) { return false; }
bool SFontNative::width(const SString&, SS_UCS4, double*) { return false; }
double SFontNative::width(const SString&) { return 0; }
double SFontNative::ascent(const SString&) { return 0; }
double SFontNative::descent(const SString&) { return 0; }
double SFontNative::gap(const SString&) { return 0; }

/* ═══════════════════════════════════════════════════════════════════════════
 *  SAwt — Abstract Widget Toolkit
 * ═══════════════════════════════════════════════════════════════════════════ */

SFontNative* SAwt::getFont(const SString&) { return 0; }

/* ═══════════════════════════════════════════════════════════════════════════
 *  SFontFB — fallback font
 * ═══════════════════════════════════════════════════════════════════════════ */

SFontFB::SFontFB() {}
SFontFB::~SFontFB() {}
