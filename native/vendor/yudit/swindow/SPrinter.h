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
 
#ifndef SPrinter_h
#define SPrinter_h

#include "swindow/SCanvas.h"
#include "swindow/SPrinter.h"

#include "stoolkit/SExcept.h"
#include "stoolkit/SString.h"
#include "stoolkit/SProperties.h"
#include "stoolkit/SIOStream.h"
#include "stoolkit/SMatrix.h"

/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-07-06
 * This is a postscript renderer for yudit.
 */
class SPrinter : public SCanvas
{
public:
  enum SMedia { A3, A4, A5, B4, B5, Executive, Folio, Ledger,
                Legal, Letter, Quarto, Statement, Tabloid  };
  enum SOrientation { PORTRAIT, LANDSCAPE };
  enum SType { POSTSCRIPT, PCL, WIN32 };

  SPrinter (const SWriter& writer, 
     SType=POSTSCRIPT, SMedia m=A4, SOrientation o=PORTRAIT);
  SPrinter (const SPrinter& printer);
  virtual ~SPrinter();

  unsigned int getWidth() const;
  unsigned int getHeight() const;
  int getX() const;
  int getY() const;
  SString getCreationDate() const;

  bool open (bool background=false);
  void newPage();
  bool cacheOn (bool on=true); 
  bool close ();
  bool hasNative() const;

  /* From SCanvas */
  virtual bool beginImage (double x, double y, const SString& id, const SColor& background);
  virtual void newpath ();
  virtual void endImage ();
  virtual void fill (const SPen& pen);
  virtual void stroke (const SPen& pen);
  virtual void fill ();
  virtual void stroke ();

  virtual void moveto (double x, double y);
  virtual void lineto (double x, double y);
  virtual void curveto (double x0, double y0, double x1, 
          double y1, double x3, double y3);
  virtual void closepath();
  virtual void pushmatrix();
  virtual void popmatrix();
  virtual void scale (double x, double y);
  virtual void translate (double x, double y);
  virtual void rotate (double angle);
  virtual void bitfont (const SPen& pen, double x, double y, 
       void* native, char* data, unsigned int len);

  virtual void bitfill (const SColor& bg, int x, int y, 
          unsigned int width, unsigned int height);
  virtual void bitline (const SColor& fg, int x, int y, int tox, int toy);
  virtual void bitpoint (const SColor& fg, int x, int y);
  virtual void bitpoints (const SColor& fg, const int* x, const int* y, 
         unsigned int size);

  virtual SS_Matrix2D  getCurrentMatrix() const;
  virtual void putImage (int x, int y, const SImage& image);
  virtual void setBackground(const SColor &color);

private:
  SType    type;
  SCanvas* delegate;
};

#endif /* SPrinter_h */
