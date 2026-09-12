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
 
#ifndef SRedrawEvent_h
#define SRedrawEvent_h

#include "stoolkit/SObject.h"


class SRedrawEvent : public SObject
{
public:
  SRedrawEvent (bool clear, int x, int y, 
       unsigned int width, unsigned int height);
  SRedrawEvent (const SRedrawEvent& evt);
  SRedrawEvent operator=(const SRedrawEvent& evt);
  virtual ~SRedrawEvent();
  virtual SObject* clone () const;
  bool merge (const SRedrawEvent& evt);
  bool clear;
  int x;
  int y;
  unsigned int width;
  unsigned int height;
};


#endif /* SRedrawEvent_h */
