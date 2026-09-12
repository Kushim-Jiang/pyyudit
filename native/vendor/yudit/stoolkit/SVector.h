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
 
#ifndef SVector_h
#define SVector_h


/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 */
#include "SBVector.h"
#include "SExcept.h"

/**
 * This is my vector. stoolkit as that. There is no code,
 * just this. This vector is for basic types like int, char e.t.c.
 * This one makes a copy of the object.
 */
template <class BType> class SVector : public SOVector
{
public:
  SVector (void) : SOVector () {}
  virtual ~SVector () {}
  virtual SObject* clone() const
  { SVector* n = new SVector (*this); CHECK_NEW(n); return n;}
  SVector (const SVector& v) : SOVector (v) { }
  SVector (BType& e) : SOVector () { append (e);}
  SVector& operator=(const SVector& v) { refer(v); return *this; }
 
  const BType* peek (unsigned int index) const
    { return (BType*) SOVector::peek (index); }

  void append (const BType& v)
    { SOVector::insert (size(), v); }

  void replace (unsigned int index, const BType& v) 
    { SOVector::replace (index, v); }

  void insert (unsigned int index, const BType& v)
    { SOVector::insert (index, v); }

  void remove (unsigned int index)
    { SOVector::remove (index); }

  void remove (unsigned int index, unsigned int _size)
    { for (unsigned int i=0; i<_size; i++) SOVector::remove (index); }

  void truncate (unsigned int _size)
    { while (size()>_size) SOVector::remove (size()-1); }

  const BType& operator[] (unsigned int index) const
    { return *peek(index); }

  //friend SVector& operator << (SVector& e1, const BType& e2)
   // { e1.append(e2); return e1; }

  unsigned int size () const 
    { return SOVector::size (); }

  unsigned int appendSorted (const BType& v)
  {
      unsigned int top = findSorted (v);
      // Append at the end...
      for (unsigned int i=top; i<size(); i++)
      {
         if (v.compare(*peek(i)) == 0) top++; 
      }
      insert (top, v);
      return top;
  }
  unsigned int findSorted (const BType& v) const
  {
      unsigned int    top, bottom, mid;
      top = size(); bottom = 0;
      while (top > bottom) {
        mid = (top+bottom)/2;
        if (v.compare(*peek(mid))==0) { top = mid; break; }
        if (v.compare(*peek(mid))<0) { top = mid; continue; }
        bottom = mid + 1;
      }
      return top;
  }
};

#endif /* SVector _h*/
