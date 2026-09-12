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
 
#ifndef SCanvas_h
#define SCanvas_h

#include "swindow/SPen.h"
#include "swindow/SImage.h"
#include "stoolkit/SMatrix.h"
#include "stoolkit/STypes.h"
#include "stoolkit/SUniqueID.h"
#include "stoolkit/SString.h"

/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 * This is the abstract widget toolkit package
 */
class SCanvas : public SCanvasBase
{
public:
  virtual ~SCanvas(){}
  virtual bool cacheOn (bool on=true)=0;
  virtual bool beginImage (double x, double y, const SString& id, const SColor& background)=0;
  virtual void endImage ()=0;
  virtual void newpath ()=0;
  virtual void fill (const SPen& pen)=0;
  virtual void stroke(const SPen& pen)=0;

  virtual void moveto (double x, double y)=0;
  virtual void lineto (double x, double y)=0;
  virtual void curveto (double x0, double y0, double x1, 
          double y1, double x3, double y3)=0;
  virtual void closepath()=0;
  virtual void pushmatrix()=0;
  virtual void popmatrix()=0;
  virtual void scale (double x, double y)=0;
  virtual void translate (double x, double y)=0;
  virtual void rotate (double angle)=0;
  virtual SS_Matrix2D  getCurrentMatrix() const=0;
  
  virtual void putImage (int x, int y, const SImage& image)=0;
  virtual void setBackground(const SColor &color)=0;

  /*
   * the following 3 methods are to be implemented for a canvas, 
   * without a pixel device
   */
  virtual void bitfont (const SPen& pen, double x, double y, void* native, char* data, unsigned int len)=0;
  virtual void bitfill (const SColor& bg, int x, int y, unsigned int width, unsigned int height)=0;
  virtual void bitline (const SColor& fg, int x, int y, int tox, int toy)=0;
  virtual void bitpoint (const SColor& fg, int x, int y)=0;
  virtual void bitpoints (const SColor& fg, const int* x, const int* y, 
         unsigned int arraySize)=0;
};

#endif /* SCanvas_h */
