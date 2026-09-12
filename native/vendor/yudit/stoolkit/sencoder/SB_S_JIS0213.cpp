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
 
#include "stoolkit/sencoder/SB_S_JIS0213.h"
#include "stoolkit/SString.h"
#include "stoolkit/SStringVector.h"
#include "stoolkit/SCluster.h"

/**
 * This is a sample (base) implementation of the core encoding class
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-05-12
 * E2 80 A8 E2 80 A9 are paragraph and line seps in utf-8 (U+20A8, U+20A9) 
 */
SB_S_JIS0213::SB_S_JIS0213() : SBEncoder ("\n,\r\n,\r"), 
  sjis0213 ("shift-jis-3")
{
  ok = sjis0213.isOK();
}

SB_S_JIS0213::~SB_S_JIS0213 ()
{
}

/**
 * return false if this generic encoder does not exist.
 */
bool
SB_S_JIS0213::isOK() const
{
  return ok;
}

/**
 * This is encoding a unicode string into a bytestring
 * @param input is a unicode string.
 */
const SString&
SB_S_JIS0213::encode (const SV_UCS4& input)
{
  const SS_UCS4* in = input.array();
  sstring.clear();
  sstring.ensure(input.size()*2);
  unsigned char c0;
  unsigned char c1;

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

    if (!sjis0213.isOK())
    {
      quoteString (in[i]);
      continue;
    }
    /* lift */
    SV_UCS4 decd;
    SV_UCS4 enc;
    enc.append (in[i]);
    if (i+1 < input.size()) enc.append (in[i+1]);
    if (i+2 < input.size()) enc.append (in[i+2]);
    unsigned int lifted = sjis0213.lift (enc, 0, false, &decd);
    if (lifted == 0 || decd.size() == 0 || decd[0] == 0)
    {
      quoteString (in[i]);
      continue;
    }
    c0 = (decd[0]&0xff);
    c1 = ((decd[0]>>8)&0xff);
    if (c1 == 0) 
    {
      sstring.append ((char)(c0));
    } else {
      sstring.append ((char)(c1));
      sstring.append ((char)(c0));
    }
    i = i+lifted-1;
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
SB_S_JIS0213::decode (const SString& input)
{
  const unsigned char* in = (unsigned char*) input.array();
  ucs4string.clear();
  ucs4string.ensure(input.size());

  for (unsigned i=0; i<input.size(); i++) 
  {
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
    if (sjis0213.isOK())
    {
      /* lift */
      SV_UCS4 ucs4; 
      SV_UCS4 decd; 
      SS_UCS4 ch = (SS_UCS4)in[i]; 
      ucs4.append (ch);
      unsigned int lifted = sjis0213.lift (ucs4, 0, true, &decd);
      if (lifted != 0 && decd.size() != 0 && decd[0] != 0)
      {
        expandYuditLigatures (&decd);
        ucs4string.append (decd);
        continue;
      }
      /* try two  bytes */
      if (i+1 < input.size())
      { 
         ucs4.clear();
         ch = ch << 8;
         ch += (SS_UCS4)in[i+1];
         ucs4.append (ch);
         lifted = sjis0213.lift (ucs4, 0, true, &decd);
         if (lifted != 0 && decd.size() != 0 && decd[0] != 0)
         {
           expandYuditLigatures (&decd);
           ucs4string.append (decd);
           i++;
           continue;
         }
      }
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
SB_S_JIS0213::delimiters ()
{
  return realDelimiters;
}

/**
 * These methods guess the line delimiters for the input
 * The one without arguments is giving the 'first approximation'
 * It returns an exact list
 */
const SStringVector&
SB_S_JIS0213::delimiters (const SString& sample)
{
  return sampleDelimiters;
}
