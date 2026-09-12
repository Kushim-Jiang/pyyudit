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
#ifndef SFontBDF_h
#define SFontBDF_h

#include "stoolkit/SIO.h"
#include "stoolkit/SString.h"
#include "stoolkit/STypes.h"
#include "swindow/SCanvas.h"
#include "swindow/SColor.h"

/**
 * This is specifically designed for 
 * Roman Czyborrra's UniFont. The font is currently maintained by
 * David Starner <dvdeug@debian.org>
 */
class SFontBDF
{
public:
  SFontBDF (const SFile& file); 
  ~SFontBDF (); 

  bool draw (double scale, SCanvas* canvas, const SColor& fg,
       const SS_Matrix2D& matrix, SS_UCS4 g, bool mirrored);

  bool width (double scale, SS_UCS4 g, double *width_);

  double width (double scale);
  double ascent (double scale);
  double descent (double scale);
  double gap (double scale);
private:
  bool find (SS_UCS4 g);
  int  nextIndex (int from, SS_UCS4* g, int* width);
  unsigned int  nextLine (unsigned int from);

  int    iscale;
  int    iascent;
  int    idescent;
  unsigned int  size;
  SString name;

  SFile       file;
  SFileImage   image;
  const char*  array;
  int  position[0x10000];
  char widths[0x10000];
};

#endif /* SFontBDF_h */
