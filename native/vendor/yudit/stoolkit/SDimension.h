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

#ifndef SDimension_h
#define SDimension_h

#include "stoolkit/SString.h"

class SDimension
{
public:
  SDimension (void);
  SDimension (int width, int height);
  SDimension (const SString& d);
  SDimension (const SDimension& d);
  SDimension operator = (const SDimension& d);
  SDimension operator - (const SDimension& d) const;
  SDimension operator * (const SDimension& d) const;
  SDimension operator * (unsigned int scale) const;
  SDimension operator / (const SDimension& d) const;
  SDimension operator + (const SDimension& d) const;
  bool operator == (const SDimension& d) const;
  bool operator != (const SDimension& d) const;
  ~SDimension ();
  SDimension minimize (const SDimension & d) const;
  SDimension maximize (const SDimension & d) const;
  unsigned int width;
  unsigned int height;
};

#endif /* SDimension_h */
