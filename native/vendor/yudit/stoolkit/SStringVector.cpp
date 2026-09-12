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
#include "SStringVector.h"
#include <string.h>

SStringVector::SStringVector (void) : SVector<SString>()
{
}

SStringVector::SStringVector (const SStringVector& base) : SVector<SString>(base)
{
}
/**
 * Do a split on base using delim.
 * empty elements are not put in the resulting vector
 */
SStringVector::SStringVector (const SString& base, const SString& delim, bool once)
  : SVector<SString>()
{
  split(base, delim, once);
}

SStringVector::SStringVector (const SString& base)
  : SVector<SString>()
{
  split(base, ",");
}

SStringVector::SStringVector (const char* base)
  : SVector<SString>()
{
  split(base, ",");
}

SObject*
SStringVector::clone() const
{
  return new SStringVector (*this);
}

SStringVector::~SStringVector  ()
{
}

/**
 * split a string into an array
 * @param s is the string to split
 * @param delim is the delimiter list
 * @param once is true if we quit after one spli
 * retgurn  the number of splits
 */
unsigned int 
SStringVector::split (const SString& s, const SString& delim, bool once)
{
  derefer ();

  char* excl = new char[256];
  memset (excl, 0, 256);

  unsigned int dsize = delim.size();
  for (unsigned int d=0; d<dsize; d++)
  {
     unsigned char index = (unsigned char) delim.array()[d];
     excl[index] = 1;
  }
  /* Find the first delim. */
  unsigned int start = 0;
  unsigned int count =0;
  unsigned int ssize = s.size();
  while (start < ssize)
  {
    unsigned int current;

    for (current=start; current<s.size(); current++)
    {
       if (excl[(unsigned char)s.array()[current]] == 1) break;
    }
    if (current != start)
    {
        SString rep (s.array(), start, current-start);
        append (rep);
        count++;
        if (once)
        {
          if (current < s.size())
          {
            SString ns (s.array(), current+1, s.size()-current-1);
            append (ns);
            count++;
          }
          return count;
        }
    }
    start = (current+1);
  }
  delete []excl;
  return count;
}

/**
 * Remove delimiters
 * return the index which matched.
 */
int
SStringVector::trim(SString *s) const
{
  int found =-1;
  unsigned int i;
  int less = 0;
  int where = -1;
  for (i=0; i<size(); i++)
  {
     found = s->find (*peek(i));
     if (found<0) continue;
     if (found < less || where ==-1)
     {
        less = found;
        where = i;
     }
  }
  if (where >= 0)
  {
     s->truncate (less);
     return where;
  }
  return -1;
}

SString
SStringVector::join(const SString& delimiter) const
{
  SString str;
  for (unsigned int i=0; i<size(); i++)
  {
    str.append (*peek(i));
    if  (size() != i+1)
    {
      str.append (delimiter);
    }
  }
  return SString(str);
}

unsigned int
SStringVector::smartSplit (const SString& sin)
{
  derefer ();
  char quoted = 0;

  char* excl = new char[256];
  memset (excl, 0, 256);

  SString delim("\t\n\r ");
  SString s (sin);
  for (unsigned int d=0; d<delim.size(); d++)
  {
     unsigned char index = (unsigned char) delim.array()[d];
     excl[index] = 1;
  }
  /* Find the first delim. */
  unsigned int start = 0;
  unsigned int count =0;
  bool escaped = false;
  while (start < s.size())
  {
    unsigned int current;

    escaped = false;
    for (current=start; current<s.size(); current++)
    {
       if (quoted==0)
       {
         if (!escaped && (s.array()[current] == '\'' || s.array()[current] == '\"'))
         {
           quoted = s.array()[current];
           start++;
           continue;
         }
         if (s.array()[current] == '\\' && current+1<s.size())
         {
           s.remove (current);
           escaped = true;
           continue;
         }
         escaped = false;
         if (excl[(unsigned char)s.array()[current]] == 1)
         {
           break;
         }
       }
       else
       {
         if (s.array()[current] == '\\' && current+1<s.size())
         {
           s.remove (current);
           escaped = true;
           continue;
         }
         if (!escaped && s.array()[current] == quoted)
         {
            break;
         }
         escaped = false;
       }
    }
    if (current != start || quoted)
    {
        if (current == start)
        {
          append (SString(""));
        }
        else
        {
          SString rep (s.array(), start, current-start);
          char c = rep[rep.size()-1];
          if (quoted && (c == '\'' || c ==  '"'))
          {
            rep.truncate (rep.size()-1);
          }
          append (rep);
        }
        quoted = 0;
        count++;
    }
    start = (current+1);
  }
  delete []excl;
  return count;
}

void
SStringVector::sort(bool ascending)
{
  if (size()<2) return;
  unsigned int* indeces = new unsigned int[size()];
  CHECK_NEW(indeces);
  unsigned int i;
  for (i=0; i<size(); i++)
  {
    indeces[i] = i;
  }
  sort (indeces, ascending, 0, size()-1);
  SStringVector nv(*this);
  clear();
  for (i=0; i<nv.size(); i++)
  {
//fprintf (stderr, "append[%u]=%u [%*.*s]\n", i, 
 //     indeces[i], SSARGS(nv[indeces[i]]));
    append (nv[indeces[i]]);
  }
  delete[] indeces;
  return;
}

void
SStringVector::sort(unsigned int* indeces, bool ascending, 
    int left, int right)
{
  int  i=left;
  int  j=right;
  int  pivot;
  int  mid;

  int  asc = (ascending)?-1:1;

  if (right > left)
  {
    pivot = (right+left)/2;
    mid= pivot;
    // Make a partition
    while (true)
    {
        // We compare everything to the pivot
      while (i<right && (compare (indeces, i, mid) * asc)  >= 0) i++;
      while (j>left && (compare (indeces, j, mid)  * asc) < 0) j--;

      // Left index reached right index 
      if (i>=j) break;

      // swap - it may swap the pivot as well.
      if (i==mid) mid=j;
      else if (j==mid) mid =i;
        // Swap
      unsigned int o = indeces[(unsigned int)i]; 
      indeces[(unsigned int)i] = indeces[(unsigned int)j];
      indeces[(unsigned int)j] = o;
    }
    // Make sure we don't sort the pivot - originally it is
    // in the left array so we exchange it with something
    // that belongs to left and lies in the boundary.
    // It might be the pivot itself...
    unsigned int o = indeces[(unsigned int)mid];
    indeces[(unsigned int)mid] = indeces[(unsigned int)j];
    indeces[(unsigned int)j] = o;

    sort (indeces, ascending, left, j-1);
    sort (indeces, ascending, i, right);
  }
}
/**
 * compare addreseed through indeces.
 */
int
SStringVector::compare (unsigned int* indeces, int i1, int i2)
{
   const SString& js = *peek (indeces[(unsigned int) i1]);
   const SString& ms = *peek (indeces[(unsigned int) i2]);
   return js.compare(ms);
}
void
SStringVector::append (const SStringVector& v)
{
  unsigned int sz = v.size();
  for (unsigned int i=0; i<sz; i++)
  {
    append (v[i]);
  }
}

void
SStringVector::append (const SString& str)
{
  SVector<SString>::append (str);
}

void
SStringVector::append (const char* str)
{
  SVector<SString>::append (SString(str));
}
