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
 
#ifndef SB_UCS2_h
#define SB_UCS2_h

#include "stoolkit/SUniMap.h"
#include "stoolkit/STypes.h"
#include "stoolkit/SString.h"
#include "stoolkit/SStringVector.h"
#include "stoolkit/sencoder/SBEncoder.h"

/**
 * Generic UCS2 (16 bit) converter.
 * I have redefined endian-ness.
 *      U+1234 -encode-> 12 34 is big endian.
 *      U+1234 -encode-> 34 12 is little endian.
 * The default is the guess-decode and big-endian encode.
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-05-12
 */
class SB_UCS2 : SBEncoder
{
public:
  enum   SEndian {AUTO_END=0, BIG_END, LITTLE_END };
  SB_UCS2 (void);
  SB_UCS2 (SEndian e, bool surrogate);
  virtual ~SB_UCS2 ();

  virtual const SString& encode (const SV_UCS4& input);
  virtual const SV_UCS4& decode (const SString& input);

  virtual const SStringVector& delimiters ();
  virtual const SStringVector& delimiters (const SString& sample);

  /* These are virtual too */
  bool isOK() const;
  void clear();

private:
  bool    start;
  bool    surrogate;
  SEndian setEndian (const SString& in);
  void    setBigEndian (bool is);
  int     decendian;
  int     encendian;
};

#endif /* SB_UCS2_h */
