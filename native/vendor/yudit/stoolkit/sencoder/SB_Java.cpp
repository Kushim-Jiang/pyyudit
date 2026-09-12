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
 
#include "stoolkit/sencoder/SB_Java.h"
#include "stoolkit/SString.h"
#include "stoolkit/SStringVector.h"
#include <stdlib.h>

/**
 * This is a sample (base) implementation of the core encoding class
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-05-12
 * E2 80 A8 E2 80 A9 are paragraph and line seps in utf-8 (U+20A8, U+20A9) 
 */
SB_Java::SB_Java(bool _surrogate) : SBEncoder ("\n,\r\n,\r,\342\200\250,\342\200\251")
{
  surrogate = _surrogate;
}

SB_Java::~SB_Java ()
{
}

/**
 * This is encoding a unicode string into a bytestring
 * @param input is a unicode string.
 */
const SString&
SB_Java::encode (const SV_UCS4& input)
{
  const SS_UCS4* in = input.array();
  sstring.clear();
  sstring.ensure(input.size()*2);
  for (unsigned int i=0; i<input.size(); i++)
  {
    if (in[i]<0x80)
    {
      sstring.append ((char)in[i]);
      continue;
    }
    SS_UCS4 c0 = in[i];
    if (!surrogate && c0 >= 0x10000 && c0 <= 0x10ffff)
    {
      /* quote as surrogtes */
      c0 = c0 - 0x10000;
      quoteString(((c0>>10) & 0x3ff) + 0xd800);
      quoteString((c0 & 0x3ff) + 0xdc00);
      continue;
    }
    quoteString(c0);
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
SB_Java::decode (const SString& input)
{
  const unsigned char* in = (unsigned char*) input.array();
  ucs4string.clear();
  ucs4string.ensure(input.size());
  SS_UCS4   decoded;
  char*     next;

  for (unsigned i=0; i<input.size(); i++) 
  {
    if (input.size() > i+5 && in[i] == '\\' && in[i+1] == 'u')
    {
      SString nin ((const char*)&in[i+2], 4);
      nin.append ((char) 0);
      decoded = (SS_UCS4)  strtoul (nin.array(),  &next, 16);
      // success we append even zeros
      if (nin.array() + 4 == next) 
      {
        if (surrogate || decoded < 0xd800 || decoded > 0xdfff)
        {
          ucs4string.append (decoded);
          i += 5;
          continue;
        }
        /* lower surrogates came first */
        if (decoded >= 0xdc00 || i + 6 + 5 >= input.size() ||  in[i+6] != '\\' || in[i+7] !=  'u')
        {
          quoteUCS4 ((SS_UCS2)decoded);
          i += 5;
          continue;
        }
        i += 6;
        SString ninl ((const char*)&in[i+2], 4);
        ninl.append ((char) 0);
        SS_UCS4 decodedl = (SS_UCS4)  strtoul (ninl.array(),  &next, 16);
        // success we append even zeros
        if (ninl.array() + 4 != next || decodedl < 0xdc00 || decodedl > 0xdfff) 
        {
          quoteUCS4 ((SS_UCS2)decoded);
          i--; /* increment later */
          continue;
        }
        ucs4string.append (((decoded&0x3ff) << 10) + (decodedl&0x3ff) + 0x10000);
        i += 5; /* increment later */
        continue;
      }
    }
    if (input.size() > i+9 && in[i] == '\\' && in[i+1] == 'U')
    {
      SString nin ((const char*)&in[i+2], 8);
      nin.append ((char) 0);
      decoded = (SS_UCS4)  strtoul (nin.array(),  &next, 16);
      // success we append even zeros
      if (nin.array() + 8 == next) 
      {
        ucs4string.append (decoded);
        i += 9;
        continue;
      }
    }

	// life goes on.. try utf-8

    // Unexpected continuation bytes
    if (in[i] <= 0xbf && in[i] >= 0x80)
    {
      quoteUCS4 (in[i]); continue;
    }

    if ((in[i] & 0xe0) ==0xc0 && input.size()-i > 1 && (in[i+1] & 0xc0)==0x80 )
    {
      // check - the second 
      decoded = (((SS_UCS4)(in[i] & 0x1f)) << 6) | ((SS_UCS4) (in[i+1] & 0x3f));
      if (decoded < 0x80)
      {
        quoteUCS4 ((SS_UCS2)decoded);
      }
      else
      {
        ucs4string.append (decoded);
      }
      i++;
      continue;
    }
    if ((in[i] & 0xf0)==0xe0 && input.size()-i > 2
      && (in[i+1] & 0xc0)==0x80 && (in[i+2] & 0xc0)==0x80)
    {
      decoded = (((unsigned short) (in[i] & 0x0f)) << 12)
          | (((unsigned short) (in[i+1] & 0x3f))<<6)
          | ((unsigned short) (in[i+2] & 0x3f));
      if (decoded < 0x800)
      {
        quoteUCS4 ((SS_UCS2) decoded);
      }
      else
      {
        ucs4string.append (decoded);
      }
      i++;
      i++;
      continue;
    }
    if ((in[i] & 0xf8)==0xf0 && input.size()-i > 3
      && (in[i+1] & 0xc0)==0x80 && (in[i+2] & 0xc0)==0x80 
      && (in[i+3] & 0xc0)==0x80)
    {
      decoded = (((unsigned int) (in[i] & 0x07)) << 18)
        | (((unsigned int) (in[i+1] & 0x3f))<<12)
        | (((unsigned short)(in[i+2] & 0x3f))<<6)
        | ((unsigned short) (in[i+3] &  0x3f));
      if (decoded < 0x10000)
      {
        quoteUCS4 ((SS_UCS4) decoded);
      }
      else
      {
        ucs4string.append (decoded);
      }
      i++;
      i++;
      i++;
      continue;
    }
    if ((in[i] & 0xfc)==0xf8 && input.size()-i > 4
      && (in[i+1] & 0xc0)==0x80 && (in[i+2] & 0xc0)==0x80 
      && (in[i+3] & 0xc0)==0x80 && (in[i+4] & 0xc0)==0x80)
    {
      decoded = (((unsigned int) (in[i] & 0x03)) << 24)
        | (((unsigned int) (in[i+1] & 0x3f)) << 18)
        | (((unsigned int) (in[i+2] & 0x3f))<<12)
        | (((unsigned short) (in[i+3] & 0x3f))<<6)
        | ((unsigned short) (in[i+4] & 0x3f));
      if (decoded < 0x200000)
      {
        quoteUCS4 ((SS_UCS4) decoded);
      }
      else
      {
        ucs4string.append (decoded);
      }
      i++;
      i++;
      i++;
      i++;
      continue;
    }
    if ((in[i] & 0xfe)==0xfc && input.size()-i > 5
      && (in[i+1] & 0xc0)==0x80 && (in[i+2] & 0xc0)==0x80 
      && (in[i+3] & 0xc0)==0x80 && (in[i+4] & 0xc0)==0x80
      && (in[i+5] & 0xc0)==0x80)
    {
      decoded =  (((unsigned int) (in[i] & 0x01)) << 30)
        | (((unsigned int) (in[i+1] & 0x3f)) << 24)
        | (((unsigned int) (in[i+2] & 0x3f)) << 18)
        | (((unsigned int) (in[i+3] & 0x3f))<<12)
        | (((unsigned short)(in[i+4] & 0x3f))<<6)
        | ((unsigned short) (in[i+5] &  0x3f));
      if (decoded < 0x4000000)
      {
        quoteUCS4 ((SS_UCS4) decoded);
      }
      else
      {
        ucs4string.append (decoded);
      }
      i++;
      i++;
      i++;
      i++;
      i++;
      continue;
    }

    if (in[i] >= 0x80)
    {
      quoteUCS4 (in[i]);
      continue;
    }
    // we translate broken utf8 into ucs2 also...
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
SB_Java::delimiters ()
{
  return realDelimiters;
}

/**
 * These methods guess the line delimiters for the input
 * The one without arguments is giving the 'first approximation'
 * It returns an exact list
 */
const SStringVector&
SB_Java::delimiters (const SString& sample)
{
  return sampleDelimiters;
}
