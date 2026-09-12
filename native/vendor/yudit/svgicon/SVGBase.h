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
 
#ifndef SVGBase_h
#define SVGBase_h

#include <swindow/SCanvasBase.h>
#include <swindow/SColor.h>
#include <swindow/SPen.h>
#include <stoolkit/SString.h>
#include <stoolkit/SBinVector.h>

/*
*  A VectorGraphic Icon Base 
*/
class SVGBase {
public:
    SVGBase (const SString& aId) 
        : pen(SColor(1.0, 1.0, 1.0, 1.0), SColor(0.0, 0.0, 0.0, 0.0)) {
        id = aId;
        scale = 1.0;
    }
    inline virtual ~SVGBase () {}
    inline void setCanvas (SCanvasBase* aCanvas, const SColor& background) {
        canvas = aCanvas;
        pen = SPen(pen.getForeground(), background); 
        //canvas->setBackground(background);
        key = id;
/*
        const SColor& cb = pen.getBackground();
        key.append ((char) 'b');
        key.append ((char) ':');
        key.append ((char) cb.red);
        key.append ((char) cb.green);
        key.append ((char) cb.blue);
*/
    }
    virtual double getWidth()=0;
    virtual double getHeight()=0;
    
    void render (double x, double y, double aScale=1.0) {
        sx = x;
        sy = y;
        scale = aScale;

        SString key1 = key;
        key1.append ("scale:");
        key1.print (scale);

        cached = canvas->beginImage (sx, sy, key1, pen.getBackground());
        renderInternal();
        canvas->endImage ();
        cached = 0;
    }
    void newpath (int aSegment) {
        segment = aSegment;
        if (cached) {
            return;
        }
        canvas->newpath (); 
    }
    void rect (int aSegment, double x, double y, 
        double width, double height, const char* fillColor, 
        const char* strokeColor, double lineWidth) {
        segment = aSegment;
        if (cached) {
            return;
        }

//fprintf (stderr, "rect %g,%g, %g,%g\n", x, y, width, height);
//x = 1; y=1; width = 2; height = 2;
        canvas->newpath ();
        moveto(x,y);
        lineto(x+width, y);
        lineto(x+width, y+height);
        lineto(x, y+height);
        closepath();
        if (fillColor) {
            color (fillColor);
            fill();
        }
        if (strokeColor) {
            color (strokeColor);
            stroke(lineWidth);
        }
    }
    void moveto (double x, double y) {
        if (!cached) {
            canvas->moveto(x*scale + sx, y*scale + sy);
        }
    } 
    void lineto(double x, double y) {
        if (!cached) {
            canvas->lineto(x*scale + sx, y*scale + sy);
        }
    }
    void curveto(double x0, double y0, double x1, double y1, double x, double y) {
        if (!cached) {
            canvas->curveto(x0 * scale + sx, y0*scale + sy, 
                x1 * scale + sx, y1 * scale + sy,
                x * scale + sx, y * scale + sy);
        }
    }
    void closepath() {
        if (!cached) {
            canvas->closepath();  
        }
    }
    void color (const char* aColor) {
        SColor color(aColor);
        pen.setForeground(color);
    }
    void stroke(double width) {
        if (!cached) {
            SPen spen (pen);
            spen.setLineWidth(width * scale);
            canvas->stroke(spen);
        }
    }
    void fill() {
        if (!cached) {
            canvas->fill(pen);
        }   
    }  
protected:

    virtual void renderInternal()=0;

private:
    SCanvasBase *canvas;
    SPen    pen;
    SString id;
    SString key;
    bool    cached;
    int     segment;
    double  scale;
    double  sx;
    double  sy;
};

#endif /* SVGBase_h */
