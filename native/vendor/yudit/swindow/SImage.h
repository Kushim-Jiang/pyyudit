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
 
#ifndef SImage_h
#define SImage_h

#include "swindow/SColor.h"
#include "stoolkit/STypes.h"
#include "stoolkit/SObject.h"
#include "stoolkit/SUniqueID.h"
#include "svgicon/SVGBase.h"

/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 * This is the abstract widget toolkit font package
 */

class SImage : public SObject
{
public:
  SImage (const char* const* xpm, SVGBase* svgBase);
  SImage (const SImage& im);
  SImage operator = (const SImage& im);
  SImage (unsigned int shades, unsigned int width, unsigned int height);
  SImage (SS_WORD32* image, unsigned int shades, 
       int origox, int origoy, 
       unsigned int width, unsigned int height);
  virtual ~SImage();
  const SUniqueID& getID() const;
  void compress();
  /* After fill, return the shade of white  */
  unsigned int getShades () const;
  unsigned int getWidth () const;
  unsigned int getHeight () const;
  int getOrigoX() const;
  int getOrigoY() const;
  void setOrigoX (int x);
  void setOrigoY (int y);
  SS_WORD32 getShade (int x, int y) const;
  virtual SObject* clone () const;
  const unsigned char* getRGBA8Buffer() const;
  SImage colorize (const SColor& color);
  SImage addLayer (const SImage& image);

  SVGBase* getSVGBase() const {
    return svgBase;
  }

  /* USed by cache device to indicate the offset */
  int  px;
  int  py;

  /* indicated the the image is off screen */
  bool offScreen;

//  (unsingned char*)

private:
  bool     moved;
  int      origox;
  int      origoy;
  void*    buffer;
  SVGBase* svgBase;
};


#endif /* SImage_h */
