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
 
#ifndef SProperties_h
#define SProperties_h

#include "SHashtable.h"
#include "SString.h"
#include "SStringVector.h"

/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 */


/**
 * We make the SBinHashtable private so that nobody can screw with 
 * The autodelete.
 */
class SProperties : public SHashtable<SString>
{
public:
  SProperties (void);
  SProperties (const SString& string);
  SProperties (const SProperties& base);
  virtual ~SProperties  ();
  virtual SObject* clone() const;
  SString& split (const SString& s, const SString& delimiters);
  void merge (const SProperties& other);
  SString toString() const;
  SString getProperty (const SString& prop, const SString& fallback) const;
};


// Debug
void test_SProperties();

#endif /* SProperties _h*/
