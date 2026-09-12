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
 
#ifndef SStringVector_h
#define SStringVector_h

#include "SString.h"
#include "SVector.h"

#include <string.h>

/**
 * We make the SBinVector private so that nobody can screw with 
 * The autodelete.
 */
class SStringVector : public SVector<SString>
{
public:
  SStringVector (void);
  SStringVector (const SStringVector& base);
  SStringVector (const SString& base);
  SStringVector (const char* base);
  SStringVector (const SString& base, const SString& delim, bool once=false);
  virtual SObject* clone() const;
  virtual ~SStringVector  ();
  /* Split a string and add it to the list */
  unsigned int split (const SString& s, const SString& delimiters, bool once=false);
  void sort(bool ascending=true);  
  /* splits and leave \" marked text untact */
  unsigned int smartSplit (const SString& s);
  int trim(SString* in) const;
  SString join(const SString& delimiter) const;
  void append (const SStringVector& v);
  void append (const SString& str);
  void append (const char* str);
private:
  void sort(unsigned int* indeces, bool ascending, int left, int right);
  int compare (unsigned int* indeces, int i1, int i2);
};

#endif /* SStringVector _h*/
