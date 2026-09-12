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

#include "SMatrix.h"
 
/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 */
#include <stdio.h>

/**
 * create a 2 d matrix 
 */
SS_Matrix2D::SS_Matrix2D(void)
{
  x0 = 1.0; y0 = 0.0; t0=0.0;
  x1 = 0.0; y1 = 1.0; t1=0.0;
  x2 = 0.0; y2 = 0.0; t2=1.0;        
}

SS_Matrix2D::SS_Matrix2D (const SS_Matrix2D& m)
{
  x0 = m.x0; y0 = m.y0; t0=m.t0;
  x1 = m.x1; y1 = m.y1; t1=m.t1;
  x2 = m.x2; y2 = m.y2; t2=m.t2;        
}

SS_Matrix2D
SS_Matrix2D::operator= (const SS_Matrix2D& m)
{
  x0 = m.x0; y0 = m.y0; t0=m.t0;
  x1 = m.x1; y1 = m.y1; t1=m.t1;
  x2 = m.x2; y2 = m.y2; t2=m.t2;        
  return *this;
}

bool
operator== (const SS_Matrix2D& m1, const SS_Matrix2D& m2)
{
  return (m1.x0 == m2.x0 &&  m1.y0 == m2.y0 && m1.t0==m2.t0
   && m1.x1 == m2.x1 &&  m1.y1 == m2.y1 && m1.t1==m2.t1
   && m1.x2 == m2.x2 && m1.y2 == m2.y2 &&  m1.t2==m2.t2);        
}
bool
operator< (const SS_Matrix2D& m1, const SS_Matrix2D& m2)
{
  return (m1.x0 < m2.x0 &&  m1.y0 < m2.y0 && m1.t0<m2.t0
   && m1.x1 < m2.x1 &&  m1.y1 < m2.y1 && m1.t1<m2.t1
   && m1.x2 < m2.x2 && m1.y2 < m2.y2 &&  m1.t2<m2.t2);        
}
bool
operator> (const SS_Matrix2D& m1, const SS_Matrix2D& m2)
{
  return (m1.x0 > m2.x0 &&  m1.y0 > m2.y0 && m1.t0>m2.t0
   && m1.x1 > m2.x1 &&  m1.y1 > m2.y1 && m1.t1>m2.t1
   && m1.x2 > m2.x2 && m1.y2 > m2.y2 &&  m1.t2>m2.t2);        
}

SS_Matrix2D
SS_Matrix2D::operator* (const SS_Matrix2D& m1) const
{
  SS_Matrix2D r;

  r.x0 = x0 * m1.x0 + y0 * m1.x1 + t0 * m1.x2;
  r.x1 = x1 * m1.x0 + y1 * m1.x1 + t1 * m1.x2;
  r.x2 = x2 * m1.x0 + y2 * m1.x1 + t2 * m1.x2;

  r.y0 = x0 * m1.y0 + y0 * m1.y1 + t0 * m1.y2;
  r.y1 = x1 * m1.y0 + y1 * m1.y1 + t1 * m1.y2;
  r.y2 = x2 * m1.y0 + y2 * m1.y1 + t2 * m1.y2;

  r.t0 = x0 * m1.t0 + y0 * m1.t1 + t0 * m1.t2;
  r.t1 = x1 * m1.t0 + y1 * m1.t1 + t1 * m1.t2;
  r.t2 = x2 * m1.t0 + y2 * m1.t1 + t2 * m1.t2;
  return SS_Matrix2D (r);
}

SS_Matrix2D::~SS_Matrix2D()
{
}
/**
 * TODO: not implemented 
 */
void
SS_Matrix2D::rotate (double angle)
{
}

void
SS_Matrix2D::scale (double x, double y)
{
  x0 = x0 * x; 
  y1 = y1 * y;
}

void
SS_Matrix2D::translate (double x, double y)
{
  t0 = t0 + x;
  t1 = t1 + y;
}
SS_Matrix2D
SS_Matrix2D::invert() const
{
  SS_Matrix2D r;

  /* FIXME: my det has always some problem. */
  return SS_Matrix2D(r);
}
double
SS_Matrix2D::toX(double _x, double _y) const
{
  return x0 * _x + y0 * _y + t0;
}

double
SS_Matrix2D::toY(double _x, double _y) const
{
  return x1 * _x + y1 * _y + t1;
}
