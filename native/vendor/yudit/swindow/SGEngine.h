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

// #########################################################################
// This code is obsolete. 
// It was developed for the 2.x.x series of Yudit in year 2000
// and was replaced in 2023 (Yudit-3.0.9) with SRasterizer.
// This class is only kept here to do performance comparison tests.
// #########################################################################
 
#ifndef SGEngine_h
#define SGEngine_h

#include "swindow/SColor.h"
#include "swindow/SPen.h"
#include "swindow/SImage.h"
#include "swindow/SGPrimitive.h"

#include "stoolkit/SVector.h"
#include "stoolkit/SBinVector.h"
#include "stoolkit/STypes.h"
#include "stoolkit/SMatrix.h"
#include "stoolkit/SObject.h"


typedef SBinVector<double> SPSPath;
typedef SVector<SPSPath> SPSPathVector;

typedef SBinVector<int> SS_InterSection;
typedef SVector<SGPrimitive> SV_Primitive;

/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 * Graphics drawing engine for the poor.
 */
class SGEngine
{
public:
  /* This should be called before fill */
  SGEngine (bool windingrule=true); 
  virtual ~SGEngine (); 

  /*---------------- similar to canvas --------------*/
  virtual bool beginImage (int x, int y, const SString& id);
  virtual void newpath ();
  virtual SImage* endImage ();

  void moveto (double x, double y);
  void lineto (double x, double y);
  void curveto (double x0, double y0, double x1, double y1, 
            double x3, double y3);
  void closepath();

  /* not implemented */
  void stroke (int x, int y, unsigned int width, unsigned int height, const SPen& pen);

  /* return xy buffer and clear states */
  void fill (int x, int y, unsigned int width, unsigned int height, const SPen& pen);

  void pushmatrix();
  void popmatrix();
  void scale (double x, double y);
  void translate (double x, double y);
  void rotate (double angle);

  /*---------------- end canvas --------------*/

  static void  setCacheSize(unsigned int size);
  static void  setCacheOn (bool on=true);
  virtual SS_Matrix2D  getCurrentMatrix() const;

private:
  bool winding;
  SV_Primitive primitive;
  void _replay (double lineWidth);
  /* these are done by replay */
  void _newpath (double lineWidth);
  void _moveto (double x, double y);
  void _lineto (double x, double y);
  void _curveto (double x0, double y0, double x1, double y1, 
        double x3, double y3);
  void _closepath();
  SImage* _stroke(int x, int y, unsigned int width, unsigned int height, const SPen& pen);
  SImage* fillInternal (int x, int y, unsigned int width, unsigned int height, double lw);
  SImage* _fill (int x, int y, unsigned int width, unsigned int height);
  void _pushmatrix();
  void _popmatrix();
  void _scale (double x, double y);
  void _translate (double x, double y);
  void _rotate (double angle);

  void setup (double d);

  void strokeScan (SS_WORD32 *image,  int width, int height, 
        const double* vectors, bool join, double origoX, double origoY, 
        const SPen& pen, bool horizontal);

  void scan (SS_InterSection** inter, int ox, int oy, 
     int height, bool swap);

  void scanCrosses (SS_InterSection** inter, int ox, int oy, 
     int height, bool swap);

  void scanWinding (SS_InterSection** inter, int ox, int oy, 
     int height, bool swap);

  void render (SS_WORD32* buffer, SS_InterSection** inter,
     unsigned int width, unsigned int height, bool swap);

  void curvetoInternal (double x0, double y0, double x1,
      double y1, double x2, double y2, double x3, double y3, int count);

  void linetoInternal (double x, double y);

  double         newpathX;
  double         newpathY;
  SString        newpathID;

  SS_MatrixStack matrix;

  unsigned int  oversample;
  unsigned int  scancount;

  /* This is sqrt (1/oversample)/2 */
  double        epsylon;

  int           colors;
  double        minx;
  double        miny;
  double        maxx;
  double        maxy;
  bool          unfinished;
  double        undelta;
  double        unx;
  double        uny;
  SPSPath       pathNow;
  SPSPathVector pathVector;
  SBinVector<SImage*> images;
};

#endif /* SGEngine_h */
