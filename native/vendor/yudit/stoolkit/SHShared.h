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
 
#ifndef _SHShared_h
#define _SHShared_h

#include <string.h>
#include "SExcept.h"

/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-05-12
 */

/*
 * This is the base class of SBinVector
 */
class SHShared 
{
public:
  inline SHShared (void);
  SHShared (unsigned int size);
  SHShared (const char* copy, unsigned int size);
  SHShared (const SHShared& copy);

  inline ~SHShared ();
  inline void ensure (unsigned int more);
  static int  debug (int level);

  char*            array;
  unsigned int     count;
  unsigned int     arraySize;
  unsigned int     vectorSize;
};


/**
 * Ensure that we have enough capacity.
 * Before this, the reference count should be one!
 * int more - the elements in bytes that we need.
 */
void
SHShared::ensure(unsigned int more)
{
  if (arraySize > vectorSize + more) return;
  char* oldArray=array;
  /* This is because hashtable _uses_ vectorsize and the
     first call to this ensure MUST allocate 'more' bytes.
   */
  unsigned int newSize = (vectorSize == 0) 
        ? more : (more + arraySize) + arraySize/2; 
  if (newSize==0) newSize=1;

  array = new char[newSize];
  CHECK_NEW(array);

  // We copy the whole thing. Don't care if arraySize is different.
  /* hashtable does not use vectorSize */
  if (arraySize)
  {
    memcpy (array, oldArray, arraySize);
    delete [] oldArray;
  }
  // Hashtable has a fixed size. If resized, ever we need this line 
  //memset (&array[arraySize], 0, newSize - arraySize);
  arraySize = newSize;
}

/**
 * Create a buffer that will be referenced by all vectors.
 */
SHShared::SHShared(void)
{
  count=1; array=0; arraySize=0; vectorSize=0;
}

/**
 * delete a buffer. The count should be one here!
 */
SHShared::~SHShared()
{
  if (arraySize != 0) {
    delete [] array;
  }
}
#endif /* _SHShared_h */
