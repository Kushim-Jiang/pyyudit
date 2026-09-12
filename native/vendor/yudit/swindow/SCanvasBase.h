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
 
#ifndef SCanvasBase_h
#define SCanvasBase_h

#include "swindow/SColor.h"
#include "swindow/SPen.h"

class SCanvasBase {
public:
    virtual bool beginImage (double x, double y, 
        const SString& id, const SColor& background)=0;
    virtual void newpath ()=0;
    virtual void moveto (double x, double y)=0;
    virtual void lineto (double x, double y)=0;
    virtual void curveto (double x0, double y0, double x1,
          double y1, double x3, double y3)=0;
    virtual void fill (const SPen& pen)=0;
    virtual void stroke (const SPen& pen)=0;
    virtual void closepath()=0;
    virtual void endImage()=0;
};

#endif /* SCanvasBase_h */
