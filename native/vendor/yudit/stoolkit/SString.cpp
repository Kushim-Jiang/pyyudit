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
 
/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 */
#include "SString.h"
#include <stdio.h>
#include "stoolkit/STypes.h" 

SString SStringNull;

/**
 * constructs one.
 */
SString::SString(void) : SBinVector<char>()
{
 // SStringNUll may not have been inited here.
 //if (this != &SStringNull) refer(SStringNull);
}

SString::SString (const char* s) : SBinVector<char>()
{
  if (s!=0) SBinVector<char>::insert (0, s, strlen (s));
}

SString::SString (const char* s, unsigned int len) : SBinVector<char>()
{
 if (s!=0) SBinVector<char>::insert (0, s, len);
}

char*
SString::cString() const
{
    char* buff = new char [size() + 1];
    CHECK_NEW(buff);
    memcpy (buff, array(), size());
    buff[size()] = 0;
    return buff;
}

SString::SString (const char* s, unsigned int from, unsigned int len)
   : SBinVector<char>()
{
 if (s!=0) SBinVector<char>::insert (0, &s[from], len);
}

SString::SString (long l)
{
  SBinVector<char>::insert (0, (char*) &l, sizeof (long));
}

SString::SString (double d)
{
  SBinVector<char>::insert (0, (char*) &d, sizeof (double));
}

SString::SString (void* p)
{
  SBinVector<char>::insert (0, (char*) &p, sizeof (void*));
}

long
SString::longValue() const
{
  long l;
  memcpy (&l, array(), sizeof (l));
  return l;
}

double
SString::doubleValue() const
{
  double l;
  memcpy (&l, array(), sizeof (l));
  return l;
}

SString::~SString()
{
}

SObject*
SString::clone() const
{
 return new SString (*this);
}

SString&
SString::operator=(const SString& v)
{
 refer(v); return *this;
} 

void
SString::truncate(unsigned int i)
{
  SBinVector<char>::truncate(i);
}

int
SString::compare (const SString& e) const
{
  if (e.isNull() && !isNull()) return 1;
  if (!e.isNull() && isNull()) return -1;

  unsigned int cmplen = (size() > e.size()) ? e.size() : size();
  for (unsigned int i=0; i<cmplen; i++)
  {
    if((unsigned char)array()[i] > (unsigned char)e.array()[i]) return 1;
    if((unsigned char)array()[i] < (unsigned char)e.array()[i]) return -1;
  }
  if (size() > e.size()) return 1;
  if (size() < e.size()) return -1;
  return 0;
}

int
SString::find (const SString& v, unsigned int from) const
{
  return SBinVector<char>::find (v, from);
}

int
SString::find (char v, unsigned int from) const
{
  return SBinVector<char>::find (v, from);
}

int
SString::replace (const SString& e, const SString& with, unsigned int from)
{
  return SBinVector<char>::replace (e, with, from);
}

int
SString::replaceAll (const SString& e, const SString& with, unsigned int from)
{
  return SBinVector<char>::replaceAll (e, with, from);
}

void
SString::insert (unsigned int ind, const SString& v)
{
  SBinVector<char>::insert (ind, v);
}

void
SString::append (const SString& v)
{
 SBinVector<char>::append (v);
}

void
SString::append (char v)
{
 SBinVector<char>::append (v);
}

void
SString::append (long l)
{
 SBinVector<char>::append ((char*) &l, sizeof (long));
}

void
SString::append (const char* v, unsigned int len)
{
 SBinVector<char>::append (v, len);
}

void
SString::refer (const SString& v)
{
 SBinVector<char>::refer (v);
}

SString&
SString::operator << (const SString& e1)
{
  append (e1);
  return *this;
}

/*
SString&
operator << (SString& e1, const SString& e2)
{
  e1.append(e2);
  return e1;
}
*/

/**
* This requires at least 32 bit integers
* @param in the character array to hash.
*/
unsigned int
SString::hashCode () const
{
  unsigned int sz = size();
  unsigned int step = sz/8 +1;
  //unsigned int step = 1;

  unsigned int ret = 0;
  unsigned char * arr = (unsigned char*) array();
  for (unsigned int i=0; i<sz; i=i+step)
  {
    ret += (unsigned int) arr[i];
  }
  return ret;
}

/**
 * perform a stoolkit unix file match 
 * *ali*.map
 */
bool
SString::match (const SString& pattern) const
{
  bool sub=false;
  SString ths(*this);

  unsigned int j=0;
  for (unsigned int i=0; i<pattern.size(); i++)
  {
    if (pattern[i] == '*')
    {
      sub=true; continue;
    }
    if (sub)
    {
      unsigned int nextwild = i;
      while (nextwild < pattern.size() && pattern[nextwild] != '*')
      {
        nextwild++;
      }
      do 
      {
        while (j < ths.size() && ths[j] != pattern[i]) j++;
        if (j==ths.size()) return false;
        unsigned int k=0;
        while (j+k < ths.size() && i+k<nextwild && ths[j+k] == pattern[i+k])
        {
           k++;
        }
        if (i+k == nextwild)
        {
           break;
        }
        j++; continue;
      } while (true);
      sub=false;
    }
    if (ths[j] != pattern[i]) return false;
    j++;
  }
  if (sub==true) return true;
  if (ths.size() != j) return false;
  return true;
}


bool
SString::operator!=(const SString& e) const
{
  return !equals(e);
}

bool
SString::operator<(const SString& e) const
{
  /* is e greater ? */
  return (compare (e) < 0);
}
bool
SString::operator>(const SString& e) const
{
  /* is e less ? */
  return (compare (e) > 0);
}
bool
SString::operator<=(const SString& e) const
{
  /* is e greater or equal ? */
  return (compare (e) <= 0);
}
bool
SString::operator>=(const SString& e) const
{
  /* is e less ? or equal */
  return (compare (e) >= 0);
}

/**
 * Convert it to lower case
 */
void
SString::lower()
{
  for (unsigned int i=0; i<size(); i++)
  {
    char a =  peek(i);
    if (a >= 'A' && a <= 'Z')
    {
      a = (a + 'a' - 'A');
      SBinVector<char>::replace (i, a);
    }
  }
}

/**
 * Convert it to lower case
 */
void
SString::upper()
{
  for (unsigned int i=0; i<size(); i++)
  {
    char a =  peek(i);
    if (a >= 'a' && a <= 'z')
    {
      a = (a - ('a' - 'A'));
      SBinVector<char>::replace (i, a);
    }
  }
}

/**
 * Print a long value
 * @param in  input value to be appended to this string
 */
void
SString::print (long in)
{
  char a[64];
  snprintf (a, 64, "%ld", in);
  append (a);
}

/**
 * Print a long value
 * @param in  input value to be appended to this string
 */
void
SString::print (int in)
{
  char a[64];
  snprintf (a, 64, "%d", in);
  append (a);
}

/**
 * Print a long value
 * @param in  input value to be appended to this string
 */
void
SString::print (unsigned long in)
{
  char a[64];
  snprintf (a, 64, "%lu", in);
  append (a);
}

/**
 * Print a long value
 * @param in  input value to be appended to this string
 */
void
SString::print (unsigned int in)
{
  char a[64];
  snprintf (a, 64, "%u", in);
  append (a);
}

/**
 * Print a double value
 * @param in  input value to be appended to this string
 */
void
SString::print (double in)
{
  char a[64];
  snprintf (a, 64, "%g", in);
  append (a);
}
