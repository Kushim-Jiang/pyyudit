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
 
#ifndef SB_NCR_h
#define SB_NCR_h

#include "stoolkit/STypes.h"
#include "stoolkit/SString.h"
#include "stoolkit/SStringVector.h"
#include "stoolkit/sencoder/SBEncoder.h"

/**
 * This is a NCR converter to convert &#dddd; and &#xhhhh; sequences.
 * This can also read utf-8 embedded format.
 * Used Java encoder/decoder as a base.
 * @author: Miikka-Markus Alhonen <Miikka-Markus.Alhonen@tigatieto.com>
 * @version: 2002-02-26
 */
class SB_NCR : SBEncoder
{
public:
  SB_NCR (void);
  virtual ~SB_NCR ();

  virtual const SString& encode (const SV_UCS4& input);
  virtual const SV_UCS4& decode (const SString& input);

  virtual const SStringVector& delimiters ();
  virtual const SStringVector& delimiters (const SString& sample);
};

#endif /* SB_NCR_h */
