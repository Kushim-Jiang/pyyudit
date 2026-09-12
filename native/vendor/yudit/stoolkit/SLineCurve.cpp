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

#include "SLineCurve.h"
#include "stoolkit/SExcept.h"
#include "stoolkit/SUtil.h"

/**
 * A stroke is a path, a collection of SLocation points.
 * This object provides extra functionality:
 * it caches the length of the path as well as the length
 * distance between elements. Also it can calculate the
 * closest distance between two points. 
 * Gaspar Sinai
 */

/**
 * Create an empty stroke.
 */
SLineCurve::SLineCurve (void)
{
}

/**
 * Shallow copy this stroke.
 */
SLineCurve::SLineCurve (const SLineCurve& d)
{
  /* no need to check if we are 'd' */
  vectors = d.vectors;
  lengths = d.lengths;
}
/**
 */
SLineCurve
SLineCurve::operator=(const SLineCurve& d)
{
  if (&d == this) return *this;
  vectors = d.vectors;
  lengths = d.lengths;
  return *this;
}

SLineCurve::~SLineCurve ()
{
  /* nothing to do */
}

/**
 * Obligatory SObject stuff.
 */
SObject*
SLineCurve::clone() const
{
  SLineCurve* s = new SLineCurve (*this);
  CHECK_NEW (s);
  return s;
}

/**
 * @return the full length of the path - open path.
 */
unsigned int
SLineCurve::length () const
{
  return length (0, lengths.size()-1);
}

/**
 * Get the length of this segment.
 * @param segment is greater than zero and less than size
 * at least two elements to calculate this.
 */
unsigned int
SLineCurve::length (unsigned int segment) const
{
  return length (segment-1, segment);
}
/**
 * Get the length from and not including to.
 * @param from start point
 * @param to end point.
 * Points can not be reversed.
 */
unsigned int
SLineCurve::length (unsigned int from, unsigned int to) const
{
  return lengths[to] - lengths[from];
}

/**
 * Calculate the distance between the first and the last point 
 */
unsigned int
SLineCurve::distance () const
{
  if (vectors.size()==0) return 0;
  return distance (0, vectors.size()-1);
}
/**
 * Calculate the shortest distance between two points.
 * This is ALWAYS less or equal to length()
 * @param p0 start point
 * @param p1 end point
 * Points can not be reversed.
 */
unsigned int
SLineCurve::distance (unsigned int p0, unsigned int p1) const
{
  const SLocation diff = vectors[p1] - vectors[p0];
  return diff.distance();
}
const SLocation&
SLineCurve::operator[] (unsigned int index) const
{
  return vectors[index];
}

/**
 * Get a vector pointing from -> to
 * @param p0 start point
 * @param p1 end point
 * Points can not be reversed.
 */
SLocation
SLineCurve::getVector (unsigned int p0, unsigned int p1) const
{
  return SLocation (vectors[p1] - vectors[p0]);
}

unsigned int
SLineCurve::size() const
{
   return vectors.size();
}

void
SLineCurve::clear()
{
  vectors.clear();
  lengths.clear();
}

/**
 * Append a new location to this line of points.
 * check consistency of lengths too, remember
 * when a vector's length is calculated it is for sure
 * less than the real length, because of the rounding
 * in ss_sqrtlong routine used by SLocation.
 */
void 
SLineCurve::append (const SLocation& l)
{
  vectors.append (l);
  /* is this the only one ? */
  if (vectors.size()==1)
  {
     lengths.append (0);
     return;
  }
  unsigned int fulllengthnow = length ();
  unsigned int diagonal = distance ();
  /* vectors are already updated */
  unsigned int len = distance (vectors.size()-2, vectors.size()-1);
  /* sanity - length can not decrease */

  if (fulllengthnow + len < diagonal)
  {
   // Notice len can only be positive or zero so 
   //   we for sure append a positive number 
    lengths.append (diagonal);
    return;
  }
  lengths.append (fulllengthnow + len);
}

