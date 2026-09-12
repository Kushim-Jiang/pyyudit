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
 
#ifndef SBEncoder_h
#define SBEncoder_h

#include "stoolkit/STypes.h"
#include "stoolkit/SString.h"
#include "stoolkit/SStringVector.h"

/**
 * This is a sample (base) implementation of the core encoding class
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-05-12
 */
class SBEncoder 
{
public:
  SBEncoder (const SStringVector& delim);
  virtual ~SBEncoder ();

  virtual const SString& encode (const SV_UCS4& input);
  virtual const SV_UCS4& decode (const SString& input);
  virtual unsigned int getDecoderMap (SStringVector* key, SStringVector* value,
        unsigned int size);

  virtual const SStringVector& delimiters ();
  virtual const SStringVector& delimiters (const SString& sample);
  virtual void clear();
  /* return the string that still waits for match */

  virtual SString preEditBuffer() const;
  virtual SV_UCS4 postEditBuffer () const;

protected:
  void quoteUCS4 (unsigned char in);
  void quoteIllegalUCS4 (unsigned char in);
  void quoteUCS4 (SS_UCS2 in);
  void quoteUCS4 (SS_UCS4 in);

  void quoteIllegalString (SS_UCS4 in);

  void quoteString (SS_UCS4 in);
  void quoteStringLE (SS_UCS4 in);
  void quoteStringBE (SS_UCS4 in);
  SStringVector realDelimiters;
  SStringVector sampleDelimiters;
  SString sstring;
  SV_UCS4 ucs4string;
  SV_UCS4 remaining;
};

#endif /* SBEncoder_h */
