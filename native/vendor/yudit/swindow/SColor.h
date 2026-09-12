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
 
#ifndef SColor_h
#define SColor_h

#include "stoolkit/STypes.h"
#include "stoolkit/SString.h"
#include "stoolkit/SObject.h"
#include "stoolkit/SVector.h"

/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 * This is the abstract widget toolkit font package
 */

class SColor : public SObject
{
public:
  SColor (void);
  SColor (SS_WORD32 value);
  SColor (const SString& name);
  SColor (const SString& name, double alpha);
  SColor (double red, double green, double blue, double alpha);
  SColor (unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha);

  SColor (const SColor& color);

  SColor operator= (const SColor& color);
  SColor lighter(double a=0.3) const;
  SColor darker(double a=0.3) const;
  virtual ~SColor();
  virtual void blend (const SColor& color);
  virtual SObject* clone () const;
  bool isDark ();
  SS_WORD32 getValue() const;
  static SS_WORD32 getNamedColor(const SString& name);
  bool operator==(const SColor& col) const;
  bool operator!=(const SColor& col) const;
  unsigned char red;
  unsigned char green;
  unsigned char blue;
  unsigned char alpha;
};

typedef SVector<SColor> SColorVector;

#endif /* SColor_h */
