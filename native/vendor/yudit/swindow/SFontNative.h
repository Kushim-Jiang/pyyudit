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
 
#ifndef SFontNative_h
#define SFontNative_h

#include "swindow/SCanvas.h"
#include "swindow/SPen.h"
#include "stoolkit/STypes.h"
#include "stoolkit/SString.h"
#include "stoolkit/SMatrix.h"
#include "stoolkit/SObject.h"
#include "stoolkit/SUniMap.h"


/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 * This is a specific native font, with a certain size
 */
class SFontNative
{
public:
  SFontNative (void); 
  virtual ~SFontNative (); 

  virtual bool draw (const SString& xlfd, 
    SCanvas* canvas, const SPen& pen, const SS_Matrix2D& matrix, SS_UCS4 g);
  virtual bool width (const SString& xlfd, SS_UCS4 g, double *width_);

  virtual double width (const SString& xlfd);
  virtual double ascent (const SString& xlfd);
  virtual double descent (const SString& xlfd);
  virtual double gap (const SString& xlfd);
};

#endif /* SFontNative_h */
