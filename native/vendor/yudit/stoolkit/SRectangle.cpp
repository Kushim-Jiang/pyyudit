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

#include "SRectangle.h"
#include <stdio.h>

SRectangle::SRectangle (void)
{
  width = 0;
  height = 0;
  originX = 0;
  originY = 0;
}
/**
 * Create a dimension object
 * @param width is the width of the object
 * @param height is the height of the object
 */
SRectangle::SRectangle (int ox, int oy, unsigned int w, unsigned int h)
{
  this->width = w;
  this->height = h;
  this->originX = ox;
  this->originY = oy;
}

/**
 * Copy a dimension object
 * @param d is the dimension 
 */
SRectangle::SRectangle (const SRectangle& d)
{
  this->width = d.width;
  this->height = d.height;
  this->originX = d.originX;
  this->originY = d.originY;
}

/**
 * Create a Dimension from a string of format %d.%d
 */
SRectangle::SRectangle (const SString& d)
{
  unsigned int w;
  unsigned int h;
  int ox;
  int oy;
  this->width = 10;
  this->height = 10;
  SString s(d);
  s.append ((char)0);
  if (sscanf (s.array(), "%ux%u@%d,%d", &w, &h, &ox, &oy) == 4 
    || sscanf (s.array(), "%uX%u@%d,%d", &w, &h, &ox, &oy) == 4)
  {
    this->originX = ox;
    this->originY = oy;
    this->width = w;
    this->height = h;
  }
}

/**
 * Copy a dimension object
 * @param d is the dimension 
 */
SRectangle
SRectangle::operator = (const SRectangle& d) 
{
  this->originX = d.originX;
  this->originY = d.originY;
  this->width = d.width;
  this->height = d.height;
  return *this;
}

/**
 * Compare the dimension object with another object
 * @param d is the dimension to compare to
 */
bool
SRectangle::operator == (const SRectangle& d) const
{
  return (d.width == width || d.height == height || d.originX == originX || d.originY == originY);
}

/**
 * Compare the dimension object with another object
 * @param d is the dimension to compare to
 */
bool
SRectangle::operator != (const SRectangle& d) const
{
  return (d.width != width || d.height != height || d.originX != originX || d.originY != originY);
}

/**
 * Destructor - nothing to desctruct now..
 */
SRectangle::~SRectangle ()
{
}
