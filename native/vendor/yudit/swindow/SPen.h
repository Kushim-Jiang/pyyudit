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
 
#ifndef SPen_h
#define SPen_h

#include "swindow/SColor.h"

/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 * This is the abstract widget toolkit font package
 */

class SPen
{
public:
  SPen (const SColor& fore);
  SPen (const SColor& fore, const SColor& back);
  SPen (const SColor& fore, const SColor& back, double width);
  SPen (const SPen& pen);
  SPen operator= (const SPen& pen);
  virtual ~SPen();
  bool operator==(const SPen& pen) const;
  bool operator!=(const SPen& pen) const;

  const SColor& getForeground() const;
  const SColor& getBackground() const;
  double getLineWidth() const;

  void setForeground (const SColor& fore);
  void setBackground (const SColor& back);
  void setLineWidth (double width);
private:
  SColor fore;
  SColor back;
  double width;
};


#endif /* SPen_h */
