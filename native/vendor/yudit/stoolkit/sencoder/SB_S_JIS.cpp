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

/*
    [first] [second]
    [0x00-0x7F]  [0xA1-0xDF] ->JIS X0201 same code single byte
    [0x81-0x9F or 0xE0-0xEF]  [0x40-0x7E or 0x80-0xFC] JIS X0208 2 bytes
    [0xF0-0xFC]  [0x40-0x7E or 0x80-0xFC] 2444 user-defined characters
*/
 
#include "stoolkit/sencoder/SB_S_JIS.h"
#include "stoolkit/SString.h"
#include "stoolkit/SStringVector.h"

/**
 * This is a sample (base) implementation of the core encoding class
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-05-12
 * E2 80 A8 E2 80 A9 are paragraph and line seps in utf-8 (U+20A8, U+20A9) 
 */
SB_S_JIS::SB_S_JIS() : SBEncoder ("\n,\r\n,\r"), jis0201 ("jis-0201"), jis0208 ("jis-0208")
{
  ok = jis0201.isOK() && jis0208.isOK();
}

SB_S_JIS::~SB_S_JIS ()
{
}

/**
 * return false if this generic encoder does not exist.
 */
bool
SB_S_JIS::isOK() const
{
  return ok;
}

/**
 * This is encoding a unicode string into a bytestring
 * @param input is a unicode string.
 */
const SString&
SB_S_JIS::encode (const SV_UCS4& input)
{
  const SS_UCS4* in = input.array();
  sstring.clear();
  sstring.ensure(input.size()*2);
  SS_UCS2   got;
  unsigned short rowOffset;
  unsigned short cellOffset;
  unsigned char c1;
  unsigned char c2;

  for (unsigned int i=0; i<input.size(); i++)
  {
    if (in[i] < 0x80)
    {
      if (in[i] == '\\')
      {  
        sstring.append ((char)0x80);
      }
      else
      {
        sstring.append ((char) in[i]);
      }
      continue;
    }
    // half-width yen
    if (in[i] == 0x00a5)
    {
      sstring.append ('\\');
      continue;
    }

    // JIS
    if (jis0208.isOK() && (got=jis0208.encode (in[i])) != 0)
    {
      c1 = got >> 8;
      c2 = got & 0xff;
   
      rowOffset =  (c1 < 95) ? 112 : 176;
      cellOffset = c1 % 2 ? (c2 > 95 ? 32 : 31 ) : 126;
      c1 = ((c1+1) >> 1) + rowOffset;
      c2 = c2 + cellOffset;
      if ( ((c1 >= 0x81 && c1 <= 0x9f) || (c1 >= 0xe0 &&  c1 <= 0xef))
       && ((c2 >= 0x40 && c2 <= 0x7e) || (c2 >= 0x80 && c2 <= 0xfc)) )
      {
        sstring.append ((char)c1);
        sstring.append ((char)c2);
        continue;
      }
    }

    if (jis0201.isOK() && (got=jis0201.encode (in[i])) != 0)
    {
      // Half-width katakana
      if (got > 0xa0 && got < 0xff) 
      {
        sstring.append ((char) got);
        continue;
      }
      // Roman
      if (got < 0x80)
      {
        sstring.append ((char) got);
        continue;
      }
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
SB_S_JIS::decode (const SString& input)
{
  const unsigned char* in = (unsigned char*) input.array();
  ucs4string.clear();
  ucs4string.ensure(input.size());
  SS_UCS4   got;
  SS_UCS4   code;
  unsigned short adjust;
  unsigned short rowOffset;
  unsigned short cellOffset;

  for (unsigned i=0; i<input.size(); i++) 
  {
    // Half-width katakana
    if (in[i] >= 0xa1 && in[i] <= 0xdf)
    {
      code = in[i];
      got = jis0201.decode (code);
      if (got ==0)
      {
        quoteUCS4 (in[i]);
      }
      else
      {
        ucs4string.append (got);
      }
      continue;
    }

    // JIS
    if (input.size() > i+1 && ((in[i] >= 0x81 && in[i] <= 0x9f) 
        || (in[i] >= 0xe0 && in[i] <= 0xef))
        && ((in[i+1] >= 0x40 && in[i+1] <= 0x7e) 
     || (in[i+1] >= 0x80 && in[i+1] <= 0xfc)) )
   
    {
      adjust = (in[i+1] < 159)? 1 : 0;
      rowOffset = in[i] < 160 ? 112 : 176;
      cellOffset = adjust ? (in[i+1]>127 ? 32 : 31) : 126;

      code = ((((in[i] - rowOffset) << 1) - adjust) << 8) 
        | (in[i+1] - cellOffset);

      got = jis0208.decode (code);
      if (got ==0)
      {
        quoteUCS4 (in[i]);
        quoteUCS4 (in[i+1]);
      }
      else
      {
        ucs4string.append (got);
      }
      i++;
      continue;
    } 

    // User defined area
    // first 0xF0-0xFC, and the second byte in the range 0x40-0x7E or 0x80-0xFC.
    if (input.size() > i+1 && (in[i] >= 0xf0 && in[i] <= 0xfc) 
      && ((in[i+1] >= 0x40 && in[i+1] <= 0x7e) 
          || (in[i+1] >= 0x80 && in[i+1] <= 0xfc)))
    {
      quoteUCS4 (in[i]);
      quoteUCS4 (in[i+1]);
      i++;
      continue;
    }

    // MAC - backslash
    if (in[i] == 0x80)
    {
      ucs4string.append ((SS_UCS4) '\\');
      continue;
    }
    // half width yen
    if (in[i] == '\\')
    {
      ucs4string.append ((SS_UCS4) 0x00a5);
      continue;
    }
    // MAC - copyright
    if (in[i] == 0xfd)
    {
      ucs4string.append ((SS_UCS4) 0xa9);
      continue;
    }
    // MAC - tm
    if (in[i] == 0xfe)
    {
      ucs4string.append ((SS_UCS4) 0x2122);
      continue;
    }
    // MAC - ... horizontal ellipsis
      if (in[i] == 0xff)
    {
      ucs4string.append ((SS_UCS4) 0x2026);
      continue;
    }

    if (in[i] > 0x80)
    {
      quoteUCS4 (in[i]);
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
SB_S_JIS::delimiters ()
{
  return realDelimiters;
}

/**
 * These methods guess the line delimiters for the input
 * The one without arguments is giving the 'first approximation'
 * It returns an exact list
 */
const SStringVector&
SB_S_JIS::delimiters (const SString& sample)
{
  return sampleDelimiters;
}
