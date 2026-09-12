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
 
#ifndef SB_Generic_h
#define SB_Generic_h

#include "stoolkit/SUniMap.h"
#include "stoolkit/STypes.h"
#include "stoolkit/SString.h"
#include "stoolkit/SStringVector.h"
#include "stoolkit/sencoder/SBEncoder.h"

/**
 * This is a generic converter based upon externap 'my' maps.
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-05-12
 */
class SB_Generic : SBEncoder
{
public:
  SB_Generic (const SString& name);
  virtual ~SB_Generic ();

  virtual const SString& encode (const SV_UCS4& input);
  virtual const SV_UCS4& decode (const SString& input);
  virtual unsigned int getDecoderMap (SStringVector* key, SStringVector* value,
        unsigned int size);

  virtual const SStringVector& delimiters ();
  virtual const SStringVector& delimiters (const SString& sample);
  virtual SString preEditBuffer() const;
  virtual SV_UCS4 postEditBuffer () const;
  virtual void clear();

  bool isOK() const;

protected:
  SUniMap   map;
  bool      clustered; /* cluster */
  bool      ok;
};

#endif /* SB_Generic_h */
