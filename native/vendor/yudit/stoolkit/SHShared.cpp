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
 
#include "SHShared.h"
#include "SExcept.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 */

static int debug_level = 0;

/**
 * Create a buffer that will be referenced by all vectors.
 * @param size is the minimum size on chars.
 */
SHShared::SHShared (unsigned int size)
{
  count=1; vectorSize=0;
  array = new char[size];
  CHECK_NEW(array);
  arraySize = size;
}

/**
 * Create a buffer that will be referenced by all vectors.
 * @param size is the minimum size on chars.
 * @param buffer - this will be copyes over one by one.
 */
SHShared::SHShared (const char* buffer, unsigned int size)
{
  count=1; array = 0;  arraySize=0; vectorSize=0;
  if (size==0)return;
  
  array = new char[size];
  CHECK_NEW(array);
  arraySize = size;
  //ensure (arraySize);
  memcpy (array, buffer, size);
}

/**
 * Create a new Object by copying an old one
 * @param a SHShared object to copy
 */
SHShared::SHShared (const SHShared& orig)
{
  count=1; array=0; arraySize=0; vectorSize=0;
  /* Copy */
  if (orig.arraySize!=0)
  {
  	ensure (orig.arraySize);
        /* This is needed this way because of Hashtable. Vectorsize in
           hashtable does not mean continuous size  */
  	memcpy (array, orig.array, orig.arraySize);
  }
  vectorSize = orig.vectorSize;
}


/**
 * Sets debug printout levels
 */

int
SHShared::debug(int level)
{
  int prev = debug_level;
  debug_level = level;
  return prev;
}
