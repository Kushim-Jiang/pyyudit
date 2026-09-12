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
 
#include "SConfig.h"
#include "SIOStream.h"
#include <stdio.h>

/**
 *----------------------------------------------------------------------------
 * This file is obsolete. 2001-01-12
 *----------------------------------------------------------------------------
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 */

/**
 * A config file consists of sections in brackets
 *    [section first section]
 * Inside a section (below the [] thingy:
 *    key 1=value 1
 *    key 2=value 2
 * key-value pairs are
 * Sections starting with '#' are comments.
 */
#ifdef USE_WINAPI
SString SLineSep("\r\n");
#else
SString SLineSep("\n");
#endif
SConfig::SConfig (const SFile& config) : file (config)
{
}

SConfig::SConfig (const SConfig& config) : file (config.file)
{
  sectHashtable = config.sectHashtable;
}

SConfig&
SConfig::operator = (const SConfig& config)
{
  file = config.file;
  sectHashtable = config.sectHashtable;
  return *this;
}

void
SConfig::setFile(const SFile& newFile)
{
  file = newFile;
}


SConfig::~SConfig()
{
}

/**
 * Write out the config in the file.
 * @return false on error.
 */
bool
SConfig::write()
{
  SInputStream o = file.getOutputStream();
  if (!o.isOK()) return false;
  SWriter w(o);
  SStringVector sections = keys();
  SString banner;
  banner << "# Property file fo Yudit" << SLineSep;
  w.write (banner);
  for (unsigned int i=0; i<sections.size(); i++)
  {
    SString sl;
    sl << SLineSep << "[" << sections[i] << "]" << SLineSep;
    w.write (sl);
    SStringVector props = keys(sections[i]);
    for (unsigned int j=0; j<props.size(); j++)
    {
       SString line;
       line << props[j] << "=" << get (sections[i], props[j]) << SLineSep;
       w.write (line);
    }
    if (!w.isOK()) break;
  }
  if (!w.isOK()) return false;
  if (!o.close()) return false;
  return true;
}

/**
 * Read the config in the file.
 * @return 0 on ok or line number on bad file or -1 on read error
 */

int
SConfig::read ()
{
  SInputStream i = file.getInputStream();
  if (!i.isOK()) return -1;
  SStringVector sepVector("\n,\r\n,\r");
  SReader r(i, sepVector);
  SString line;
  SString section;
  int linenum =0;
  SProperties list;
  while (true)
  {
    linenum++;
    bool ret = r.read(&line);
    if (ret == false) return -1;

    // EOF
    if (line.size()==0)
    {
      if (section.size()!=0)
      {
        set (section, list);
      }
      break;
    }

    sepVector.trim (&line);
    if (line[0] == '#') continue;
    if (line[0] == '!') continue;
    if (line[0] == '[')
    {
      int ends = line.find("]");
      if (ends <= 0) return linenum;
      SString sub(line.array(), 1, ends-1);
      if (sub.size()==0) return linenum;
      if (section.size()!=0)
      {
        set (section, list);
      }
      list.clear();
      section = sub;
      continue;
    }
    SStringVector l;
    if (l.split (line, "=", true) !=2) continue;
    list.put (l[0], l[0]);
  }
  return 0;
}

/**
 *@return the section keys
 */
SStringVector
SConfig::keys()
{
  SStringVector k;
  sectHashtable.keys (&k);
  return SStringVector(k);
}

/**
 *@return the  keys
 */
SStringVector
SConfig::keys(const SString& section)
{
  SStringVector k;
  if (sectHashtable.get(section)!=0)
  {
    sectHashtable[section].keys (&k);
  }
  return SStringVector(k);
}

/**
 * Get the value in section with a key.
 */
const SString&
SConfig::get (const SString& section, const SString& key)
{
  if (sectHashtable.get(section)==0)
  {
     return SStringNull;
  }
  const SString* vle = sectHashtable[section].get (key);
  if (vle ==0) return SStringNull;
  return sectHashtable[section][key];
}

/**
 * Set the appropriate value.
 * create the section if it does not exist.
 * @param section is the section to set.
 * @param key is the key in the section 
 * @param vle is the value 
 */
void
SConfig::set(const SString& section, const SString& key, const SString& vle)
{
  if (sectHashtable.get(section)==0)
  {
    SProperties n;
    sectHashtable.put (section, n);
  }
  /**
   * This is crying for optimization
   */
  SProperties l = sectHashtable[section];
  l.put (key, vle);
  sectHashtable.put (section, l);
}

/**
 * A bit higher level set.
 * Set the appropriate value.
 * create the section if it does not exist.
 * @param section is the section to set.
 * @param list is the list to replace.
 */
void
SConfig::set(const SString& section, const SProperties& list)
{
  sectHashtable.put (section, list);
}

/**
 * Erase a key from the config.
 */
void
SConfig::remove (const SString& section, const SString& key)
{
  if (sectHashtable.get (section)==0) return;
  /**
   * This is crying for optimization
   */
  SProperties l = sectHashtable[section];
  l.remove (key);
  if (l.size()==0)
  {
    sectHashtable.remove (section);
  }
  else
  {
    sectHashtable.put (section, l);
  }
}

/**
 * Erase a whole section from the config.
 */
void
SConfig::remove (const SString& section)
{
  sectHashtable.remove (section);
}
