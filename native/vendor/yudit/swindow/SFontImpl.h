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
 
#ifndef SFontImpl_h
#define SFontImpl_h

#include "swindow/SCanvas.h"
#include "swindow/SPen.h"

#include "stoolkit/SMatrix.h"
#include "stoolkit/SStringVector.h"
#include "stoolkit/SProperties.h"
#include "stoolkit/SObject.h"


/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 * This is the abstract widget toolkit font package
 */
class SFontImpl : public SObject
{
public:
  SFontImpl (const SString& name, const SString& encoding); 
  SFontImpl (const SFontImpl& ff);
  SFontImpl operator= (const SFontImpl& ff);
  virtual ~SFontImpl (); 

  static void setPath (const SStringVector& path);
  static void guessPath();
  static const SStringVector& getPath();

  virtual SObject*  clone() const;

  void   scale (double x, double y);

  bool draw (SCanvas* canvas, const SPen& pen, 
       const SS_Matrix2D& matrix, SS_UCS4 uch, bool isLRContext, 
       bool isSelected, bool baseOK);
  bool width (SS_UCS4 uch, double* width);

  double width () const;

  double ascent () const;

  double descent () const;

  double gap () const;
  bool isTTF() const;
  bool isLeftAligned(SS_UCS4 uch) const;

  void setAttributes (const SProperties& properties);
  bool isLR () const { return lrFont; }
  bool isRL () const { return rlFont; }
  bool needSoftMirror (SS_UCS4 uch, bool isLRContext) const;

  /* set the base character for better glyph positioning */
  static void setBase(SS_UCS4 base);

private:
  void createSaneXLFD ();
  SString xlfd;
  SString name;
  SString encoding;
  SS_Matrix2D matrix;
  bool lrFont;
  bool rlFont;
  void* delegate;
};

#endif /* SFontImpl_h */
