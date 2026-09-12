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
 
#include "stoolkit/sencoder/SB_UHC.h"
#include "stoolkit/SString.h"
#include "stoolkit/SStringVector.h"

/**
 * Korean text converter (MS-Windows Korean)
 * rewritten code contributed by Jungshik Shin
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-05-12
 */
SB_UHC::SB_UHC() : SBEncoder ("\n,\r\n,\r"), ksc_5601_r ("ksc-5601-r")
{
  ok = ksc_5601_r.isOK();
}

SB_UHC::~SB_UHC ()
{
}

/**
 * return false if this generic encoder does not exist.
 */
bool
SB_UHC::isOK() const
{
  return ok;
}

/**
 * This is encoding a unicode string into a bytestring
 * @param input is a unicode string.
 */
const SString&
SB_UHC::encode (const SV_UCS4& input)
{
  const SS_UCS4* in = input.array();
  sstring.clear();
  sstring.ensure(input.size()*2);
  SS_UCS2   got;

  for (unsigned int i=0; i<input.size(); i++)
  {
    if (in[i] < 0x80)
    {
      sstring.append ((char) in[i]);
      continue;
    }
    if (ksc_5601_r.isOK() && (got=ksc_5601_r.encode (in[i])) != 0)
    {
      sstring.append ((char) (got>>8));
      sstring.append ((char) (got&0xff));
      continue;
    }
    quoteString (in[i]);
  }
  return sstring;
}

/**
 * Decode an input string into a unicode string.
 * @param input is a string.
 *   he output can be null, in this case a line is not
 *   read fully. If input size is zero output will be flushed.
 */
const SV_UCS4&
SB_UHC::decode (const SString& input)
{
  const unsigned char* in = (unsigned char*) input.array();
  ucs4string.clear();
  ucs4string.ensure(input.size());
  SS_UCS4   got;

  for (unsigned i=0; i<input.size(); i++) 
  {
    got=0;
    //
    // patch from Jungshik Shin 9 Nov 1998
    // Modified for better readability Gaspar Sinai 2010-01-24
    //
    if (input.size() > i+1 && (
        (in[i] > 0xa0 && in[i] < 0xff && in[i+1] > 0xa0 && in[i+1] < 0xff)
     || (in[i] > 0x80 && in[i] < 0xc6 && 
          ((in[i+1] > 0x40 && in[i+1] < 0x5b) 
       ||  (in[i+1] > 0x60 && in[i+1] < 0x7b) 
       ||  (in[i+1] > 0x80 && in[i+1] < 0xff))) 
     || (in[i] == 0xc6 && in[i+1] > 0x40 && in[i+1] < 0x53)))

    {
      // KSC5601
      if (ksc_5601_r.isOK())
      {
        got = ksc_5601_r.decode ((SS_UCS2)((in[i] & 0xff) << 8)
          | (in[i+1] & 0xff) );
      }
      if (got != 0)
      {
        ucs4string.append (got);
      }
      else
      {
        quoteUCS4 ((unsigned char) in[i]);
        quoteUCS4 ((unsigned char) in[i+1]);
      }
      i++;
      continue;
    } 
    ucs4string.append ((SS_UCS4) in[i]);
  }
  return ucs4string;
}


/**
 * These methods guess the line delimiters for the input
 * The one without arguments is giving the 'first approximation'
 * It returns an inclusive list of all possibilities.
 */
const SStringVector&
SB_UHC::delimiters ()
{
  return realDelimiters;
}

/**
 * These methods guess the line delimiters for the input
 * The one without arguments is giving the 'first approximation'
 * It returns an exact list
 */
const SStringVector&
SB_UHC::delimiters (const SString& sample)
{
  return sampleDelimiters;
}
