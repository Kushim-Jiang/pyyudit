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
 
#ifndef SBVector_h
#define SBVector_h

/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 */
#include "SObject.h"
#include "SShared.h"
#include "SExcept.h"

#include <string.h>

class SBVector : public SObject
{
public:
  inline SBVector(void);
  inline SBVector(unsigned int size);
  inline virtual ~SBVector ();

  SBVector (const SBVector& v);
  SBVector& operator=(const SBVector& v);
  virtual SObject* clone() const 
  { SBVector* n = new SBVector (*this); CHECK_NEW(n); return n;}
  
  /**
   * insert, replace, replaceAll, remove, clear require that you properly 
   * derefer objects.
   */
  inline void  insert (unsigned int index, const char* e, unsigned int len);
  inline void  remove (unsigned int index, unsigned int len);
  void  replace (unsigned int index, const char* e, unsigned int len);
  void  clear ();

  int   replace (const char* e, unsigned int len, 
             const char* with, unsigned int withLen, unsigned int from=0, unsigned int align = 1);
  int   replaceAll (const char* e, unsigned int len, 
             const char* with, unsigned int  withLen, unsigned int from=0, unsigned int align = 1);

  inline char* peek (unsigned int index) const;
  int   find (const char* e, unsigned int len, unsigned int from, unsigned int align=1) const;

  inline unsigned int  size () const;
  static int    debug (int level);
  inline bool isNull () const;
  inline bool equals (const SBVector& v) const;
  inline const char* array() const;

protected:
  inline unsigned int count () const;
  inline void  derefer ();
  void  refer(const SBVector& v);
  inline void  ensure (unsigned int more);
private:
  SShared*  buffer;
  inline void  cleanup ();
};

/**
 * This is the base for all object vectors
 */
class SOVector : public SBVector
{
public:
  SOVector (void);
  SOVector (unsigned int size);
  SOVector (const SOVector& v);
  virtual ~SOVector ();
  virtual SObject* clone() const;
  SOVector& operator=(const SOVector& v);

  void replace (unsigned int index, const SObject& v);
  void insert (unsigned int index, const SObject& v);
  void remove (unsigned int index);
  inline void  ensure (unsigned int more)
   { SBVector::ensure (more * sizeof (SObject*)); }

  unsigned int size () const;

  void clear ();
  void refer (const SOVector& v);
  const SObject* peek (unsigned int index) const;

protected:
  void derefer();

private:
  void replaceQuetly (unsigned int index, const SObject* r);
  void cleanup();
};


//PUBLIC/INLINE
/**
 * This is the vector class. I don't prefer using STL.
 */
SBVector::SBVector(void)
{
    buffer = new SShared();
}
/**
 * This is the vector class. I don't prefer using STL.
 */
SBVector::SBVector(unsigned int _size)
{
    buffer = new SShared(_size);
}


//PUBLIC/INLINE
/**
 * Return the size of the char array
 */
unsigned int
SBVector::size() const
{
  return buffer->vectorSize;
}

//PUBLIC/INLINE
bool
SBVector::isNull () const
{
  return buffer->arraySize==0;
}

//PUBLIC/INLINE
const char*
SBVector::array() const
{
  return buffer->array;
}

//PUBLIC/INLINE
/**
 * Return the element at index.
 * @param index is the reference index of len blocks
 */
char*
SBVector::peek (unsigned int index) const
{
  if (index > buffer->vectorSize) return 0;
  return &buffer->array[index];
}


//PROTECTED/INLINE
unsigned int
SBVector::count() const
{
  return buffer->count;
}

//PROFILE - made inline, moved on top of source file
/**
 * The array will change. If the buffer is shared copy the buffer.
 */
void
SBVector::derefer()
{
  if (buffer->count==1) return;
  buffer->count--;
  buffer = new SShared (*buffer);
}

//PROFILE - made inline, moved to the top of source file
/**
 * clean up. usually called before delete.
 * remove the buffer or its reference.
 */
void
SBVector::cleanup ()
{
  if (buffer->count==1) {
    delete buffer;
  } else {
    buffer->count--;
  }
}
//PUBLIC
/**
 * The destructor
 */
SBVector::~SBVector()
{
  cleanup();
}

/**
 * Insert char's with len length at index. 
 * @param index is the reference index of len blocks
 * @param e is the pointer to the char array to be saved
 * @param len is the length of the block.
 */
void 
SBVector::insert (unsigned int index, const char* in, unsigned int len)
{
  derefer ();
  if (len==0 && buffer->arraySize!=0) return;
  ensure (len); /* Allocate at least len bytes*/
#ifdef NO_MEMMOVE 
  register char* _array = buffer->array;
  for (register unsigned int i=buffer->vectorSize; i>index; i--)
  {
    _array[i+len-1] = _array[i-1];
  }
#else /*NO_MEMMOVE*/
  if (index<buffer->vectorSize)
  {
    memmove (&buffer->array[index+len], &buffer->array[index], 
      buffer->vectorSize-index);
  }
#endif /*NO_MEMMOVE*/
  memcpy (&buffer->array[index], in, len);
  buffer->vectorSize += len;
}

/**
 * Remove char's with len length at index 
 * @param index is the reference index of len blocks
 * @param len is the length of the block.
 */
void 
SBVector::remove (unsigned int index, unsigned int len)
{
  derefer ();
  if (len ==0) return;
  // Move the elements down.
#ifdef NO_MEMMOVE 
  register char* _array = buffer->array;
  for (register unsigned int i=index+len; i<buffer->vectorSize; i++)
  {
    _array[i-len] = _array[i];
  }
#else
  memmove (&buffer->array[index], &buffer->array[index+len], 
    buffer->vectorSize-index-len);
#endif
  buffer->vectorSize -= len;
}

/**
 * Ensure that we have enough capacity.
 * Before this, the reference count should be one!
 * int more - the elements in bytes that we need.
 */
void
SBVector::ensure(unsigned int more)
{
  buffer->ensure (more);
}


bool
SBVector::equals (const SBVector& e) const
{
  //if (e.array() == array()) return true;

  unsigned int cmplen = e.size();
  if (size() != cmplen) return false;
  return (memcmp (array(), e.array(), cmplen) == 0);

}
#endif /* SBVector_h */
