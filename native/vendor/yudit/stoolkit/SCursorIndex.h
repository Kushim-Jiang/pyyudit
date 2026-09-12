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

#ifndef SCursorIndex_h
#define SCursorIndex_h

#include "stoolkit/STextIndex.h"

class SCursorIndex 
{
public:
  SCursorIndex (void);
  SCursorIndex (int x, int y, bool before=true);
  SCursorIndex (const SCursorIndex& d);

  SCursorIndex operator = (const SCursorIndex& d);
  inline bool operator == (const SCursorIndex& d) const;
  inline bool operator != (const SCursorIndex& d) const;
  ~SCursorIndex ();
  STextIndex getTextIndex() const;

  STextIndex textIndex;
  bool       before; /* logical */
};

bool
SCursorIndex::operator == (const SCursorIndex& d) const
{
  return (textIndex == d.textIndex && before == d.before);
}
bool
SCursorIndex::operator != (const SCursorIndex& d) const
{
  return (textIndex != d.textIndex || before != d.before);
}

#endif /* SCursorIndex_h */
