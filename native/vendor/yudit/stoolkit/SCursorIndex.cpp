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

#include "SCursorIndex.h"

SCursorIndex::SCursorIndex (void)
{
  before = true; 
}

SCursorIndex::SCursorIndex (int x, int y, bool _before)
 : textIndex (x, y)
{
  before  = _before;
}

SCursorIndex::SCursorIndex (const SCursorIndex& d)
{
  textIndex = d.textIndex;
  before = d.before;
}
SCursorIndex
SCursorIndex::operator = (const SCursorIndex& d)
{
  textIndex = d.textIndex;
  before = d.before;
  return *this;
}

SCursorIndex::~SCursorIndex ()
{
}

/**
 * return the textindex calculated from 
 * this object using the before flag.
 */
STextIndex
SCursorIndex::getTextIndex() const
{
  if (before)
  {
    return STextIndex(textIndex.line, textIndex.index);
  }
  return STextIndex(textIndex.line, textIndex.index+1);
}
