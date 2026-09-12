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
 
#ifndef SRasterizer_h
#define SRasterizer_h

#include "stoolkit/STypes.h"
#include "stoolkit/SBinVector.h"
#include "stoolkit/SMatrix.h"
#include "stoolkit/SObject.h"

#include "swindow/SColor.h"
#include "swindow/SPen.h"
#include "swindow/SImage.h"



class SRenderPathElement {

public:

    SRenderPathElement* begin;
    SRenderPathElement* next;
    SRenderPathElement* prev;
    SRenderPathElement* end;
    double x;
    double y;
    /* Create a new path element, begin pointing to first element in group */
    SRenderPathElement (SRenderPathElement* begin, double x, double y) : begin(begin),next (0),  prev(0), end(0), x(x), y(y) {
        closedPath = false;
    }
    bool isClosedPath () const {
        return begin->closedPath;
    }
    // Get the next element a backward loop
    const SRenderPathElement* nextForward () const {
        const SRenderPathElement* ret = next;
        if (ret == 0 || begin != ret->begin) {  // loop back
            ret = begin;
        }
        return ret;
    }
    // Get the next element a backward loop
    const SRenderPathElement* nextBackward () const {
        const SRenderPathElement* ret = prev;
        if (ret == 0 || begin != ret->begin) {  // loop back
            ret = begin->end;
        }
        return ret;
    }
    // Get the next element in the a forward-backward loop
    const SRenderPathElement* nextShuttle (bool* forward) const {
        const SRenderPathElement* ret = (*forward) 
            ? nextForward ()  
            : nextBackward ();
        if (*forward) {
            if (ret == begin) {
                *forward = false;
                ret = nextBackward ();
            }
        } else {
            if (ret == begin->end) {
                *forward = true;
                ret = nextForward ();
            }
        }
        return ret;
    }
    bool closedPath;
};

class BounddingBox {
public:
    double        minX;
    double        minY;
    double        maxX;
    double        maxY;
};

class SRenderPathList  {
    public:
        SRenderPathList() : elementCount(0), head(0), current(0), begin(0) {}
        ~SRenderPathList() { clear(); }

        void append (double x, double y, bool isbegin=false);
        void clear ();
        unsigned int size() const { return elementCount; }
        void closepath () {
            if (begin) begin->closedPath = true;
            begin = 0;
        }
        bool hasBegan () {
            return (begin != 0);
        }
        const SRenderPathElement* getCurrent () const {
            return current;
        }
        BounddingBox bbox;

        const SRenderPathElement* getHead() const {
            return head;
        }
        const SRenderPathElement* getTail() const {
            return tail;
        }
    private:
        unsigned int elementCount;
        SRenderPathElement* head;
        SRenderPathElement* tail;
        SRenderPathElement* current;
        SRenderPathElement* begin;
};

// Split a Bezier  
class BezierSplitter {
public:
    virtual ~BezierSplitter() {}
    virtual void splitBezier (SRenderPathList* out, double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3, int iteration) = 0;
};

class LineJoiner {
    public:
    enum SJoinStyle { BEVEL, ROUND, MITER };
        LineJoiner (BezierSplitter* splitter, SRenderPathList* out, double lineWidth, SJoinStyle joinStyle=BEVEL) : out(out), splitter(splitter), lineWidth(lineWidth), joinStyle(joinStyle) {}

        void joinLoop (double x0, double y0, double x1, double y1, double x2, double y2, bool isBegin);
        // joinStyle is not used, this is to suppress compiler warning
        SJoinStyle getJoinStyle() {
            return joinStyle;
        }
        // jsplitter is not used, this is to suppress compiler warning
        BezierSplitter* getSplitter() {
            return splitter;
        }
    private:
        SRenderPathList* out; 
        BezierSplitter*  splitter;
        double lineWidth;
        SJoinStyle joinStyle;
};

/**
 * This is a completely re-worked version of SGEngine,
 * that was written in year 2000 for the first version of Yudit with 
 * bezier interface.
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2023-01-22
 */
class SRasterizer : public BezierSplitter {
public:
    enum SWindingRule { NONZERO, EVENODD };

    SRasterizer (unsigned int overSample=SS_RASTERIZER_OVERSAMPLE, SWindingRule windingRule=NONZERO) 
    : overSample(overSample),  windingRule (windingRule) {
        SS_Matrix2D m;
        matrix.append (m);
    } 
    virtual ~SRasterizer () {
        clearImages();
    }; 

    void setOverSample (unsigned int arg_overSample) {
        overSample = arg_overSample;
    }
    void setWindingRule (SWindingRule arg_windingRule) {
        windingRule = arg_windingRule;
    }

    bool beginImage (int x, int y, const SString& id);
    void newpath ();
    SImage* endImage ();

    void moveto (double x, double y);
    void lineto (double x, double y);
    void curveto (double x1, double y1, double x2, double y2, double x3, double y3);
    void closepath();

    // our SRasterizer only supports stroke paths with miter
    void stroke (int x, int y, unsigned int maxWidth, unsigned int maxHeight, const SPen& pen);
    void fill (int x, int y, unsigned int maxWidth, unsigned int maxHeight, const SPen& pen);
    void scale (double x, double y) {
        SS_Matrix2D m = matrix[matrix.size()-1];
        m.scale (x, y);
        popmatrix ();
        matrix.append (m);
    }
    void translate (double x, double y) {
        SS_Matrix2D m = matrix[matrix.size()-1];
        m.translate (x, y);
        popmatrix ();
        matrix.append (m);
    }
    void rotate (double angle) {
        SS_Matrix2D m = matrix[matrix.size()-1];
        m.rotate (angle);
        popmatrix ();
        matrix.append (m);
    }
    /*---------------- end canvas --------------*/
    static void  setCacheSize(unsigned int size);
    static void  setCacheOn (bool isOn=true);

    /* we limit matrix size to ten */
    void pushmatrix() {
        if (matrix.size() <= 10) {
            SS_Matrix2D m = matrix[matrix.size()-1];
            matrix.append (m);
        } else {
            fprintf (stderr, "pushpmatrix size: %u\n", matrix.size());
        }
    }
    void popmatrix() {
        if (matrix.size() > 1) {
            matrix.truncate(matrix.size()-1);
        }
    }
    SS_Matrix2D  getCurrentMatrix() const {
        return SS_Matrix2D(matrix[matrix.size()-1]);
    }
    const SRenderPathElement* getCurrent () const {
        return renderPath.getCurrent();
    }
private:
    void updateBBox();
    void clearImages() {
        for (unsigned i=0; i<images.size(); i++) {
            SImage* im = images[i];
            if (im) delete im;
        }
        images.clear();
    }
    virtual void splitBezier (SRenderPathList* out, double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3, int iteration);

    SImage* fillInternal (SWindingRule rule, const SRenderPathList& path, unsigned int maxWidth, unsigned int maxHeight);

    SImage* strokeInternal (double lineWidth, unsigned int maxWidth, unsigned int maxHeight);

    unsigned int overSample;
    SWindingRule windingRule;
    SS_MatrixStack matrix;

    SBinVector<SImage*> images;
    SRenderPathList renderPath;

    SString newImageID;
    double newImageX;
    double newImageY;
};

#endif /* SRasterizer_h */
