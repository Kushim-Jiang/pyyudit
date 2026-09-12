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
 
#include "swindow/SImage.h"
#include "stoolkit/SExcept.h"
#include <stdlib.h>

class SImageBuffer
{
public:
  SImageBuffer (unsigned int shades, unsigned int width, unsigned int height);
  SImageBuffer (SS_WORD32* image, unsigned int shades, unsigned int width, unsigned int height);

  inline SS_WORD32 getShade(const int x, const int y) const;
  void compress();
  ~SImageBuffer();
  const SUniqueID& getID() const;
  const unsigned char* getRGBA8Buffer() const;

  unsigned int count;

  SS_WORD32*   image;
  unsigned int width;
  unsigned int height;
  unsigned int shades;

  SUniqueID    id;
  bool compressed;
};

//PROFILE - made inline, better prototype
SS_WORD32
SImageBuffer::getShade(const int x, const int y) const
{
  if (x < 0 || x >= (int)width) return 0;
  if (y < 0 || y >= (int)height) return 0;
  if (compressed)
  {
    return (SS_WORD32) (((unsigned char*)image)[y * width + x]);
  }
  return image[y * width + x];
}

/**
 * return the unique id of this buffer
 */
const SUniqueID&
SImageBuffer::getID() const
{
  return id;
}

SImageBuffer::SImageBuffer (unsigned int s, 
       unsigned int w, unsigned int h)
{
  shades = s;
  width = w;
  height = h;
  count = 1;
  image = new SS_WORD32 [width*height];
  CHECK_NEW (image);
  compressed = false;
}

SImageBuffer::SImageBuffer (SS_WORD32* i, unsigned int s, 
       unsigned int w, unsigned int h)
{
  shades = s;
  width = w;
  height = h;
  count = 1;
  image = i;
  compressed = false;
}

SImageBuffer::~SImageBuffer()
{
  if (image) delete (unsigned char*)image;
}

void
SImageBuffer::compress ()
{
  if (image == 0 || compressed || shades == 0 || shades > 255)
  {
     return;
  }
  unsigned char* ib = new unsigned char [width*height];
  CHECK_NEW (ib);
  for (unsigned int i=0; i<width*height; i++)
  {
     ib[i] = image[i];
  }
  delete image;
  image = (SS_WORD32*) ib;
  compressed = true;
}

/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 * This is the abstract widget toolkit font package
 */

SImage::SImage (unsigned int _colors, unsigned int _width, unsigned int _height)
{
  buffer = new SImageBuffer (_colors, _width, _height);
  CHECK_NEW (buffer);
  origox = 0;
  origoy = 0;
  px = 0; py=0;
  offScreen = false;
  moved = false;
  svgBase = 0;
}

SImage::SImage (SS_WORD32* _im, unsigned int _shades, int _origox, int _origoy,
  unsigned int _width, unsigned int _height)
{
  buffer = new SImageBuffer (_im, _shades, _width, _height);
  CHECK_NEW (buffer);
  origox = _origox;
  origoy = _origoy;
  px = 0; py=0;
  offScreen = false;
  moved = false;
  svgBase = 0;
}

void
SImage::setOrigoX (int x)
{
  origox = x;
  moved = true;
}

void
SImage::setOrigoY (int y)
{
  origoy = y;
  moved = true;
}

/**
 * This is for the Object class
 */
SObject*
SImage::clone () const
{
  SImage* ret =  new SImage(*this);
  CHECK_NEW (ret);
  return ret;
}

SImage::SImage(const SImage& im) 
{
  buffer = ((SImage*) &im)->buffer;
  ((SImageBuffer*)buffer)->count++;
  px = im.px; py=im.py;
  offScreen = im.offScreen;
  origox = im.origox; origoy = im.origoy;
  svgBase = im.svgBase;
  moved = im.moved;
}

/**
 * This is a shallow copy 
 */
SImage
SImage::operator=(const SImage& im)
{
  if (&im == this) return *this;
  if (buffer)
  {
     SImageBuffer* b = (SImageBuffer*) buffer;
     b->count--;
     if (b->count == 0) delete b;
  }
  buffer = ((SImage*) &im)->buffer;
  ((SImageBuffer*)buffer)->count++;
  px = im.px; py=im.py;
  offScreen = im.offScreen;
  origox = im.origox; 
  origoy = im.origoy;
  moved = im.moved;
  svgBase = im.svgBase;
  return *this;
}

SImage::~SImage()
{
  if (buffer)
  {
     SImageBuffer* b = (SImageBuffer*) buffer;
     b->count--;
     if (b->count == 0) delete b;
  }
}

int
SImage::getOrigoX() const
{
  return origox;
}

int
SImage::getOrigoY() const
{
  return origoy;
}

/**
 * Return the number of shades we have.
 * If it is zero, we have a truecolor image, otherwise 
 * the image juste have 'shades'.
 * both truecolor and shaded images get the value with getSHade()
 */
unsigned int
SImage::getShades () const
{
  return ((SImageBuffer*)buffer)->shades;
}

unsigned int
SImage::getWidth () const
{
  return ((SImageBuffer*)buffer)->width;
}

unsigned int 
SImage::getHeight () const
{
  return ((SImageBuffer*)buffer)->height;
}

/**
 * return the shade value.
 * This value is between 1.0..0 1.0 being the brightest.
 * @param x is the x coordinate
 * @param y is the y coordinate 
 */
SS_WORD32
SImage::getShade(int x, int y) const
{
  return ((SImageBuffer*)buffer)->getShade(x, y);
}
/**
 * Create an image from an xpm array
 * This is a very stoolkit version. Don't expect much.
 */
void
SImage::compress ()
{
  ((SImageBuffer*)buffer)->compress();
}

/**
 * Create an image from an xpm array
 * This is a very stoolkit version. Don't expect much.
 */
SImage::SImage (const char* const* xpm, SVGBase* aSvgBase)
{
  svgBase = aSvgBase;
  int w;
  int h;
  int clrs;
  int colperp;
  if (sscanf (xpm[0], "%d %d %d %d", &w, &h, &clrs, &colperp)  != 4)
  {
    fprintf (stderr, "Error in xpm.\n");
    return;
  }
  if (clrs > 64 && colperp > 1)
  {
    fprintf (stderr, "Too many colors in xpm.\n");
    return;
  }
  unsigned char *p = new unsigned char[256];
  CHECK_NEW (p);

  memset (p, 0, clrs);
  char shade[32];
  char col[32];
  unsigned char ui;
  SColorVector list;
  char* next;
  SS_WORD32 ret;

  unsigned int i;
  for (i=0; i<(unsigned int)clrs; i++)
  {
    ui = (unsigned char) xpm[1+i][0];
    p[ui] = i;
    //fprintf (stderr, "%u\n", (unsigned int)ui); 
    /* TODO: create colors in list */
    if (sscanf (&xpm[i+1][2], "s s_%s c %s", shade, col) == 2 
  || sscanf (&xpm[i+1][2], "c %s s s_%s", col, shade) == 2)
    {
       ret = (SS_WORD32)  strtoul (shade,  &next, 16);
       list.append (SColor(col, (double)ret/255));
//fprintf (stderr, "col:%s s:%s = %lx ret=%lx\n", col, shade,
//  (unsigned long) list[i].getValue(), (unsigned long)ret); 
    }
    else if (sscanf (&xpm[i+1][2], "c %s", col) == 1)
    {
//fprintf (stderr, "col %s\n", col); 
      if (SString(col) == SString("None")
        || SString(col) == SString("none"))
      {
        list.append(SColor(0.0, 0.0, 0.0, 0.0));
//fprintf (stderr, "XXXX %s = %lx (%lx)\n", col, 
//  (unsigned long) list[i].getValue(), 
//  (unsigned long) SColor(0.0,0.0,0.0,0.0).getValue());
      }
      else
      {
        list.append(SColor(col));
      }
//fprintf (stderr, "col%s = %lx (%lx)", col, 
//  (unsigned long) list[i].getValue(), 
//  (unsigned long) SColor(col, 1.0).getValue());
    }
    else if (sscanf (&xpm[i+1][2], "s s_%s", shade) == 1)
    {
       ret = (SS_WORD32)  strtoul (&shade[1],  &next, 16);
       list.append(SColor(
          (unsigned char) 0, (unsigned char) 0, 
          (unsigned char) 0, (unsigned char) ret));
    }
    else
    {
       fprintf (stderr, "Error in xpm line %d.\n", 1+i);
       delete[] p;
       return;
    }
  }

  SImageBuffer* b = new SImageBuffer (0, w, h);
  buffer = b;
  CHECK_NEW (buffer);
  origox = 0;
  origoy = 0;
  px = 0; py=0;
  offScreen = false;

  unsigned int j;
  for (i=0; i<(unsigned int)h; i++)
  {
    for (j=0; j<(unsigned int)w; j++)
    {
      ui = p[(unsigned char) xpm[1+clrs+i][j]];
      b->image[i * w + j] = list[ui].getValue();
    //fprintf (stderr, "%u\n", (unsigned int)ui); 
    /* TODO: create colors in list */
    //fprintf (stderr, "%lx ", (unsigned long) list[ui].getValue());
    }
  }
  delete[] p;
}

/**
 * Return a unique id that is the same for images that
 * share the same buffer.
 */
const SUniqueID&
SImage::getID() const
{
  return  ((SImageBuffer*)buffer)->getID();
}

const unsigned char*
SImageBuffer::getRGBA8Buffer() const {
  // FIMXE: Endianness
  return (const unsigned char*) image;
}

const unsigned char*
SImage::getRGBA8Buffer() const {
  return ((SImageBuffer*)buffer)->getRGBA8Buffer();
}

SImage
SImage::addLayer (const SImage& layer) {
    if (moved || layer.moved) 
    {
        fprintf (stderr, "Can nto add a layer that has moved.\n");
        return SImage(*this);
    }
    // TODO, check largest size and merge with layer.
    if (layer.px != px) {
        fprintf (stderr, "px different\n");
        return SImage(*this);
    }
    if (layer.py != py) {
        fprintf (stderr, "py different\n");
        return SImage(*this);
    }

    int minX = (origox < layer.origox) ? origox : layer.origox;
    int minY = (origoy < layer.origoy) ? origoy : layer.origoy;

    int maxX =  (origox + (int) getWidth() > layer.origox + (int) layer.getWidth()) 
        ? origox +  (int) getWidth() : layer.origox + (int) layer.getWidth();

    int maxY =  (origoy + (int) getHeight() > layer.origoy + (int) layer.getHeight()) 
        ? origoy +  (int) getHeight() : layer.origoy + (int) layer.getHeight();

    unsigned int resultWidth = maxX-minX;
    unsigned int resultHeight = maxY-minY;

    SS_WORD32* buffer = new SS_WORD32[resultWidth * resultHeight];
    memset (buffer, 0, resultWidth * resultHeight * sizeof (SS_WORD32));
    CHECK_NEW (buffer);

    for (int x=0; x<(int)resultWidth; x++) 
    {
        for (int y=0; y<(int)resultHeight; y++) 
        {
            // There is range checking in getShade
            SS_WORD32 tsh = getShade (x+minX-origox, y+minY-origoy);
            SS_WORD32 lsh = layer.getShade (x+minX-layer.origox, y+minY-layer.origoy);
            SColor tc (tsh);
            SColor lc (lsh);
            tc.blend(lc);
            buffer[y * resultWidth + x ] = tc.getValue();
        }
    }
    SImage ret (buffer, 0, minX, minY, resultWidth, resultHeight);
    ret.px = px;
    ret.py = py;
    ret.offScreen |= offScreen;
    ret.offScreen |= layer.offScreen;
    return SImage(ret);
}

SImage
SImage::colorize (const SColor& color) {
    int shades = getShades();
    if (shades <= 0) return SImage(*this);
    if (getHeight() == 0) return SImage(*this);
    if (getWidth() == 0) return SImage(*this);
    SS_WORD32* buffer = new SS_WORD32[getHeight() * getWidth()];
    CHECK_NEW (buffer);
    for (unsigned int y=0; y<getHeight(); y++)
    {
        for (unsigned int x=0; x<getWidth(); x++)
        {
            SS_WORD32 sh = getShade (x, y);
            unsigned int ourAlpha = (255 * sh)/(shades-1);
            ourAlpha = (color.alpha * ourAlpha) / 255;

            SS_WORD32 pixel = 0;
            pixel |= (unsigned int) ourAlpha << 24;
            pixel |= (unsigned int) color.red << 16;
            pixel |= (unsigned int) color.green << 8;
            pixel |= (unsigned int) color.blue ;
            buffer[y * getWidth() + x ] = pixel;
        }
    }
    SImage ret (buffer, 0, getOrigoX(), getOrigoY(), 
        getWidth(), getHeight());
    ret.px = px;
    ret.py = py;
// SGC
    ret.offScreen |= offScreen;
    return SImage (ret);
}
