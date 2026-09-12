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
 
#ifndef SFont_h
#define SFont_h

#include "swindow/SFontImpl.h"
#include "swindow/SCanvas.h"
#include "swindow/SPen.h"

#include "stoolkit/SStringVector.h"
#include "stoolkit/SUniMap.h"
#include "stoolkit/STypes.h"
#include "stoolkit/SVector.h"
#include "stoolkit/SBinHashtable.h"
#include "stoolkit/STextData.h"

/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 * This is the abstract widget toolkit font package
 */

#define SD_XLFD_ANY "-*-*-*-*-*-*-*-*-*-*-*-*-*-*"


typedef SVector<SFontImpl> SFontImplVector;

class SFont
{
public:
  /* This is a little help in xlfd */
  enum SE_Weight {ANYWEIGHT, REGULAR, MEDIUM, DEMIBOLD, BOLD};
  enum SE_Slant {ANYSLANT, ROMAN, ITALIC, OBLIQUE};
  enum SE_Spacing {ANYSPACING, MONOSPACE, CONDENSED, PROPORTIONAL};

  SFont (void);
  SFont (const SString name);
  SFont (const SString name, double size);
  SFont (const SFont& font);
  SFont operator = (const SFont& font);
  virtual ~SFont();

  static void put (const SString name, const SFontImplVector& list);
  static void clear();

  void   setSize (double s);

  double getHeight() const;
  double getAscent() const;
  double getSize() const;

  void draw (SCanvas* canvas, const SPen& pen, 
       const SS_Matrix2D& matrix, const SGlyph& glyph);

  double width (const SGlyph& glyph);

  double width () const;

  double ascent () const;

  double descent () const;

  double gap () const;

  static const SString& getDefaultFontList();

private:
  inline void setBase (SS_UCS4 base);
  double fontWidth;
  double fontAscent;
  double fontDescent;
  double fontGap;
  double fontScale;
  double fallbackScale;
  SString name;
  SString xlfd;
  SFontImplVector fontVector;
};

/**
 * Set the base for diacritical marks 
 */
void
SFont::setBase(SS_UCS4 base)
{
  SFontImpl::setBase(base);
}

#endif /* SFont_h */
