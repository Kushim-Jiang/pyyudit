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
 
/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 */
#include "SObject.h"
#include "SExcept.h"
#include <stdio.h>


#if SS_DEBUG_MEMORY_LEAK
int SObject::debugLevel = 0;
int SObject::created =0;

SObject::~SObject()
{
  created--;
  if (debugLevel && created==0)
  {
     fprintf (stderr, "Debug OK: SObject::~SObject no memory leak.\n");
  }
}

int
SObject::debug (int level)
{
  int old = debugLevel;
  debugLevel = level;
  return old;
}

#endif
