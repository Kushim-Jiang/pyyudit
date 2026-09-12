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
 
#ifndef SFontLookup_h 
#define SFontLookup_h

/*!
 * \file SFontLookup.h
 * \brief A common set of routines that a font should have. 
 * \author: Gaspar Sinai <gaspar@yudit.org>
 * \version: 2000-04-23
 */

#include "stoolkit/STypes.h"

/*!
 * \class SFontLookup
 * \brief This interface hides the font lookup details so
 *  the ScriptProcessor can work on any font that implements it.
 */
class SFontLookup
{
public:
  virtual ~SFontLookup() {}
  /*!
  * \brief Find the glyph index for the character. 
  * \param in is the unicode-encoded character.
  * \return the glyph index or null.
  */
  virtual SS_GlyphIndex gindex (SS_UCS4 in)=0;

  /*!
   * \brief Get the width of the unscaled glyph.
   * \param in is the glyph index.
   * \return the width of the glyph.
   *  If width is negative or 0, then this mark
   *  should be aligned to the end of the previous
   *  character:
   *   <-------base------->
   *   x-------x----------x 
   *           <--length-->
   */
  virtual int gwidth (SS_GlyphIndex in)=0;

  /*!
   * \brief Perform a glyph/ligature substitution.
   * \param feature is a 4-character OTF-feature, like "gsub" 
   * \param in is the input glyph index array.
   * \param in_size is the size of input glyph index array.
   * \param start is the starting point of the substitution
   * \param out is the output will be used to put the Glyphs in.
   *      At least the input glyph size should be allocated.
   * \param out_size will contain how many glyphs were placed in out 
   *      array.
   * \param script is the code of the script, like "deva" 
   * \param is_contextual will be true after a successful 
   *     chaining contextual substitution.
   * \return how many glyphs should disappear from the input 
   *     array between start...start + retvle -1.
   */
  virtual unsigned int gsub (const char* script, const char* feature, 
      const SS_GlyphIndex* in, unsigned int in_size,
      unsigned int* start,  SS_GlyphIndex* out, unsigned int* out_size,
      bool* is_contextual)=0;

  /*!
   * \brief Get the relative position of glyphs.
   * \param script is the code of the script, like "deva" 
   * \param feature is a 4-character OTF-feature, like "blwm" 
   * \param in contains 2 glyphs. The first glyph assumed to be 
   *     at position 0.0.
   * \param x will contain the relative x position of the
   *     second glyph. If no position can be retrieved the
   *     method will not change the value of this. 
   * \param y will contain the relative y position of the
   *     second glyph. If no position can be retrieved the
   *     method will not change the value of this. 
   * \return true if a position was retrieved.
   */
  virtual bool gpos (const char* script, const char* feature, 
      const SS_GlyphIndex* in, int* x,  int* y)=0;

  /*!
   * \return the glyph class:
   *   \li 0 - Unknown
   *   \li 1 - Base Glyph (single character spacing glyph)
   *   \li 2 - Base Glyph (single character spacing glyph)
   *   \li 3 - Mark Glyph (non-spacing combining glyph)
   *   \li 4 - Component Glyph (part of a single character, spacing glyph)
  */
  virtual unsigned int getGlyphClass(SS_GlyphIndex in) = 0;

  /*!
   * \brief try to attach mark to base.
   * \param where takes the following values:
   *   1 - below.
   * \return true on success. 
  */
  virtual bool attach (SS_GlyphIndex base, SS_GlyphIndex mark,
    int where, int* x, int* y) = 0;
  
};

#endif /* SFontLookup_h */
