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
 
#ifndef SAccelerator_h
#define SAccelerator_h

#include "stoolkit/SString.h"
#include "stoolkit/SObject.h"

/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 * This is the abstract widget toolkit font package
 */
class SAccelerator : public SObject
{
public:
  SAccelerator (void);
  SAccelerator (const SString& str);
  SAccelerator (int key, bool ctrl=true, bool shift=false, bool meta=false);
  virtual ~SAccelerator ();
  SAccelerator(const SAccelerator& a);
  SAccelerator operator = (const SAccelerator& a);
  const SString& toString() const;
  SObject* clone() const;
  SString string;
  int  key;
  bool ctrl;
  bool shift;
  bool meta;
};

class SAcceleratorListener
{
public:
  SAcceleratorListener();
  virtual ~SAcceleratorListener();
  virtual void acceleratorPressed (const SAccelerator& a)=0;
  virtual void acceleratorReleased (const SAccelerator& a)=0;
};

#endif /* SAccelerator_h */
