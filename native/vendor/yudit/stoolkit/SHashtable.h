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
 
#ifndef SHashtable_h
#define SHashtable_h

/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 */
#include "SBHashtable.h"
#include "SString.h"
#include "SExcept.h"


/**
 * This is a hash for SObjects
 */ 
template <class BType> class SHashtable : public SOHashtable
{
public:
  SHashtable (void) : SOHashtable() {}
  SHashtable (const SHashtable& base) : SOHashtable(base)  {};
  virtual SObject* clone() const
  { SHashtable* n = new SHashtable (*this); CHECK_NEW(n); return n;}
  virtual ~SHashtable  () { }
  SHashtable& operator=(const SHashtable& v) { refer (v);  return *this; } 

  const BType* get (const SString& key) const
    { return (BType*) SOHashtable::get (key); }

  const SString& key (unsigned int row, unsigned int col) const
    { return SBHashtable::key (row, col); }

  const BType* get (unsigned int row, unsigned int col) const
    { return (BType*) SOHashtable::get(row, col); }

  void put (const SString& key, const BType& e, bool replace=true)
    {  SOHashtable::put (key, e, replace); }

  void remove (const SString& key) { SOHashtable::remove (key); }

  void clear () { SOHashtable::clear(); }

  const BType& operator[] (const SString& key) const { return *get (key);  }
};

#endif /* SHashtable_h _h*/
