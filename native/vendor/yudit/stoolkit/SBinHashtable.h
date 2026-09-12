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
 
#ifndef SBinHashtable_h
#define SBinHashtable_h

/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 */
#include "SBHashtable.h"
#include "SString.h"
#include "SExcept.h"

/**
 * This is my vector. stoolkit as that. There is no code,
 * just this.
 */
template <class Type> class SBinHashtable : public SBHashtable
{
public:
  SBinHashtable (void) : SBHashtable () { }
  virtual ~SBinHashtable () { } // SBHashtable cleanup will be automatically called.  
  SBinHashtable (const SBinHashtable<Type>& v) : SBHashtable (v) { }
  virtual SObject* clone() const 
  { SBinHashtable* n = new SBinHashtable (*this); CHECK_NEW(n); return n;}

  SBinHashtable<Type>& operator=(const SBinHashtable<Type>& v)
    { refer(v); return *this; }

  void put (const SString& key, Type e, bool replace=true)
    {
      SBHashtable::put (key, (const char*) &e, sizeof (Type), replace);
    }
  void put (unsigned int row, unsigned int sub, Type e)
    {
      SBHashtable::put (row, sub, (const char*) &e, sizeof (Type));
    }
  void remove (const SString& key)
    {
      SBHashtable::remove (key); 
    }
  // This will dlete
  void  clear () 
     { SBHashtable::clear(); }
  unsigned int size () const
    { return SBHashtable::size(); }
  unsigned int size (unsigned int subb) const
    { return SBHashtable::size(subb); }
  Type get (const SString& key) const
    { return (Type) (*(Type*)SBHashtable::get(key)); }
  Type operator[] (const SString& key)  const
    { return (Type) (*(Type*)SBHashtable::get(key)); }
  Type get (unsigned int row, unsigned int column) const
    { return (Type) (*(Type*)SBHashtable::get(row, column)); }
  const SString& key (unsigned int row, unsigned int column) const
    { return SBHashtable::key(row, column); }
};

#endif /* SBinHashtable_h _h*/
