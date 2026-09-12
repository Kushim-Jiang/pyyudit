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

#include "SLocation.h"
#include "stoolkit/SExcept.h"
#include "stoolkit/SUtil.h"

/**
 * Create a dimension object
 * @param x is the x of the object
 * @param y is the y of the object
 */
SLocation::SLocation (int x0, int y0)
{
  this->x = x0;
  this->y = y0;
}

/**
 * Create a dimension object
 * width default location is 0,0
 */
SLocation::SLocation ()
{
  x = 0;
  y = 0;
}

/**
 * Copy a dimension object
 * @param d is the dimension 
 */
SLocation::SLocation (const SLocation& d)
{
  this->x = d.x;
  this->y = d.y;
}
/**
 * Create a location object from another one.
 * @param d is the dimension 
 */
SLocation::SLocation (const SDimension& d)
{
  this->x = d.width;
  this->y = d.height;
}

/**
 * Copy a dimension object
 * @param d is the dimension 
 */
SLocation
SLocation::operator = (const SLocation& d)
{
  this->x = d.x;
  this->y = d.y;
  return *this;
}

/**
 * Compare the dimension object with another object
 * @param d is the dimension to compare to
 */
bool
SLocation::operator == (const SLocation& d) const
{
  return (d.x == x && d.y == y);
}

/**
 * Compare the dimension object with another object
 * @param d is the dimension to compare to
 */
bool
SLocation::operator != (const SLocation& d) const
{
  return (d.x != x || d.y != y);
}

/**
 * Compare the dimension object with another object
 * @param d is the dimension to compare to
 */
bool
SLocation::operator < (const SLocation& d) const
{
  if (y < d.y ) return true;
  if (y > d.y ) return false;
  return (x < d.x );
}

/**
 * Compare the dimension object with another object
 * @param d is the dimension to compare to
 */
bool
SLocation::operator > (const SLocation& d) const
{
  if (y > d.y ) return true;
  if (y < d.y ) return false;
  return (x > d.x );
}

/**
 * Compare the dimension object with another object
 * @param d is the dimension to compare to
 */
bool
SLocation::operator <= (const SLocation& d) const
{
  if (y < d.y ) return true;
  if (y > d.y ) return false;
  return (x <= d.x );
}

/**
 * Compare the dimension object with another object
 * @param d is the dimension to compare to
 */
bool
SLocation::operator >= (const SLocation& d) const
{
  if (y > d.y ) return true;
  if (y < d.y ) return false;
  return (x >= d.x );
}
SLocation
SLocation::operator - (const SLocation& d) const
{

  return SLocation (x - d.x, y - d.y);
}

SLocation
SLocation::operator + (const SLocation& d) const
{
  return SLocation (x + d.x, y + d.y);
}

SLocation
SLocation::operator + (const SDimension& d) const
{
  return SLocation (x + (int)d.width, y + (int)d.height);
}

SLocation
SLocation::operator * (const SDimension& d) const
{
  return SLocation (x * (int)d.width, y * (int)d.height);
}

/**
 * Destructor - nothing to desctruct now..
 */
SLocation::~SLocation ()
{
}

SObject*
SLocation::clone() const
{
  SLocation* l = new SLocation (x, y);
  CHECK_NEW (l);
  return l;
}

SLocation
SLocation::operator / (const SDimension& d) const
{
  return SLocation ((d.width>0) ? x / d.width : x,
          (d.height>0) ? y / d.height : y);
}

/**
 * The returned location can not be larger than d.
 */
SLocation
SLocation::maximize (const SLocation & d) const
{
  return SLocation (
    (x > d.x) ? d.x : x,
    (y > d.y) ? d.y : y
  );
}

/**
 * Assume SLocation is a vector, create the scalar product with l
 * @param l is the other vector.
 */
int
SLocation::scalarProduct (const SLocation &l) const
{
  return x * l.x + y * l.y;
}

/**
 * Assume SLocation is a vector, create the vectorial product with l
 * @param l is the other vector.
 */
int 
SLocation::vectorProduct (const SLocation &l) const
{
  return (x * l.y) - (y * l.x);
}

/**
 * The returned location can not be smaller than d.
 */
SLocation
SLocation::minimize (const SLocation & d) const
{
  return SLocation (
    (x < d.x) ? d.x : x,
    (y < d.y) ? d.y : y
  );
}
/**
 * The returned location can not be larger than d.
 */
SLocation
SLocation::maximize (const SDimension & d) const
{
  return SLocation (
    (x > (int)d.width) ? (int)d.width : x,
    (y > (int)d.height) ? (int)d.height : y
  );
}

/**
 * The returned location can not be smaller than d.
 */
SLocation
SLocation::minimize (const SDimension & d) const
{
  return SLocation (
    (x < (int)d.width) ? (int)d.width : x,
    (y < (int)d.height) ? (int)d.height : y
  );
}
SLocation
SLocation::minmerge (const SLocation & l) const 
{
  SLocation ret = *this;
  if (l.x < ret.x) ret.x = l.x;
  if (l.y < ret.y) ret.y = l.y;
  return SLocation (ret);
}

SLocation
SLocation::maxmerge (const SLocation & l) const 
{
  SLocation ret = *this;
  if (l.x > ret.x) ret.x = l.x;
  if (l.y > ret.y) ret.y = l.y;
  return SLocation (ret);
}

/**
 * return the distance from origo squared
 */
unsigned long
SLocation::distance2 () const 
{
  return (long)x*(long)x + (long)y*(long)y;
}

/**
 * return the distance from origo 
 */
unsigned int
SLocation::distance () const 
{
  return (unsigned int) ss_sqrtlong (distance2());
}

/**
 * return the angle from origo
 * in a 0..31 scale 32 is an undeterminable case.
 */
int
SLocation::angle32 () const 
{
  return ss_atan32 (x, y);
}
