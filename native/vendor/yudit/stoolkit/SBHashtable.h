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
 
#ifndef SBHashtable_h
#define SBHashtable_h

/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 */
#include "SObject.h"
#include "SHShared.h"
#include "SString.h"
#include "SStringVector.h"


/**
 * This is almost the same as vector, bu index is not integer but string.
 */
class SBHashtable : public SObject
{
public:
  SBHashtable(void);
  virtual ~SBHashtable ();
  SBHashtable (const SBHashtable& v);
  SBHashtable& operator=(const SBHashtable& v);
  virtual SObject* clone() const;
  
  /* This is in the library */
  void  put (const SString& key, const char* e, int len, bool replace=true); 

  void  remove (const SString& key);
  void  clear ();
  const char* get (const SString& key) const;

  unsigned int   size () const;
  unsigned int   size (int bucket) const;

  static int debug (int level);
  void keys (SStringVector* list) const;
protected:
  const char* get (unsigned int bucket, unsigned int subbucket) const;
  void put (unsigned int bucket, unsigned int subbucket, 
                 const char* e, int len);
  const SString& key (unsigned int bucket, unsigned int subbucket) const;
  void  rehash ();
  void  refer(const SBHashtable& v);
  void  derefer ();
  void  ensure (unsigned int more);
  SHShared*  buffer;
  void  cleanup ();
private:
  static int debugLevel;
};

/**
 * This is an Object hash.
 */
class SOHashtable : public SBHashtable
{
public:
  SOHashtable (void);
  SOHashtable (const SOHashtable& base);
  virtual SObject* clone() const;
  SOHashtable& operator=(const SOHashtable& v);
  virtual ~SOHashtable  ();

  const SObject* get (const SString key) const;
  const SObject* get (unsigned int row, unsigned int col) const;

  void put (const SString& key, const SObject& e, bool replace=true);
  void put (unsigned int bucket, unsigned int subbucket, SObject& e);
  void remove (const SString& key);
  void clear ();
protected:
  void refer (const SOHashtable& v);
  void derefer();
private:
  void cleanup();
};

#endif /* SBHashtable_h */
