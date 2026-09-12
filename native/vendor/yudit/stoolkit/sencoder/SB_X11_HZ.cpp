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
 
#include "stoolkit/sencoder/SB_X11_HZ.h"
#include "stoolkit/SString.h"
#include "stoolkit/SStringVector.h"

#define SS_ESC 27

/**
 * This is a sample (base) implementation of the core encoding class
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-05-12
 * E2 80 A8 E2 80 A9 are paragraph and line seps in utf-8 (U+20A8, U+20A9) 
 */
SB_X11_HZ::SB_X11_HZ() : SBEncoder ("\n,\r\n,\r"), gb_2312_l ("gb-2312-l")
{
  ok = gb_2312_l.isOK();
}

SB_X11_HZ::~SB_X11_HZ ()
{
}

/**
 * return false if this generic encoder does not exist.
 */
bool
SB_X11_HZ::isOK() const
{
  return ok;
}

/**
 * This is encoding a unicode string into a bytestring
 * @param input is a unicode string.
 */
const SString&
SB_X11_HZ::encode (const SV_UCS4& input)
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

  	if (gb_2312_l.isOK() && (got=gb_2312_l.encode ((SS_UCS4)in[i])) != 0)
  	{
  		if ((got&0xff00) > 0x2000 && (got&0xff00) < 0x7f00
  			&& (got&0xff) > 0x20 && (got&0xff) < 0x7f) 
  		{
  			if (current==0)
  			{
  				sstring.append ((char) SS_ESC);
  				sstring.append ((char) '$');
  				sstring.append ((char) '(');
  				sstring.append ((char) 'A');
  			}
  			current=&gb_2312_l;
  			sstring.append ((char) ((got&0x7f00)>>8));
  			sstring.append ((char) (got&0x7f));
  			continue;
  		}
  	}
    quoteString (in[i]);
  }
  // Change it to roman
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
SB_X11_HZ::decode (const SString& input)
{
  const unsigned char* in = (unsigned char*) input.array();
  ucs4string.clear();
  ucs4string.ensure(input.size());
  SS_UCS4   got;
  SUniMap*  current=0;
  // bool      gbRoman=false;

  for (unsigned i=0; i<input.size(); i++) 
  {
    if (input.size() > i+3 && in[i] == SS_ESC 
      && in[i+1] == '$' && in[i+2] == '(' && in[i+3] == 'A')
    {
      current=&gb_2312_l;  // Chinese
      i++; i++; i++;
      //gbRoman=false;
      continue;
    } 
    if (input.size() > i+2 && in[i] == SS_ESC && in[i+1] == '(' 
      && in[i+2] == 'B')
    {
      current=0; // ASCII
      i++; i++;
      //gbRoman=false;
      continue;
    }
    if (input.size() > i+2 && in[i] == SS_ESC && in[i+1] == '(' 
      && in[i+2] == 'T')
    {
      current=&gb_2312_l; // GB Roman
      i++; i++;
      //gbRoman=true;
      continue;
    }

    // It should not happen but it does.
    if (in[i] < ' ') current=0;
    if (current) 
    {
      got = (input.size() > i+1) 
        ?  current->decode (((SS_UCS2)in[i]<< 8) | in[i+1])
        : 0;
      if (got)
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
SB_X11_HZ::delimiters ()
{
  return realDelimiters;
}

/**
 * These methods guess the line delimiters for the input
 * The one without arguments is giving the 'first approximation'
 * It returns an exact list
 */
const SStringVector&
SB_X11_HZ::delimiters (const SString& sample)
{
  return sampleDelimiters;
}
