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
 
#ifndef SFontFB_h
#define SFontFB_h

#include "swindow/SCanvas.h"
#include "swindow/SPen.h"
#include "stoolkit/STypes.h"
#include "stoolkit/SMatrix.h"


/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 * This is a falllback font that always have glyphs
 */
class SFontFB
{
public:
  SFontFB (void); 
  virtual ~SFontFB (); 
  enum SIGN { CR, LF, CRLF, LFCR, PS, LS, FF, TAB, CTRL, LRM, RLM, FB_ZWNJ, FB_ZWJ };

  void draw (SCanvas* canvas, const SPen& pen, const SS_Matrix2D& matrix, 
    SS_UCS4 g);

  void signDraw (SCanvas* canvas, const SPen& pen, 
     const SS_Matrix2D& matrix, SIGN sign, SS_UCS4 g); 

  double width (const SS_Matrix2D& matrix, SS_UCS4 g);

  double signWidth (const SS_Matrix2D& matrix, SIGN sign); 

  double width (const SS_Matrix2D& matrix) const;
  double ascent (const SS_Matrix2D& matrix) const;
  double descent (const SS_Matrix2D& matrix) const;
  double gap (const SS_Matrix2D& matrix) const;

  /* mulltiply this with size you want to get the matrix */
  double scale () const;
  /* in height */
  unsigned int tabSize;
private:
  void drawOne (SCanvas* canvas, const SS_Matrix2D& matrix, short* guide);
};

#endif /* SFontFB_h */
