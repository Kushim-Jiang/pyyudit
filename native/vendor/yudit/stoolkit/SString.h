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
 
#ifndef SString_h
#define SString_h

#include "SBinVector.h"

#include <string.h>

// This is for %*.*s printf
#define SSARGS(_s) (int)_s.size(), (int)_s.size(), _s.array()
/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 */
class SString : public SBinVector<char>
{
public:
  SString(void);
  SString (long l);
  SString (double d);
  SString (void* p);
  SString (const char* s);
  SString (const char* s, unsigned int len);
  SString (const char* s, unsigned int from, unsigned int len);
  virtual SObject* clone() const;
  SString& operator=(const SString& v);
  void print (int in);
  void print (unsigned int in);
  void print (long in);
  void print (unsigned long in);
  void print (double in);
  virtual ~SString ();

  int find (const SString& v, unsigned int from =0) const;
  int find (char v, unsigned int from =0) const;

  int replace (const SString& e, const SString& with, unsigned int from =0);
  int replaceAll (const SString& e, const SString& with, unsigned int from=0);
  void insert (unsigned int ind, const SString& v);
  void append (const SString& v);
  void append (long l);
  void append (const char *, unsigned int len);
  void append (char);
  bool match (const SString& pattern) const;

  SString& operator << (const SString& e2);
  //friend SString& operator << (SString& e1, const SString& e2);

  unsigned int hashCode () const;
  long longValue() const;
  double doubleValue() const;
  inline bool operator == (const SString& e) const;
  bool operator != (const SString& e) const;
  bool operator < (const SString& e) const;
  bool operator > (const SString& e) const;
  bool operator <= (const SString& e) const;
  bool operator >= (const SString& e) const;

  int compare (const SString& s) const;
  void truncate (unsigned int size);
  void lower ();
  void upper ();
    
  char* cString() const;
  
protected:
  void refer (const SString& v);
};

extern SString SStringNull;

//PUBLIC/INLINE
/**
 * Is it equal
 */
bool
SString::operator==(const SString& e) const
{
  return equals(e);
}


#endif /* SString _h*/
