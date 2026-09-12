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
 
#ifndef SPrimitive_h
#define SPrimitive_h

#include "stoolkit/SObject.h"
#include "stoolkit/SString.h"


/**
 * This is the graphic primitives.
 * This is what happens between a SPen'ed newpath and stroke or fill.
 */
#define MAX_SG_PARAMS 6

class SGPrimitive : public SObject
{
public:
  enum SGType { NONE,
    NEWPATH, MOVETO, LINETO, CURVETO, CLOSEPATH, 
    PUSHMATRIX, POPMATRIX, SCALE, TRANSLATE, ROTATE, STROKE, FILL };
  
  SGPrimitive (void);
  SGPrimitive (const SGPrimitive& p);
  SGPrimitive operator = (const SGPrimitive& p);
  virtual ~SGPrimitive ();
  SObject* clone() const;

  SString getKey(double originx, double originy) const;
  double getOriginX (double originx);
  double getOriginY (double originy);

  /* You need to call one of these */
  void newpath ();
  void stroke (int x, int y, unsigned int width, unsigned int height);
  void fill (int x, int y, unsigned int width, unsigned int height);
  void moveto (double x, double y);
  void lineto (double x, double y);
  void curveto (double x0, double y0, 
       double x1, double y1, double x3, double y3);
  void closepath();
  void pushmatrix();
  void popmatrix();
  void scale (double x, double y);
  void translate (double x, double y);
  void rotate (double angle);

  SGType type;
  double params[MAX_SG_PARAMS];
};


#endif /* SPrimitive_h */
