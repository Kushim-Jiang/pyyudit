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
 
#include "swindow/SPen.h"
/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 * This is the abstract widget toolkit font package
 */

/**
 * This class draws objects
 * @param _fore is the foreground color
 */
SPen::SPen (const SColor& _fore) : 
  fore (_fore), back (0.0, 0.0, 0.0, 0.0), width(1.0)
{
}

/**
 * This class draws objects
 * @param _fore is the foreground color
 * @param _back is the foreground color
 */
SPen::SPen (const SColor& _fore, const SColor& _back ) : 
  fore (_fore), back (_back), width(1.0)
{
}

/**
 * This class draws objects
 * @param _fore is the foreground color
 * @param _back is the foreground color
 * _width is the brush with. It can control sub-pixel redering.
 */
SPen::SPen (const SColor& _fore, const SColor& _back, double _width ) : 
  fore (_fore), back (_back), width(_width)
{
}

SPen::SPen (const SPen& pen) 
 : fore (pen.fore), back (pen.back), width (pen.width)
{
}

SPen
SPen::operator= (const SPen& pen)
{
  if (&pen == this) return *this;
  fore = pen.fore;
  back = pen.back;
  width = pen.width;
  return *this;
}

bool
SPen::operator==(const SPen& pen) const
{
  return (fore == pen.fore && back == pen.back && width == pen.width);
}

bool
SPen::operator!=(const SPen& pen) const
{
  return (fore != pen.fore || back != pen.back || width != pen.width);
}

SPen::~SPen()
{
}
const SColor&
SPen::getForeground() const
{
  return fore;
}
const SColor&
SPen::getBackground() const
{
  return back;
}

double
SPen::getLineWidth() const
{
 return width;
}

void
SPen::setForeground (const SColor& f)
{
  fore = f;
}
void
SPen::setBackground (const SColor& b)
{
  back = b;
}
/**
 * Set the width, that is mostly used for sub-pixel rendering.
 */
void
SPen::setLineWidth (double _width)
{
  width = _width;
}
