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
 
#ifndef SHWConverter_h
#define SHWConverter_h

#include "stoolkit/SLocation.h"
#include "stoolkit/SStringVector.h"
#include "stoolkit/SLineCurve.h"

/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2001-10-02
 */

/*
 * Handwriting conversion module for Yudit.
 */
class SHWConverter
{
public:
  SHWConverter(void);
  SHWConverter(const SString& name);
  SHWConverter(const SHWConverter& c);
  ~SHWConverter();
  SHWConverter operator= (const SHWConverter& c);
  bool isOK () const;
  SStringVector convert(const SLineCurves& strokes, bool directed=false) const;
  static void setDebugLevel (int level);
  const SString& getName() const;
protected:
  void* shared;
  SString name;
};

#endif /* SHWConverter_h */
