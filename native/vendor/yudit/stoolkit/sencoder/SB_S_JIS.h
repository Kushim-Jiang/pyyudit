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
 
#ifndef SB_S_JIS_h
#define SB_S_JIS_h

#include "stoolkit/SUniMap.h"
#include "stoolkit/STypes.h"
#include "stoolkit/SString.h"
#include "stoolkit/SStringVector.h"
#include "stoolkit/sencoder/SBEncoder.h"

/**
 * This is the shift-jis converter for Japanese. Used mainly in PCs
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-05-12
 */
class SB_S_JIS : SBEncoder
{
public:
  SB_S_JIS ();
  virtual ~SB_S_JIS ();

  virtual const SString& encode (const SV_UCS4& input);
  virtual const SV_UCS4& decode (const SString& input);

  virtual const SStringVector& delimiters ();
  virtual const SStringVector& delimiters (const SString& sample);

  bool isOK() const;

protected:
  SUniMap   jis0201;
  SUniMap   jis0208;
  bool      ok;
};

#endif /* SB_S_JIS_h */
