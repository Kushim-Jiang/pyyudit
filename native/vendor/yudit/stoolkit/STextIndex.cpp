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

#include "STextIndex.h"
#include "stoolkit/SExcept.h"

/**
 * Create a text index object
 * @param line is the line of the object
 * @param index is the character index of the object
 */
STextIndex::STextIndex (int _line, int _index)
{
  this->line = _line;
  this->index = _index;
}

/**
 * Create a dimension object
 * width default index is 0,0
 */
STextIndex::STextIndex ()
{
  line = 0;
  index = 0;
}

/**
 * Copy a textindex object
 * @param d is the textindex 
 */
STextIndex::STextIndex (const STextIndex& d)
{
  this->line = d.line;
  this->index = d.index;
}

/**
 * Copy a textindex object
 * @param d is the textindex 
 */
STextIndex
STextIndex::operator = (const STextIndex& d)
{
  this->line = d.line;
  this->index = d.index;
  return *this;
}
/**
 * Subtract a textindex object
 * @param d is the textindex 
 */
STextIndex
STextIndex::operator - (const STextIndex& d)
{
  this->line -= d.line;
  if (index > d.index)
  {
    this->index -= d.index;
  }
  else
  {
    this->line--;
    this->index = 0;
  }
  return *this;
}

/**
 * Compare the textindex object with another object
 * @param d is the textindex to compare to
 */
bool
STextIndex::operator == (const STextIndex& d) const
{
  return (d.line == line && d.index == index);
}

/**
 * Compare the textindex object with another object
 * @param d is the textindex to compare to
 */
bool
STextIndex::operator != (const STextIndex& d) const
{
  return (d.line != line || d.index != index);
}

/**
 * Compare the dimension object with another object
 * @param d is the dimension to compare to
 */
bool
STextIndex::operator < (const STextIndex& d) const
{
  if (line < d.line) return true;
  if (line > d.line ) return false;
  return (index < d.index);
}

/**
 * Compare the dimension object with another object
 * @param d is the dimension to compare to
 */
bool
STextIndex::operator > (const STextIndex& d) const
{
  if (line > d.line ) return true;
  if (line < d.line ) return false;
  return (index > d.index);
}

/**
 * Compare the dimension object with another object
 * @param d is the dimension to compare to
 */
bool
STextIndex::operator <= (const STextIndex& d) const
{
  if (line < d.line ) return true;
  if (line > d.line ) return false;
  return (index <= d.index );
}

/**
 * Compare the dimension object with another object
 * @param d is the dimension to compare to
 */
bool
STextIndex::operator >= (const STextIndex& d) const
{
  if (line > d.line ) return true;
  if (line < d.line ) return false;
  return (index >= d.index );
}

/**
 * Destructor - nothing to desctruct now..
 */
STextIndex::~STextIndex ()
{
}

SObject*
STextIndex::clone() const
{
  STextIndex* l = new STextIndex (line, index);
  CHECK_NEW (l);
  return l;
}
