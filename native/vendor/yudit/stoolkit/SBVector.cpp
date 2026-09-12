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
 
#include "SBVector.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>




/**
 * Set the debug level
 * @param level 0 means no debug printouts.
 * @return the previous level
 */
int
SBVector::debug (int level)
{
  return SShared::debug(level);
}

/**
 * Create a vector from a const vector. This is 
 * used when you do
 *      SBVector v = old;
 * @param v is the old
 */
SBVector::SBVector (const SBVector& v)
{
  // Assign the common buffer and  increment the reference count
  buffer = (SShared*) v.buffer;
  buffer->count++;
}

/**
 * Assign a vector from a const vector. This is 
 * used when you do
 *      SBVector v;
 *      v = old;
 * @param v is the old
 */
SBVector&
SBVector::operator=(const SBVector& v)
{
  refer (v);
  return *this;
}

/**
 * Assign a vector from a const vector by changing the reference.
 * @param v is the input vector
 */
void
SBVector::refer(const SBVector& v)
{
  // Are we the same objects
  if (&v==this) return;
  cleanup ();
  buffer = (SShared*) v.buffer;
  buffer->count++;
  return;
}


/**
 * Replace char's with len length at index
 * @param index is the reference index of len blocks
 * @param e is the pointer to the char array to be saved
 * @param len is the length of the block.
 */
void 
SBVector::replace (unsigned int index, const char* in, unsigned int len)
{
  derefer ();
  const char* e = in;
  SShared* ns =0;
  
  if (len==0) return;
  if (len > 0 && buffer->array < in &&  &buffer->array[buffer->arraySize] > in)
  {
    fprintf (stderr, "SBVector::replace copying into itself.\n");
    ns = new SShared (in, len);
    e = ns->array; 
  }
  memcpy (&buffer->array[index], e, len);
  if (ns) delete ns;
}

/**
 * replace one occurrence of buffer with another.
 */
int
SBVector::replace (const char* e, unsigned int len, const char* with, unsigned int withLen, unsigned int from, unsigned int align)
{
  derefer ();
  int i = find (e, len, from, align);
  if (i<0) return i; // Max element size
  remove (i, len);
  insert (i, with, withLen);
  return i+withLen;
}

/**
 * replace one occurrence of buffer with another.
 */
int
SBVector::replaceAll (const char* e, unsigned int len, const char* with, unsigned int  withLen, unsigned int from, unsigned int align)
{
  derefer ();
  int count = 0;
  int begin =from;
  while (begin>=0)
  {
    begin=replace (e, len, with, withLen, begin, align);
    if (begin > 0) count++;
  }
  return count;
}


/**
 * Clear the array and make a new SShared.
 */
void
SBVector::clear ()
{
  derefer ();
  cleanup ();
  buffer = new SShared();
}



/**
 * Find a subvector.
 */
int
SBVector::find (const char* e, unsigned int len, unsigned int from, unsigned int align) const
{
  //fprintf (stderr, "Address=%lx\n", (unsigned long) (e));
  unsigned int sz = size();
  char* _array = buffer->array;
  unsigned j;
  for (unsigned int i=from; i+len <= sz; i+=(unsigned int)align)
  {
    for (j=0; j<len; j++)
    {
  //fprintf (stderr, "array[%u + %u]=%d, e[%u] =%d\n", i, j, (int) buffer->array[i+j], j, (int) e[j]);
      if (_array[i+j] != e[j]) break;
    }
    if (j>=len) return (int) i;
  }
  return -1024;
}


/**
 * This is a replay of the obove for SObject collections
 */
SOVector::SOVector(void) : SBVector()
{
}

/**
 * This is a replay of the obove for SObject collections
 */
SOVector::SOVector(unsigned int _size) : SBVector(_size * sizeof (SObject*))
{
}

SOVector::SOVector(const SOVector& v) : SBVector(v)
{
}

SOVector::~SOVector ()
{
  cleanup();
}

SOVector&
SOVector::operator=(const SOVector& v)
{
  refer (v);
  return *this;
}

SObject*
SOVector::clone() const
{
  return new SOVector (*this);
}

void
SOVector::replace (unsigned int index, const SObject& v)
{
  derefer();
  SObject* r = (SObject*) peek (index); 
  replaceQuetly (index, &v);
  delete r;
}

void
SOVector::insert (unsigned int index, const SObject& v)
{
  derefer();
  SObject* ref= (SObject*) v.clone();
  SBVector::insert (index*sizeof(SObject*), (char*)&ref, sizeof (SObject*));
}

void
SOVector::remove (unsigned int index)
{
  derefer();
  SObject* old = (SObject*) peek (index); if (old) delete old;
  SBVector::remove (index * sizeof (SObject*), sizeof (SObject*));
}

unsigned int
SOVector::size () const
{
  return SBVector::size()/sizeof(SObject*);
}

void
SOVector::clear ()
{
  cleanup ();
  SBVector::clear();
}

void
SOVector::refer (const SOVector& v)
{
  if (&v != this)
  {
    cleanup();
    SBVector::refer(v);
  } 
}

const SObject*
SOVector::peek (unsigned int index) const
{
   return *(SObject**)SBVector::peek (index * sizeof (SObject*));
}

void
SOVector::derefer()
{
  if (count()==1) return;
  SBVector::derefer();

  for (unsigned int i=0; i<size(); i++)
  {
    SObject* r = (SObject*) peek (i); 
    replaceQuetly (i, r);
  }
}

void
SOVector::replaceQuetly (unsigned int index, const SObject* r)
{
  SObject* n = r->clone();
  SBVector::replace (index*sizeof(SObject*), (char*) &n, sizeof (SObject*));
}

void
SOVector::cleanup()
{
  if (count()!=1) return;
  for (unsigned int i=0; i<size(); i++)
  {
    SObject* r = (SObject*) peek (i); if (r==0) continue;
    delete r;
  }
}
