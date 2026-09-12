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
 
#ifndef SConfig_h
#define SConfig_h

#include "SIO.h"
#include "SString.h"
#include "SStringVector.h"
#include "SProperties.h"
#include "SHashtable.h"

/**
 *----------------------------------------------------------------------------
 * This file is obsolete. 2001-01-12
 *----------------------------------------------------------------------------
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 */

typedef SHashtable<SProperties> SSectionHashtable;

class SConfig 
{
public:
  SConfig (const SFile& config);
  SConfig (const SConfig& config);
  SConfig& operator = (const SConfig& config);
  virtual ~SConfig();

  const SString& getName() { return file.getName(); }
  void setFile (const SFile& newFile);

  bool write ();
  int read ();

  void set(const SString& section, const SString& key, const SString& vle);
  void set(const SString& section, const SProperties& list);

  SStringVector keys ();
  SStringVector keys (const SString& section);
  const SString& get (const SString& section, const SString& key);

  void remove (const SString& section, const SString& key);
  void remove (const SString& section);

protected:
  SSectionHashtable sectHashtable;
  SFile file;
};

#endif /* SConfig_h */
