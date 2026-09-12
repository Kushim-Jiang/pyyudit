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

#ifndef SRectangle_h
#define SRectangle_h

#include "stoolkit/SString.h"

class SRectangle
{
public:
  SRectangle (void);
  SRectangle (int originX, int originY, unsigned int width, unsigned int height);
  SRectangle (const SString& d);
  SRectangle (const SRectangle& d);
  SRectangle operator = (const SRectangle& d);
  bool operator == (const SRectangle& d) const;
  bool operator != (const SRectangle& d) const;
  ~SRectangle ();
  SRectangle minimize (const SRectangle & d) const;
  SRectangle maximize (const SRectangle & d) const;
  int originX;
  int originY;
  unsigned int width;
  unsigned int height;
};

#endif /* SRectangle_h */
