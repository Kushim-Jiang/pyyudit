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
 
#ifndef SB_UTF8_h
#define SB_UTF8_h

#include "stoolkit/STypes.h"
#include "stoolkit/SString.h"
#include "stoolkit/SStringVector.h"
#include "stoolkit/sencoder/SBEncoder.h"

/**
 * This is the 8 bit unicode tarnsformation format. This format is
 * the only one that can encode 32 bit unicode.
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-05-12
 */
class SB_UTF8 : SBEncoder
{
public:
  SB_UTF8 (bool surrogate);
  virtual ~SB_UTF8 ();

  virtual const SString& encode (const SV_UCS4& input);
  virtual const SV_UCS4& decode (const SString& input);

  virtual const SStringVector& delimiters ();
  virtual const SStringVector& delimiters (const SString& sample);
private:
  bool surrogate;
};

#endif /* SB_UTF8_h */
