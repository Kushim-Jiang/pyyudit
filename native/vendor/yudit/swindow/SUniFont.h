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
#ifndef SUniFont_h
#define SUniFont_h

#include "stoolkit/SIO.h"
#include "stoolkit/SString.h"
#include "stoolkit/STypes.h"
#include "stoolkit/SBinHashtable.h"
#include "swindow/SCanvas.h"
#include "swindow/SColor.h"

/* We stuff with and pos info int and int:
   theoretical limit: 262,144 MBytes fonts size */
#define SD_POS_MASK    0x0fffffff
#define SD_WIDTH_MASK  0xf0000000
#define SD_WIDTH_SHIFT 28

#define SD_GET_POS(_x) ((unsigned int)((_x)&SD_POS_MASK))
#define SD_GET_WIDTH(_x) ((unsigned int)(((_x)&SD_WIDTH_MASK)>>SD_WIDTH_SHIFT))
#define SD_GET_POSWIDTH(_pos,_width) ((unsigned int)((_pos) | ((_width) << SD_WIDTH_SHIFT)))

/**
 * This is specifically designed for 
 * Roman Czyborrra's UniFont. The font is currently maintained by
 * David Starner <dvdeug@debian.org>
 */
class SUniFont
{
public:
  SUniFont (const SFile& file); 
  ~SUniFont (); 

  bool draw (double scale, SCanvas* canvas, const SColor& fg,
       const SS_Matrix2D& matrix, SS_UCS4 g, bool mirrored);

  bool width (double scale, SS_UCS4 g, double *width_);

  double width (double scale);
  double ascent (double scale);
  double descent (double scale);
  double gap (double scale);
private:
  bool width (double scale, SS_UCS4 g, double *width_, unsigned int* pw);
  unsigned int find (SS_UCS4 g);
  int  nextIndex (int from, int size, SString* g);

  SFile       file;
  SFileImage   image;
  const char*  array;

  SBinHashtable<unsigned int> posWidth;
};

#endif /* SUniFont_h */
