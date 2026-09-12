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
#include "SProperties.h"
#include "SExcept.h"

#include <stdio.h>
#include <string.h>

SProperties::SProperties (void) : SHashtable<SString>()
{
}

SProperties::SProperties (const SString& string)
{
  /* split into lines */
  SStringVector delimiters("\r\n,\n,\r", ",");
  SString current;

  unsigned int dsize = delimiters.size();
  unsigned int ssize = string.size();
  int found = (int) string.size();

  unsigned int di=0;
  unsigned int from=0;
  SString comment;
  while (ssize > from)
  {
    /* This is just a splitting rourtine.  We can not use 
       the Vector's split because that purges empty lines.
    */
    for (di=0; di<dsize; di++){
      found = string.find (delimiters[di], from);
      if (found>=0) break;
    }
    SString s =  (di < dsize) 
      ? SString (&string.array()[from], (unsigned int)found - from)
      : SString (&string.array()[from], string.size()  - from);

    from = (di < dsize) 
      ? (unsigned int) found + delimiters[di].size() 
      : string.size(); 

    if (s.size()>0 && s[0] == '#')
    {
      /* only pre-ceeding. */
      if (current.size()==0)
      {
        /* last line is not null terminated. */
        if (comment.size())
        {
           comment.append ("\n");
        }
        comment.append (s);
        continue;
      }
      s.clear();
    }
    if (s.size()>0 && s[s.size()-1] == '\\')
    {
      s.truncate (s.size()-1);
      current.append (s);
      continue;
    }
    current.append (s);
    bool white = true;
    for (unsigned j=0; j<current.size(); j++)
    {
      if (current[j] != ' ' && current[j] != '\t')
      {
        white = false;
        break;
      }
    }
    if (white)
    {
      current.clear();
      continue;
    }

    SStringVector kvle(current, "=", true);

    if (kvle.size()==0)
    {
      current.clear();
      continue;
    }
    /* no key-value pairs. nothing at all */
    if (kvle.size()==1 && kvle[0] == current)
    {
      current.clear();
      continue;
    }
    current.clear();

    /* save comments under property.key.#  */
    if (comment.size())
    {
      SString ckey = kvle[0];
      ckey.append (".#");
      put (ckey,  comment);
      comment.clear();
    }

    /* no value, only key exists */
    if (kvle.size()==1)
    {
      put (kvle[0], "");
      continue;
    }
    put (kvle[0], kvle[1]);
  }
}

SProperties::SProperties (const SProperties& base) : SHashtable<SString>(base)
{
}

SProperties::~SProperties  ()
{
}

SString
SProperties::toString() const
{
  SStringVector keys;
  for (unsigned int i=0; i<size(); i++)
  {
    for (unsigned int j=0; j<size(i); j++)
    {
      const SString* value;
      if ((value=get (i,j))==0) continue;
      const SString& k =  key (i,j);
      // Skip comments
      if( k.size() > 1 && k[k.size()-2]== '.' && k[k.size()-1] == '#')
      { 
        continue;
      }
      keys.append (k);
    }
  }
  /* sort them */
  keys.sort();
  SStringVector ret;
  for (unsigned int k=0; k<keys.size(); k++)
  { 
    const SString& key = keys[k];
    const SString* value = get(key); 
    /* restore comments */
    SString ckey = key;
    ckey.append (".#"); 
    const SString* comment = get (ckey);
    if (comment && comment->size() > 0)
    {
       ret.append (*comment);
    }
    SString line (key);
    line.append ("=");
    line.append (*value);
    ret.append (line);
  }
  return ret.join ("\n");
}

SObject*
SProperties::clone() const
{
  SProperties* p = new SProperties (*this);
  CHECK_NEW(p);
  return p;
}

/**
 * split a string into an array
 * @param s is the string to split
 * @param delim is the delimiter list
 * @param once is true if we quit after one spli
 * retgurn the key or SStringNull. returning key is not yet implemented
 */
SString&
SProperties::split (const SString& s, const SString& delim)
{
  int start = 0;
  //unsigned int count =0;
  /* Find the first delim. */
  SString mykey;
  while (start < (int) s.size())
  {
    int current;
    int smallest = s.size();

    for (unsigned int i=0; i<delim.size(); i++)
    {
       current = s.find (delim.array()[i], start);
       if (current == -1) continue;
       if (current < smallest) smallest = current;
    }
    if (smallest < 0) break;
    if (smallest != start)
    {
        //sub.trunc (smallest);
       if (smallest+1  >= (int) s.size()) break;
       mykey = SString(s.array(), start, smallest-start);
       SString value = SString(s.array(), smallest+1, (int)s.size()-smallest-1);
       //count += 2;
       put (mykey, value);
       break;
    }
    start = (smallest+1);
  }
  //return (count==2) ? *new SString(mykey) : SStringNull;
  // Just aint working yet...
  return SStringNull;
}

/**
 * merge the other property into this property
 */
void
SProperties::merge (const SProperties& other)
{
  for (unsigned int i=0; i<other.size(); i++)
  {
    for (unsigned int j=0; j<other.size(i); j++)
    {
      const SString* value;
      if ((value=other.get (i,j))==0) continue;
      const SString& key =  other.key (i,j);
      put (key, *value);
    }
  }
}

SString
SProperties::getProperty (const SString& prop, const SString& fallback) const
{
  const SString* p = get(prop);
  if (p==0) p = &fallback;
  return SString (*p);
}
