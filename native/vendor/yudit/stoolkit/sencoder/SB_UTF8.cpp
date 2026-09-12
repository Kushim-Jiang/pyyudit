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
 
#include "stoolkit/sencoder/SB_UTF8.h"
#include "stoolkit/SString.h"
#include "stoolkit/SStringVector.h"

/**
 * This is a sample (base) implementation of the core encoding class
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-05-12
 * E2 80 A8 E2 80 A9 are paragraph and line seps in utf-8 (U+20A8, U+20A9) 
 */
SB_UTF8::SB_UTF8(bool _surrogate) : SBEncoder ("\n,\r\n,\r,\342\200\250,\342\200\251")
{
  surrogate = _surrogate;
}

SB_UTF8::~SB_UTF8 ()
{
}

/**
 * This is encoding a unicode string into a bytestring
 * @param input is a unicode string.
 */
const SString&
SB_UTF8::encode (const SV_UCS4& input)
{
  const SS_UCS4* in = input.array();
  sstring.clear();
  sstring.ensure (input.size() * 3);
  unsigned int isize = input.size();
  SS_UCS4 ini;
  for (unsigned int i=0; i<isize; i++)
  {
    // The order is important.
    ini = in[i];
    /* if surrogates should not be passed through, convert them.  */
    if (!surrogate && ini >= 0xd800 && ini <= 0xdfff)
    {
      SS_UCS4 hi = ini;
      if (i+1 >= isize)
      { 
        quoteIllegalString (hi);
        continue;
      }
      if (hi < 0xd800 || hi > 0xdbff)
      {
        quoteIllegalString (hi);
        continue;
      } 
      i++;
      SS_UCS4 lo = in[i];
      if (lo < 0xdc00 || lo > 0xdfff)
      {
        quoteIllegalString (lo);
        continue;
      } 
      /* This is th sortest form required by standard */
      ini = ((hi & 0x3ff) << 10) + (lo & 0x3ff) + 0x10000;
    }
    if ((ini & 0x9fffff00) == 0x9fffff00)
    {
      sstring.append ((char) (ini & 0xff));
      continue;
    }
    if (ini >= 0x4000000)
    {
      sstring.append ((char) (0xfc | ((ini >> 30) & 0x3)));
      sstring.append ((char) (0x80 | ((ini >> 24) & 0x3f)));
      sstring.append ((char) (0x80 | ((ini >> 18) & 0x3f)));
      sstring.append ((char) (0x80 | ((ini >> 12) & 0x3f)));
      sstring.append ((char) (0x80 | ((ini >> 6) & 0x3f))); 
      sstring.append ((char) (0x80 | (ini  & 0x3f))); 
      continue;
    }
    if (ini >= 0x200000)
    {
      sstring.append ((char) (0xf8 | ((ini >> 24) & 0x7)));
      sstring.append ((char) (0x80 | ((ini >> 18) & 0x3f)));
      sstring.append ((char) (0x80 | ((ini >> 12) & 0x3f)));
      sstring.append ((char) (0x80 | ((ini >> 6) & 0x3f))); 
      sstring.append ((char) (0x80 | (ini  & 0x3f))); 
      continue;
    }
    if (ini >= 0x10000)
    {
      sstring.append ((char) (0xf0 | (ini >> 18)));
      sstring.append ((char) (0x80 | ((ini >> 12) & 0x3f)));
      sstring.append ((char) (0x80 | ((ini >> 6) & 0x3f))); 
      sstring.append ((char) (0x80 | (ini  & 0x3f))); 
      continue;
    }
    if (ini >= 0x0800)
    {
      sstring.append ((char) (0xe0 | (ini >> 12)));
      sstring.append ((char) (0x80 | ((ini >> 6) & 0x3f))); 
      sstring.append ((char) (0x80 | (ini  & 0x3f))); 
      continue;
    }
    if (ini >= 0x80 && ini <= 0x07ff)
    {
      sstring.append ((char) (0xc0 | (ini >> 6))); 
      sstring.append ((char) (0x80 | (ini  & 0x3f))); 
      continue;
    }
    sstring.append ((char) ini);
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
SB_UTF8::decode (const SString& input)
{
  const unsigned char* in = (unsigned char*) input.array();
  SS_UCS4   decoded;

  unsigned int isize = input.size();
  ucs4string.clear();
  ucs4string.ensure(isize);

  for (unsigned i=0; i<isize; i++) 
  {
    // Unexpected continuation bytes
    if (in[i] <= 0xbf && in[i] >= 0x80)
    {
      quoteIllegalUCS4 (in[i]); continue;
    }

    if ((in[i] & 0xe0) ==0xc0 && isize-i > 1 && (in[i+1] & 0xc0)==0x80 )
    {
      // check - the second 
      decoded = (((SS_UCS4)(in[i] & 0x1f)) << 6) | ((SS_UCS4) (in[i+1] & 0x3f));
      if (decoded < 0x80)
      {
        quoteIllegalUCS4 (in[i]);
        quoteIllegalUCS4 (in[i+1]);
      }
      else
      {
        ucs4string.append (decoded);
      }
      i++;
      continue;
    }
    if ((in[i] & 0xf0)==0xe0 && isize-i > 2
      && (in[i+1] & 0xc0)==0x80 && (in[i+2] & 0xc0)==0x80)
    {
      decoded = (((unsigned short) (in[i] & 0x0f)) << 12)
          | (((unsigned short) (in[i+1] & 0x3f))<<6)
          | ((unsigned short) (in[i+2] & 0x3f));
      if (decoded < 0x800)
      {
        quoteIllegalUCS4 (in[i]);
        quoteIllegalUCS4 (in[i+1]);
        quoteIllegalUCS4 (in[i+2]);
      }
      else if (!surrogate && decoded >= 0xd800 && decoded <= 0xdfff)
      {
        quoteIllegalUCS4 (in[i]);
        quoteIllegalUCS4 (in[i+1]);
        quoteIllegalUCS4 (in[i+2]);
      }
      else
      {
        ucs4string.append (decoded);
      }
      i++;
      i++;
      continue;
    }
    if ((in[i] & 0xf8)==0xf0 && isize-i > 3
      && (in[i+1] & 0xc0)==0x80 && (in[i+2] & 0xc0)==0x80 
      && (in[i+3] & 0xc0)==0x80)
    {
      decoded = (((unsigned int) (in[i] & 0x07)) << 18)
        | (((unsigned int) (in[i+1] & 0x3f))<<12)
        | (((unsigned short)(in[i+2] & 0x3f))<<6)
        | ((unsigned short) (in[i+3] &  0x3f));
      if (decoded < 0x10000)
      {
        quoteIllegalUCS4 (in[i]);
        quoteIllegalUCS4 (in[i+1]);
        quoteIllegalUCS4 (in[i+2]);
        quoteIllegalUCS4 (in[i+3]);
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
    if ((in[i] & 0xfc)==0xf8 && isize-i > 4
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
        quoteIllegalUCS4 (in[i]);
        quoteIllegalUCS4 (in[i+1]);
        quoteIllegalUCS4 (in[i+2]);
        quoteIllegalUCS4 (in[i+3]);
        quoteIllegalUCS4 (in[i+4]);
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
    if ((in[i] & 0xfe)==0xfc && isize-i > 5
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
        quoteIllegalUCS4 (in[i]);
        quoteIllegalUCS4 (in[i+1]);
        quoteIllegalUCS4 (in[i+2]);
        quoteIllegalUCS4 (in[i+3]);
        quoteIllegalUCS4 (in[i+4]);
        quoteIllegalUCS4 (in[i+5]);
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
      quoteIllegalUCS4 (in[i]);
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
SB_UTF8::delimiters ()
{
  return realDelimiters;
}

/**
 * These methods guess the line delimiters for the input
 * The one without arguments is giving the 'first approximation'
 * It returns an exact list
 */
const SStringVector&
SB_UTF8::delimiters (const SString& sample)
{
  return sampleDelimiters;
}
