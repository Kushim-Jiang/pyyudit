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
 
#include "stoolkit/sencoder/SB_ISO2022_JP.h"
#include "stoolkit/SString.h"
#include "stoolkit/SStringVector.h"

#define SS_ESC 27

/**
 * This is a sample (base) implementation of the core encoding class
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-05-12
 * E2 80 A8 E2 80 A9 are paragraph and line seps in utf-8 (U+20A8, U+20A9) 
 */
SB_ISO2022_JP::SB_ISO2022_JP() : SBEncoder ("\n,\r\n,\r"), jis0201 ("jis-0201"), jis0208 ("jis-0208"), jis0212("jis-0212")
{
  ok = jis0201.isOK() && jis0208.isOK() && jis0212.isOK();
}

SB_ISO2022_JP::~SB_ISO2022_JP ()
{
}

/**
 * return false if this generic encoder does not exist.
 */
bool
SB_ISO2022_JP::isOK() const
{
  return ok;
}

/**
 * This is encoding a unicode string into a bytestring
 * @param input is a unicode string.
 */
const SString&
SB_ISO2022_JP::encode (const SV_UCS4& input)
{
  const SS_UCS4* in = input.array();
  sstring.clear();
  sstring.ensure(input.size()*2);
  SUniMap* current=0;
  bool katakana = false;
  SS_UCS2   got;

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
    if (jis0201.isOK() && (got=jis0201.encode (in[i])) != 0)
    {
      if (got > 0xa0 && got < 0xff) 
      {
        if ((current != &jis0201 || !katakana) /*&& current != 0*/)
        {
          sstring.append ((char) SS_ESC);
          sstring.append ((char) '(');
          sstring.append ((char) 'I');
          current = &jis0201;
        }
        current = &jis0201;
        katakana=true;
        sstring.append ((char) (got&0x7f));
        continue;
      }
      if (got < 0x7f)
      {
        // Roman
        if ((current != &jis0201 || !katakana)/* && current != 0*/)
        {
          sstring.append ((char) SS_ESC);
          sstring.append ((char) '(');
          sstring.append ((char) 'J');
          current = &jis0201;
        }
        katakana=false;
        sstring.append ((char) got);
        continue;
      }
    }
    if (jis0208.isOK() && (got=jis0208.encode (in[i])) != 0)
    {
      if ((got&0xff00) > 0x2000 && (got&0xff00) < 0x8f00
        && (got&0xff) > 0x20 && (got&0xff) < 0x8f) 
      {
        if (current != &jis0208 /*&& current != 0*/)
        {
          sstring.append ((char) SS_ESC);
          sstring.append ((char) '$');
          sstring.append ((char) 'B');
          current = &jis0208;
        }
        sstring.append ((char) ((got&0xff00)>>8));
        sstring.append ((char) (got&0xff));
        continue;
      }
    }
    if (jis0212.isOK() && (got=jis0212.encode (in[i])) != 0)
    {
      if ((got&0xff00) > 0x2000 && (got&0xff00) < 0x8f00
        && (got&0xff) > 0x20 && (got&0xff) < 0x8f) 
      {
        if (current != &jis0212 /* && current != 0*/)
        {
          sstring.append ((char) SS_ESC);
          sstring.append ((char) '$');
          sstring.append ((char) '(');
          sstring.append ((char) 'D');
          current = &jis0212;
        }
        sstring.append ((char) ((got&0xff00)>>8));
        sstring.append ((char) (got&0xff));
        continue;
      }
    }
    if (current != 0)
    {
      sstring.append ((char) SS_ESC);
      sstring.append ((char) '(');
      sstring.append ((char) 'B');
      current = 0;
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
SB_ISO2022_JP::decode (const SString& input)
{
  const unsigned char* in = (unsigned char*) input.array();
  SUniMap* current=0;
  bool katakana = false;
  ucs4string.clear();
  ucs4string.ensure(input.size());
  SS_UCS4   got;

  for (unsigned i=0; i<input.size(); i++) 
  {
    if (input.size() > i+2 && in[i]==SS_ESC && in[i+1] == '$' && in[i+2] == '@')
    {
      current=&jis0208; // JIS C 6226-1978
      i++; i++;
      continue;
    } 
    if (input.size() > i+2 && in[i]==SS_ESC && in[i+1] == '$' && in[i+2] == 'B')
    {
      current=&jis0208; // JIS X 0X208-1983
      i++; i++;
      continue;
    } 
    if (input.size() > i+5 && in[i]==SS_ESC && in[i+1] == '&' 
      && in[i+2] == '@' && in[i+3]==SS_ESC && in[i+4] == '$' && in[i+5] == 'B')
    {
      current=&jis0208; // JIS X 0X208-1990
      i++; i++; i++; i++; i++;
      continue;
    } 
    if (input.size() > i+2 && in[i]==SS_ESC && in[i+1] == '(' && in[i+2] == 'J')
    {
      current=&jis0201;  // JIS Roman
      katakana=false;
      i++; i++;
      continue;
    } 
    if (input.size() > i+2 && in[i]==SS_ESC && in[i+1] == '(' && in[i+2] == 'H')
    {
      current=&jis0201; // JIS Roman - old bad escape
      katakana=false;
      i++; i++;
      continue;
    }
    if (input.size()  > i+2 && in[i]==SS_ESC && in[i+1] == '(' && in[i+2] == 'I')
    {
      current=&jis0201; // Half width katakana
      katakana=true;
      i++; i++;
      continue;
    }
    if (input.size() > i+2 && in[i]==SS_ESC && in[i+1] == '(' && in[i+2] == 'B')
    {
      current=0; // ASCII
      i++; i++;
      continue;
    }
    if (input.size() > i+3 && in[i]==SS_ESC && in[i+1] == '$' 
      && in[i+2] == '(' && in[i+3] == 'D')
    {
      current=&jis0212; // JIS X 0212-1990
      i++; i++; i++;
      continue;
    }

    // It should not happen but it does.
    if (in[i] < ' ') current=0;

    if (current) 
    {
      if (current==&jis0201)
      {
        // Tyr katakana first, then jis roman.
        got=(katakana) ? current->decode ((SS_UCS2)(in[i]|0x80))
            :  current->decode ((SS_UCS2)(in[i]));
      }
      else
      {
        if (input.size()-i > 1)
        {
          got = current->decode ((SS_UCS2)in[i] << 8 | in[i+1]);
        }
        else
        {
          got = 0;
        }
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
      if (current!=&jis0201) i++;
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
SB_ISO2022_JP::delimiters ()
{
  return realDelimiters;
}

/**
 * These methods guess the line delimiters for the input
 * The one without arguments is giving the 'first approximation'
 * It returns an exact list
 */
const SStringVector&
SB_ISO2022_JP::delimiters (const SString& sample)
{
  return sampleDelimiters;
}
