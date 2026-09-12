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
 
#include "stoolkit/sencoder/SB_HZ.h"
#include "stoolkit/SString.h"
#include "stoolkit/SStringVector.h"

#define SS_ESC 27

/**
 * This is a sample (base) implementation of the core encoding class
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-05-12
 * E2 80 A8 E2 80 A9 are paragraph and line seps in utf-8 (U+20A8, U+20A9) 
 */
SB_HZ::SB_HZ() : SBEncoder ("\n,\r\n,\r"), gb_2312_l ("gb-2312-l")
{
  ok = gb_2312_l.isOK();
}

SB_HZ::~SB_HZ ()
{
}

/**
 * return false if this generic encoder does not exist.
 */
bool
SB_HZ::isOK() const
{
  return ok;
}

/**
 * This is encoding a unicode string into a bytestring
 * @param input is a unicode string.
 */
const SString&
SB_HZ::encode (const SV_UCS4& input)
{
  const SS_UCS4* in = input.array();
  sstring.clear();
  sstring.ensure(input.size()*2);
  SUniMap*  current=0;
  SS_UCS2   got;

  for (unsigned int i=0; i<input.size(); i++)
  {
    if (in[i] < 0x80)
    {
      if (current != 0)
      {
        sstring.append ((char) '~');
        sstring.append ((char) '}');
        current=0;
      }
      sstring.append ((char) in[i]);
      continue;
    }
    if (gb_2312_l.isOK() && (got=gb_2312_l.encode (in[i])) != 0)
    {
      if (current == 0)
      {
        sstring.append ((char) '~');
        sstring.append ((char) '{');
        current=&gb_2312_l;
      }
      if ((got&0x7f00) > 0x2000 && (got&0x7f00) < 0x7f00
        && (got&0x7f) > 0x20 && (got&0x7f) < 0x7f) 
      {
        sstring.append ((char) ((got>>8) & 0x7f));
        sstring.append ((char) ((got&0xff) & 0x7f));
        continue;
      }
      sstring.append ((char) '~');
      sstring.append ((char) '}');
      current=0;
    }
    quoteString (in[i]);
  }
  if (current != 0)
  {
    sstring.append ((char) '~');
    sstring.append ((char) '}');
  }
  return sstring;
}

/**
 * Decode an input string into a unicode string.
 * It may get rid of new-line and merge tow lines!
 * Upper routines should know this.
 * @param input is a string.
 *   he output can be null, in this case a line is not
 *   read fully. If input size is zero output will be flushed.
 */
const SV_UCS4&
SB_HZ::decode (const SString& input)
{
  const unsigned char* in = (unsigned char*) input.array();
  ucs4string.clear();
  ucs4string.ensure(input.size());
  SUniMap*  current=0;
  SS_UCS4   got;

  for (unsigned i=0; i<input.size(); i++) 
  {
    got=0;
    if (current==0 && input.size() > i+1 
      && in[i] == '~' && in[i+1] == '~')
    {
      ucs4string.append ((SS_UCS4) in[i]);
      i++; 
      continue;
    }
    if (input.size() > i+1 && in[i] == '~' && in[i+1] == '\n')
    {
      i++; 
      continue;
    }
    if (current==0 && input.size() > i+1 && in[i] == '~' && in[i+1] == '{')
    {
      current = &gb_2312_l;
      i++;
      continue;
    }
    if (current!=0 && input.size() > i+1 && in[i] == '~' && in[i+1] == '}')
    {
      current = 0;
      i++;
      continue;
    }
  
    if (current!=0 && input.size() > i+1)
    {
      got = current->decode ((SS_UCS2)(in[i] << 8) | in[i+1]);
      if (got != 0)
      {
        ucs4string.append (got);
        i++;
        continue;
      }
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
SB_HZ::delimiters ()
{
  return realDelimiters;
}

/**
 * These methods guess the line delimiters for the input
 * The one without arguments is giving the 'first approximation'
 * It returns an exact list
 */
const SStringVector&
SB_HZ::delimiters (const SString& sample)
{
  return sampleDelimiters;
}
