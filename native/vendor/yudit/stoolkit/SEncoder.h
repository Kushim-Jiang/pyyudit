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
 
#ifndef SEncoder_h
#define SEncoder_h

#include "stoolkit/STypes.h"
#include "stoolkit/SString.h"
#include "stoolkit/SStringVector.h"


class SEncoder : SObject 
{
public:
  SEncoder (void);
  SEncoder (const SString& name);
  SEncoder (const SEncoder& c);
  SEncoder& operator = (const SEncoder& c);
  virtual ~SEncoder ();

  const SString& encode (const SV_UCS4& input);
  const SV_UCS4& decode (const SString& input, bool more=true);
  unsigned int getDecoderMap (SStringVector* key, SStringVector* value,
        unsigned int size);

  /* These methods guess the line delimiters for the input */
  const SStringVector& delimiters ();
  const SString& getName() const;

  virtual SObject* clone() const;
  bool isOK() const;
  void      clear();
  static const SStringVector& builtin();
  static SStringVector external();

  /* return the string that still waits for match */
  SString preEditBuffer() const; /* for non-clustering it is remainder */
  SV_UCS4 postEditBuffer () const; /* for clustering */
  
protected:
  bool      ok;
  void      load ();
  SString   name;
  SString       buffer;
  SStringVector   delim;
  SV_UCS4       retUCS4;
  SV_UCS4       remaining;
  void*         delegate;
};

#endif /* SEncoder_h */
