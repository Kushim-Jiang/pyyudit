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
 
#ifndef SBinVector_h
#define SBinVector_h


/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 */
#include "SBVector.h"
#include "SExcept.h"
#include <stdio.h>


/**
 * This is my vector. stoolkit as that. There is no code,
 * just this. This vector is for basic types like int, char e.t.c.
 */
template <class Type> class SBinVector : public SBVector
{
public:
  SBinVector (void) : SBVector () { }
  virtual ~SBinVector () {}
  SBinVector (const SBinVector<Type>& v) : SBVector(v) {};
  virtual SObject* clone() const
  { SBinVector* n = new SBinVector (*this); CHECK_NEW(n); return n;}

  int replace (const SBinVector&e, const SBinVector&with, unsigned int from=0)
  {
      return SBVector::replace((char*)e.array(), e.size()*sizeof (Type),
               (char*)with.array(), with.size()*sizeof(Type), 
               from, sizeof(Type))/sizeof(Type);
  }

  int replaceAll (const SBinVector&e, const SBinVector&with, unsigned int from=0)
  {
      return SBVector::replaceAll((char*)e.array(), e.size()*sizeof (Type),
               (char*)with.array(), with.size()*sizeof(Type), 
               from, sizeof(Type));
  }

  void append (const SBinVector<Type>& v)
    { insert (size(), v.array(), v.size()); }

  void append (const Type& v)
    { insert (size(), &v); }

  void append (const Type* v, unsigned int len)
    { insert (size(), v, len); }

  void insert (unsigned int ind, const Type& v)
    { insert (ind, &v, 1); }

  void insert (unsigned int ind, const SBinVector<Type>& v)
    { insert (ind, v.array(), v.size()); }

  unsigned int appendSorted (const Type& v)
  {
      unsigned int top = findSorted (v);
      // Append at the end...
      for (unsigned int i=top; i<size() && v == peek(i); i++)
      {
         top++; 
      }
      insert (top, v);
      return top;
  }
  /*
   * It just returns the smallest not necessarily the same
   */
  unsigned int findSorted (const Type& v)
  {
      unsigned int    top, bottom, mid;
      top = size(); bottom = 0;
      while (top > bottom) {
        mid = (top+bottom)/2;
        Type m = peek(mid);
        if (v == m) { top = mid; break; }
        if (v < m) { top = mid; continue; }
        bottom = mid + 1;
      }
      return top;
  }

  Type peek (unsigned int index) const
    { return *((Type*)SBVector::peek(index * sizeof (Type))); }

  void truncate (unsigned int _size)
    { remove (_size, size() - _size); }

  void ensure (unsigned int _size) 
    { SBVector::ensure (_size * sizeof (Type)); }

  unsigned int size () const
    { return SBVector::size() / sizeof (Type); }

  Type operator[] (unsigned int index) const
    { return (Type) (*(Type*)SBVector::peek(index * sizeof (Type))); }

  const Type* array () const
    { return (Type*) SBVector::array(); }


  void insert (unsigned int index, const Type* e, unsigned int len=1)
  {
      SBVector::insert (index*sizeof(Type), (const char*) e, len * sizeof (Type));
  }

  void replace (unsigned int index, Type e)
  {
      SBVector::replace (index*sizeof (Type), (const char*) &e, sizeof (Type));
  }

  void remove (unsigned int index, unsigned int len=1)
  {
      SBVector::remove (index*sizeof (Type), len * sizeof (Type)); 
  }

  int find (const SBinVector<Type>& v, unsigned int from=0) const
  {
    int start = SBVector::find ((char*) v.array(), v.size(),
          from * sizeof(Type), sizeof (Type));
    return (start<0) ? -1 : start/sizeof(Type);
  }

  int find (const Type v, unsigned int from=0) const
  {
    const Type tmpV = v;
    int start = SBVector::find ((const char*) (&tmpV), 
        sizeof (Type), from * sizeof(Type),
      sizeof (Type));
    return (start<0) ? -1 : start/sizeof(Type);
  }
};

#endif /* SBinVector _h*/
