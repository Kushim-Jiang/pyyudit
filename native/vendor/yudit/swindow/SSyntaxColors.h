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
 
#ifndef SSyntaxColors_h
#define SSyntaxColors_h

#include "swindow/SColor.h"

#include "stoolkit/SString.h"
#include "stoolkit/syntax/SSyntax.h"

class SSyntaxColors
{
public:
  SSyntaxColors(void);
  SSyntaxColors(const SSyntaxColors& c);
  SSyntaxColors& operator=(const SSyntaxColors& v);
  virtual ~SSyntaxColors ();
  SColor  colors [(unsigned int) SSyntax::SD_MAX];
};

#endif /* SSyntaxColors_h _h*/
