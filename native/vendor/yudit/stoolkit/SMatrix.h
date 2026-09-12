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
 
#ifndef SMatrix_h
#define SMatrix_h

#include "stoolkit/SBinVector.h"
/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 */

/*
 * Matrices and the like
 */
class SS_Matrix2D
{
public:
  SS_Matrix2D(void);
  SS_Matrix2D(const SS_Matrix2D& m);
  ~SS_Matrix2D();
  SS_Matrix2D operator= (const SS_Matrix2D& m);
  friend bool operator== (const SS_Matrix2D& m1, const SS_Matrix2D& m2);
  friend bool operator< (const SS_Matrix2D& m1, const SS_Matrix2D& m2);
  friend bool operator> (const SS_Matrix2D& m1, const SS_Matrix2D& m2);
  SS_Matrix2D operator* (const SS_Matrix2D& m1) const;
  void rotate (double angle);
  void translate (double x, double y);
  SS_Matrix2D invert() const;

  double toX(double x, double y) const;
  double toY(double x, double y) const;

  void scale (double x, double y);
  double x0; double y0; double t0;
  double x1; double y1; double t1;
  double x2; double y2; double t2;
};

typedef SBinVector<SS_Matrix2D> SS_MatrixStack;

#endif /* SMatrix_h */
