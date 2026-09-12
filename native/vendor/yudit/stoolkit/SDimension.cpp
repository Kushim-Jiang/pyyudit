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

#include "SDimension.h"
#include <stdio.h>

SDimension::SDimension (void)
{
  width = 0;
  height = 0;
}
/**
 * Create a dimension object
 * @param width is the width of the object
 * @param height is the height of the object
 */
SDimension::SDimension (int w, int h)
{
  this->width = w;
  this->height = h;
}

/**
 * Copy a dimension object
 * @param d is the dimension 
 */
SDimension::SDimension (const SDimension& d)
{
  this->width = d.width;
  this->height = d.height;
}

/**
 * Create a Dimension from a string of format %d.%d
 */
SDimension::SDimension (const SString& d)
{
  unsigned int w;
  unsigned int h;
  this->width = 10;
  this->height = 10;
  SString s(d);
  s.append ((char)0);
  if (sscanf (s.array(), "%ux%u", &w, &h) == 2 
    || sscanf (s.array(), "%uX%u", &w, &h) == 2)
  {
    this->width = w;
    this->height = h;
  }
}

/**
 * Copy a dimension object
 * @param d is the dimension 
 */
SDimension
SDimension::operator = (const SDimension& d) 
{
  this->width = d.width;
  this->height = d.height;
  return *this;
}

SDimension
SDimension::operator - (const SDimension& d) const
{
  return SDimension (width - ((d.width > width) ? width : d.width),
    height - ((d.height > height) ? height : d.height));
}

SDimension
SDimension::operator * (unsigned int scale) const
{
  return SDimension (width * scale, height * scale);
}

/**
 * The returned dimension can not be smaller than d.
 */
SDimension
SDimension::minimize (const SDimension & d) const
{
  return SDimension (
    (width < d.width) ? d.width : width,
    (height < d.height) ? d.height : height
  );
}
/**
 * The returned dimension can not be larger than d.
 */
SDimension
SDimension::maximize (const SDimension & d) const
{
  return SDimension (
    (width > d.width) ? d.width : width,
    (height > d.height) ? d.height : height
  );
}

SDimension
SDimension::operator * (const SDimension& d) const
{
  return SDimension (width * d.width, height * d.height);
}

SDimension
SDimension::operator / (const SDimension& d) const
{
  return SDimension ((d.width>0) ? width / d.width : width,
          (d.height>0) ? height / d.height : height);
}

SDimension
SDimension::operator + (const SDimension& d) const
{
  return SDimension (width + d.width, height + d.height);
}

/**
 * Compare the dimension object with another object
 * @param d is the dimension to compare to
 */
bool
SDimension::operator == (const SDimension& d) const
{
  return (d.width == width && d.height == height);
}

/**
 * Compare the dimension object with another object
 * @param d is the dimension to compare to
 */
bool
SDimension::operator != (const SDimension& d) const
{
  return (d.width != width || d.height != height);
}

/**
 * Destructor - nothing to desctruct now..
 */
SDimension::~SDimension ()
{
}
