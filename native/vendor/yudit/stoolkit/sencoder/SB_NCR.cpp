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
 
#include "stoolkit/sencoder/SB_NCR.h"
#include "stoolkit/SString.h"
#include "stoolkit/SStringVector.h"
#include <stdlib.h>

/**
 * This is a sample (base) implementation of the core encoding class
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-05-12
 * E2 80 A8 E2 80 A9 are paragraph and line seps in utf-8 (U+20A8, U+20A9) 
 */
SB_NCR::SB_NCR() : SBEncoder ("\n,\r\n,\r,\342\200\250,\342\200\251")
{
}

SB_NCR::~SB_NCR ()
{
}

/**
 * This is encoding a unicode string into a bytestring
 * @param input is a unicode string.
 */
const SString&
SB_NCR::encode (const SV_UCS4& input)
{
  const SS_UCS4* in = input.array();
  sstring.clear();
  sstring.ensure(input.size()*2);
  int j; SS_UCS4 value; bool first;
  for (unsigned int i=0; i<input.size(); i++)
  {
    if (in[i]<0x80)
    {
      sstring.append ((char)in[i]);
      continue;
    }
    sstring.append((char) '&');
    sstring.append((char) '#');
    first = true;
    value = in[i];
    if ((j = value / 1000000000))
    {
       sstring.append((char)(j + '0'));
       first = false;
       value %= 1000000000;
    }
    if ((j = value / 100000000) || !first)
    {
       sstring.append((char)(j + '0'));
       first = false;
       value %= 100000000;
    }
    if ((j = value / 10000000) || !first)
    {
       sstring.append((char)(j + '0'));
       first = false;
       value %= 10000000;
    }
    if ((j = value / 1000000) || !first)
    {
       sstring.append((char)(j + '0'));
       first = false;
       value %= 1000000;
    }
    if ((j = value / 100000) || !first)
    {
       sstring.append((char)(j + '0'));
       first = false;
       value %= 100000;
    }
    if ((j = value / 10000) || !first)
    {
       sstring.append((char)(j + '0'));
       first = false;
       value %= 10000;
    }
    if ((j = value / 1000) || !first)
    {
       sstring.append((char)(j + '0'));
       first = false;
       value %= 1000;
    }
    if ((j = value / 100) || !first)
    {
       sstring.append((char)(j + '0'));
       first = false;
       value %= 100;
    }
    if ((j = value / 10) || !first)
    {
       sstring.append((char)(j + '0'));
       value %= 10;
    }
    sstring.append((char)(value + '0'));
    sstring.append((char) ';');
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
SB_NCR::decode (const SString& input)
{
  const unsigned char* in = (unsigned char*) input.array();
  ucs4string.clear();
  ucs4string.ensure(input.size());
  SS_UCS4   decoded;
  char*     next;

  for (unsigned i=0; i<input.size(); i++) 
  {
    if (input.size() > i+4 && in[i] == '&' && in[i+1] == '#' && in[i+2] == 'x')
    {
      SString nin ((const char*)&in[i+3], 9);
      nin.append ((char) 0);
      decoded = (SS_UCS4)  strtoul (nin.array(),  &next, 16);
      // success we append even zeros
      if (*next == ';') 
      {
        ucs4string.append (decoded);
        i += next - nin.array() + 3;
        continue;
      }
    }
    else if (input.size() > i+4 && in[i] == '&' && in[i+1] == '#')
    {
      SString nin ((const char*)&in[i+2], 11);
      nin.append ((char) 0);
      decoded = (SS_UCS4)  strtoul (nin.array(),  &next, 10);
      // success we append even zeros
      if (*next == ';') 
      {
        ucs4string.append (decoded);
        i += next - nin.array() + 2;
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
SB_NCR::delimiters ()
{
  return realDelimiters;
}

/**
 * These methods guess the line delimiters for the input
 * The one without arguments is giving the 'first approximation'
 * It returns an exact list
 */
const SStringVector&
SB_NCR::delimiters (const SString& sample)
{
  return sampleDelimiters;
}
