/** 
 *  Yudit Unicode Editor Source File
 *
 *  GNU Copyright (C) 1997-2023  Gaspar Sinai <gaspar@yudit.org>  
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License, version 2,
 *  dated June 1991. See file COPYYING for details.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
 
/*!
 * \file SScriptProcessor.h
 * \brief Script Processing routines.
 * \author: Gaspar Sinai <gaspar@yudit.org>
 * \version: 2000-04-23
 */
#ifndef ScriptProcessor_h 
#define ScriptProcessor_h

#include "swindow/SFontLookup.h"
#include "stoolkit/SRendClass.h"

/*!
 * \class SScriptProcessor
 * \brief A script processor that enables font-dependent
 * processing of characters in a font-independent way.
 *
 * The input is the unicode character cluster and the
 * output is the unscaled with, glyph list and 
 * the position of the glyphs in the list.
 *
 * All Yudit script processing routines should go into 
 * this class after collecting them. Currently they are
 * scattered all over the place. 
 */
class SScriptProcessor
{
public:
  enum SScriptCode { 
      SC_NONE=0, 
      SC_DEVANAGARI,
      SC_BENGALI,
      SC_GURMUKHI,
      SC_GUJARATI,
      SC_ORIYA, 
      SC_TAMIL, 
      SC_TELUGU, 
      SC_KANNADA, 
      SC_MALAYALAM, 
      SC_SINHALA, // Not done 
      SC_THAI, 
      SC_LAO, 
      SC_TIBETAN, 
      SC_JAMO, 
      SC_MAX 
  };

  SScriptProcessor (SFontLookup* font);
  ~SScriptProcessor (void);

  
  unsigned int put (const SS_UCS4* in, unsigned int len, bool is_begin);
  void apply();
  const SV_GlyphIndex& getGlyphs () const;
  const SV_INT&        getPositions () const;
  int                  getWidth () const;

  bool isSupported (SS_UCS4 c) const;
  static void support (bool on);
  static void doInit (bool on);

protected:
  /* generic */
  void decompose ();
  void reorder ();
  void reorderIndic ();
  void reorderStraight ();

  bool gindex ();

  void gsub (const char* feature);
  void gsubclean();

  void gposInit();
  void gpos (const char* feature);
  void gposFinal();

  /* util */
  unsigned int  findBaseConsonant(); // 1 based.
  unsigned int  findLastConsonant(); // 1 based.

  void replaceIn (unsigned int from, unsigned int until, 
         const SS_UCS4* with, unsigned int size);

  void replaceOut (unsigned int from, unsigned int until, 
         SS_GlyphIndex* with, unsigned int size);

  /* input */
  SV_UCS4       m_in;
  SV_UCS4       m_orig;

  /* output */
  SV_GlyphIndex m_out;
  int           m_width;
  SV_INT        m_positions;

  bool          m_is_begin;

protected:

  SBinVector<SRendClass::RType> m_out_type;

  bool allowed (const char* sub, SRendClass::RType st);

  void debugIn(const char* message) const;
  void debugOut(const char* message) const;
  void debugInOut(const char* message) const;
  void debugPos(const char* message) const;
  void debugDeltaPos(const char* message) const;

  /* temporary */
  SV_INT           m_xpos;
  SV_INT           m_ypos;
  SV_UINT          m_pos_base_index;

  /* reordering */
  SBinVector<SRendClass::RType>       m_reorder_guide;

  SS_UCS4       m_ra;
  bool          m_script_has_reph;

  SScriptCode   m_script;
  const char*   m_otfScript;

  /* an abstract font lookup */
  SFontLookup*  m_font;

};

#endif /* ScriptProcessor_h */
