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
 
#include "stoolkit/sencoder/SB_X11_KSC.h"
#include "stoolkit/SString.h"
#include "stoolkit/SStringVector.h"

#define SS_ESC 27

/**
 * This is a sample (base) implementation of the core encoding class
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-05-12
 * E2 80 A8 E2 80 A9 are paragraph and line seps in utf-8 (U+20A8, U+20A9) 
 */
SB_X11_KSC::SB_X11_KSC() : SBEncoder ("\n,\r\n,\r"), ksc_5601_r ("ksc-5601-r")
{
  ok = ksc_5601_r.isOK();
}

SB_X11_KSC::~SB_X11_KSC ()
{
}

/**
 * return false if this generic encoder does not exist.
 */
bool
SB_X11_KSC::isOK() const
{
  return ok;
}

/**
 * This is encoding a unicode string into a bytestring
 * @param input is a unicode string.
 */
const SString&
SB_X11_KSC::encode (const SV_UCS4& input)
{
  const SS_UCS4* in = input.array();
  sstring.clear();
  sstring.ensure(input.size()*2);
  SS_UCS2   got;
  SUniMap*  current=0;

  for (unsigned int i=0; i<input.size(); i++)
  {
    if (in[i] < 0x80)
    {
      if (current != 0)
      {
        sstring.append ((char) SS_ESC);
        sstring.append ((char) '(');
        sstring.append ((char) 'B');
      }
      current=0;
      sstring.append ((char) in[i]);
      continue;
    }

    if (ksc_5601_r.isOK() && (got=ksc_5601_r.encode (in[i])) != 0)
    {
      if ((got&0xff00) > 0xa000 && (got&0xff00) < 0xff00
        && (got&0xff) > 0xa0 && (got&0xff) < 0xff) 
      {
        if (current==0)
        {
          sstring.append ((char) SS_ESC);
          sstring.append ((char) '$');
          sstring.append ((char) '(');
          sstring.append ((char) 'C');
        }
        current=&ksc_5601_r;
        sstring.append ((char) ((got&0x7f00)>>8));
        sstring.append ((char) (got&0x7f));
        continue;
      }
    }
    quoteString (in[i]);
  }
  if (current != 0)
  {
    sstring.append ((char) SS_ESC);
    sstring.append ((char) '(');
    sstring.append ((char) 'B');
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
SB_X11_KSC::decode (const SString& input)
{
  const unsigned char* in = (unsigned char*) input.array();
  ucs4string.clear();
  ucs4string.ensure(input.size());
  SS_UCS4   got;
  SUniMap*  current=0;

  for (unsigned i=0; i<input.size(); i++) 
  {
    if (input.size() > i+3 && in[i] == SS_ESC 
      && in[i+1] == '$' 
      && in[i+2] == '(' 
      && in[i+3] == 'C')
    {
      current=&ksc_5601_r;  // Korean
      i++; i++; i++;
      continue;
    } 
    if (input.size() > i+2 && in[i] == SS_ESC && in[i+1] == '(' 
      && in[i+2] == 'B')
    {
      current=0; // ASCII
      i++; i++;
      continue;
    }
    if (input.size() > i+3 && in[i] == SS_ESC && in[i+1] == '(' 
      && in[i+2] == 'A' && in[i+3] > 0x20 && in[i+3] <0x7f)
    {
      current=0; // 1 byte ASCII
      i++; i++; i++;
      ucs4string.append ((SS_UCS4) in[i]);
      continue;
    }

    // It should not happen but it does.
    if (in[i] < ' ') current=0;

    if (current!=0) 
    {
      if ( input.size() > i+1 && in[i]>0x20 && in[i] < 0x7F
        && in[i+1] > 0x20 && in[i+1] < 0x7F)
      {
        got = current->decode (((((SS_UCS2)in[i]<< 8) | in[i+1]) | 0x8080 )) ;
      }
      else
      {
        got = 0;
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
    continue;
  }
  return ucs4string;
}


/**
 * These methods guess the line delimiters for the input
 * The one without arguments is giving the 'first approximation'
 * It returns an inclusive list of all possibilities.
 */
const SStringVector&
SB_X11_KSC::delimiters ()
{
  return realDelimiters;
}

/**
 * These methods guess the line delimiters for the input
 * The one without arguments is giving the 'first approximation'
 * It returns an exact list
 */
const SStringVector&
SB_X11_KSC::delimiters (const SString& sample)
{
  return sampleDelimiters;
}
