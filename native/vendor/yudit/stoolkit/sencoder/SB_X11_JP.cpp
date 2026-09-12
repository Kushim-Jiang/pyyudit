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
 
#include "stoolkit/sencoder/SB_X11_JP.h"
#include "stoolkit/SString.h"
#include "stoolkit/SStringVector.h"

#define SS_ESC 27

/**
 * This is a sample (base) implementation of the core encoding class
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-05-12
 * E2 80 A8 E2 80 A9 are paragraph and line seps in utf-8 (U+20A8, U+20A9) 
 */
SB_X11_JP::SB_X11_JP() : SBEncoder ("\n,\r\n,\r"), jis0201 ("jis-0201"), jis0208 ("jis-0208"), jis0212("jis-0212")
{
  ok = jis0201.isOK() && jis0208.isOK() && jis0212.isOK();
}

SB_X11_JP::~SB_X11_JP ()
{
}

/**
 * return false if this generic encoder does not exist.
 */
bool
SB_X11_JP::isOK() const
{
  return ok;
}

/**
 * This is encoding a unicode string into a bytestring
 * This encoding is also known as X11 Compound Text
 * @param input is a unicode string.
 */
const SString&
SB_X11_JP::encode (const SV_UCS4& input)
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
    if (jis0201.isOK() && (got=jis0201.encode ((SS_UCS4)in[i])) != 0)
    {
      if (got > 0xa0 && got < 0xff) 
      {
        if ((current != &jis0201 || katakana!= 1) /*&& current!=0*/)
        {
          sstring.append ((char) SS_ESC);
          sstring.append ((char) ')');
          sstring.append ((char) 'I');
        }
        current = &jis0201;
        katakana=1;
        sstring.append ((char) got);
        continue;
      }
      if (got < 0x7f)
      {
        // Roman
        if ((current != &jis0201 || katakana!= 1) /*&& current!=0*/)
        {
          sstring.append ((char) SS_ESC);
          sstring.append ((char) '(');
          sstring.append ((char) 'J');
          current = &jis0201;
        }
        katakana=0;
        sstring.append ((char) got);
        continue;
      }
    }
    if (jis0208.isOK() && (got=jis0208.encode ((SS_UCS4)in[i])) != 0)
    {
      if ((got&0xff00) > 0x2000 && (got&0xff00) < 0x8f00
        && (got&0xff) > 0x20 && (got&0xff) < 0x8f) 
      {
        if (current != &jis0208 /*&& current != 0*/)
        {
          sstring.append ((char) SS_ESC);
          sstring.append ((char) '$');
          sstring.append ((char) ')');
          sstring.append ((char) 'B');
          current = &jis0208;
        }
        // GR should have high bit set
        sstring.append ((char) ((got|0x8000)>>8));
        sstring.append ((char) (got|0x80));
      
        continue;
      }
    }
    if (jis0212.isOK() && (got=jis0212.encode ((SS_UCS4)in[i])) != 0)
    {
      if ((got&0xff00) > 0x2000 && (got&0xff00) < 0x8f00
        && (got&0xff) > 0x20 && (got&0xff) < 0x8f) 
      {
        if (current != &jis0212 /*&& current!=0*/)
        {
          sstring.append ((char) SS_ESC);
          sstring.append ((char) '$');
          sstring.append ((char) '(');
          sstring.append ((char) 'D');
          current = &jis0212;
        }
        sstring.append ((char) ((got&0x7f00)>>8));
        sstring.append ((char) (got&0x7f));

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
    quoteString(in[i]);
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
 * This encoding is also known as X11 Compound Text
 * @param input is a string.
 *   he output can be null, in this case a line is not
 *   read fully. If input size is zero output will be flushed.
 */
const SV_UCS4&
SB_X11_JP::decode (const SString& input)
{
  const unsigned char* in = (unsigned char*) input.array();
  SUniMap* current=0;
  bool right = false;
  ucs4string.clear();
  ucs4string.ensure(input.size());
  SS_UCS4   got;

  for (unsigned i=0; i<input.size(); i++) 
  {
    if (input.size() > i+2 && in[i] == SS_ESC && in[i+1] == ')' && in[i+2] == 'I')
    {
      current=&jis0201; // JIS X 0X201-1976 right  -katakana
      right = true;
      i++; i++;
      continue;
    } 
    if (input.size() > i+2 && in[i] == SS_ESC && in[i+1] == '(' 
      && in[i+2] == 'J')
    {
      current=&jis0201; // JIS X 0X201-1976 left half -roman
      right = false;
      i++; i++; 
      continue;
    } 
    if (input.size() > i+3 && in[i] == SS_ESC && in[i+1] == '$' 
      && in[i+2] == ')' && in[i+3] == 'B')
    {
      current=&jis0208; // JIS X 0X208-1990
      right = true;
      i++; i++; i++; 
      continue;
    } 
    if (input.size() > i+3 && in[i] == SS_ESC && in[i+1] == '$' 
      && in[i+2] == '(' && in[i+3] == 'D')
    {
      current=&jis0212; // JIS X JIS0212-1990
      right = false;
      i++; i++; i++; 
      continue;
    } 
    //
    // Kterm has the habit of setting GR instead of GL
    //
    if (input.size() > i+3 && in[i] == SS_ESC && in[i+1] == '$' 
      && in[i+2] == ')' && in[i+3] == 'D')
    {
      current=&jis0212; // JIS X JIS0212-1990
      right = true;
      i++; i++; i++; 
      continue;
    } 
    if (input.size()  > i+2 && in[i] == SS_ESC  && in[i+1] == '(' 
      && in[i+2] == 'B')
    {
      current=0; // ASCII
      right = false;
      i++; i++;
      continue;
    } 
    // G0 and G1 in an 8-bit env
    if (input.size() > i+2 && in[i] == SS_ESC && in[i+1] == ' ' 
      && in[i+2] == 'C')
    {
      i++; i++; 
      continue;
    } 
    // In 8 bit C1 is 8 bits
    if (input.size()  > i+2 && in[i] == SS_ESC && in[i+1] == ' ' 
      && in[i+2] == 'G')
    {
      i++; i++; 
      continue;
    } 
    // In 8 bit C1 is 8 bits
    if (input.size()  > i+2 && in[i] == SS_ESC && in[i+1] == ' ' 
      && in[i+2] == 'I')
    {
      i++; i++; 
      continue;
    } 
    // ASCII is G0
    if (input.size() > i+2 && in[i] == SS_ESC && in[i+1] == '(' 
      && in[i+2] == 'B')
    {
      i++; i++; 
      continue;
    } 
    // Right ISO latin is G1
    if (input.size() > i+2 && in[i] == SS_ESC && in[i+1] == '-' 
      && in[i+2] == 'A')
    {
      i++; i++; 
      continue;
    } 
    // Left to right text
    if (input.size() > i+2 && in[i] == SS_ESC && in[i+1] == '1' 
      && in[i+2] == ']')
    {
      i++; i++; 
      continue;
    } 
    // right to left text
    if (input.size() > i+2 && in[i] == SS_ESC && in[i+1] == '2' 
      && in[i+2] == ']')
    {
      i++; i++; 
      continue;
    } 
    // end of text
    if (input.size() > i+1 && in[i] == SS_ESC && in[i+1] == ']' )
    {
      i++;  
      break;
    } 

    // It should not happen but it does.
    // Removed because kterm assumes that conversion did not 
    // change. This is wrong, but let's respect kterm.
    //if (in[i] < ' ') current=0;
    if (current) 
    {
      if (current==&jis0201)
      {
        // katakana should have high bit set
        if (right && in[i] < 0x80)
        {
          // escaped ASCII
          got = (SS_UCS4) in[i];
          ucs4string.append (got);
          continue;
        }
        else
        {
          got = current->decode ((SS_UCS2)(in[i]));
        }
      }
      else
      {
        // GR should have high bit set
        if (right && in[i] < 0x80)
        {
          // escaped ASCII
          got = (SS_UCS4) in[i];
          ucs4string.append (got);
          continue;
        }

        if (input.size() > i+1)
        {
          got = current->decode (0x7f7f & ((SS_UCS2)in[i] << 8 | in[i+1]));
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
SB_X11_JP::delimiters ()
{
  return realDelimiters;
}

/**
 * These methods guess the line delimiters for the input
 * The one without arguments is giving the 'first approximation'
 * It returns an exact list
 */
const SStringVector&
SB_X11_JP::delimiters (const SString& sample)
{
  return sampleDelimiters;
}
