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
 
#include "stoolkit/sencoder/SB_EUC_JP.h"
#include "stoolkit/SString.h"
#include "stoolkit/SStringVector.h"

/**
 * This is a sample (base) implementation of the core encoding class
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-05-12
 * E2 80 A8 E2 80 A9 are paragraph and line seps in utf-8 (U+20A8, U+20A9) 
 */
SB_EUC_JP::SB_EUC_JP() : SBEncoder ("\n,\r\n,\r"), jis0201 ("jis-0201"), jis0208 ("jis-0208"), jis0212("jis-0212")
{
  ok = jis0201.isOK() && jis0208.isOK() && jis0212.isOK();
}

SB_EUC_JP::~SB_EUC_JP ()
{
}

/**
 * return false if this generic encoder does not exist.
 */
bool
SB_EUC_JP::isOK() const
{
  return ok;
}

/**
 * This is encoding a unicode string into a bytestring
 * @param input is a unicode string.
 */
const SString&
SB_EUC_JP::encode (const SV_UCS4& input)
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
    if (jis0201.isOK() && (got=jis0201.encode (in[i])) != 0)
    {
      if (got > 0xa0 && got < 0xff) 
      {
        sstring.append ((char) 0x8e);
        sstring.append ((char) got);
        continue;
      }
    }
    if (jis0208.isOK() && (got=jis0208.encode (in[i])) != 0)
    {
      got |= 0x8080;
      if ((got&0xff00) > 0xa000 && (got&0xff00) < 0xff00
        && (got&0xff) > 0xa0 && (got&0xff) < 0xff) 
      {
        sstring.append ((char) ((got&0xff00)>>8));
        sstring.append ((char) (got&0xff));
        continue;
      }
    }
    if (jis0212.isOK() && (got=jis0212.encode (in[i])) != 0)
    {
      got |= 0x8080;
      if ((got&0xff00) > 0xa000 && (got&0xff00) < 0xff00
        && (got&0xff) > 0xa0 && (got&0xff) < 0xff) 
      {
        sstring.append ((char) 0x8f);
        sstring.append ((char) ((got&0xff00)>>8));
        sstring.append ((char) (got&0xff));
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
SB_EUC_JP::decode (const SString& input)
{
  const unsigned char* in = (unsigned char*) input.array();
  ucs4string.clear();
  ucs4string.ensure(input.size());
  SS_UCS4   got;

  for (unsigned i=0; i<input.size(); i++) 
  {
    got=0;
    if (input.size() -i > 1 && in[i] == 0x8e && in[i+1] > 0xa0
      && in[i+1] < 0xff)
    {
      // JISC 6226-1978 half width katakana
      if (jis0201.isOK())
      {
        got = jis0201.decode ((SS_UCS2)in[i+1]);
      }
      if (got != 0)
      {
        ucs4string.append (got);
      }
      else
      {
        quoteUCS4 (in[i]);
        quoteUCS4 (in[i+1]);
      }
      i++;
      continue;
    } 
    if (input.size() -i > 1 && in[i] > 0xa0 && in[i] < 0xff
      && in[i+1] > 0xa0 && in[i+1] < 0xff)
    {
      // JIS X 0X208-1990
      if (jis0208.isOK())
      {
        got = jis0208.decode (((SS_UCS2)(in[i]&0x7f) << 8) | (in[i+1]&0x7f) );
      }
      if (got != 0)
      {
        ucs4string.append (got);
      }
      else
      {
        quoteUCS4 (in[i]);
        quoteUCS4 (in[i+1]);
      }
      i++;
      continue;
    } 
    if (input.size() -i > 2 && in[i] == 0x8f 
      && in[i+1] > 0xa0 && in[i+1] < 0xff
      && in[i+2] > 0xa0 && in[i+2] < 0xff)
    {
      // JIS X 0212-1990
      if (jis0212.isOK())
      {
        got = jis0212.decode (((SS_UCS2)(in[i+1]&0x7f) << 8) | (in[i+2]&0x7f) );
      }
      if (got != 0)
      {
        ucs4string.append (got);
      }
      else
      {
        quoteUCS4 (in[i]);
        quoteUCS4 (in[i+1]);
        quoteUCS4 (in[i+2]);
      }
      i++; i++;
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
SB_EUC_JP::delimiters ()
{
  return realDelimiters;
}

/**
 * These methods guess the line delimiters for the input
 * The one without arguments is giving the 'first approximation'
 * It returns an exact list
 */
const SStringVector&
SB_EUC_JP::delimiters (const SString& sample)
{
  return sampleDelimiters;
}
