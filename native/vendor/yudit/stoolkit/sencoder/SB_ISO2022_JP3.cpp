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
 
#include "stoolkit/sencoder/SB_ISO2022_JP3.h"
#include "stoolkit/SString.h"
#include "stoolkit/SStringVector.h"
#include "stoolkit/SCluster.h"

#define SS_ESC 27

/**
 * This is a sample (base) implementation of the core encoding class
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-05-12
 * E2 80 A8 E2 80 A9 are paragraph and line seps in utf-8 (U+20A8, U+20A9) 
 */
SB_ISO2022_JP3::SB_ISO2022_JP3() : SBEncoder ("\n,\r\n,\r"), 
  jis02131 ("jis-0213-1"), jis02132("jis-0213-2")
{
  ok = jis02131.isOK() && jis02132.isOK();
}

SB_ISO2022_JP3::~SB_ISO2022_JP3 ()
{
}

/**
 * return false if this generic encoder does not exist.
 */
bool
SB_ISO2022_JP3::isOK() const
{
  return ok;
}

/**
 * This is encoding a unicode string into a bytestring
 * @param input is a unicode string.
 */
const SString&
SB_ISO2022_JP3::encode (const SV_UCS4& input)
{
  const SS_UCS4* in = input.array();
  sstring.clear();
  sstring.ensure(input.size()*2);
  SUniMap* current=0;
  SS_UCS2   got;
  unsigned char c0;
  unsigned char c1;

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
    if (jis02131.isOK())
    {
      SV_UCS4 decd; SV_UCS4 enc;
      enc.append (in[i]);
      if (i+1 < input.size()) enc.append (in[i+1]);
      if (i+2 < input.size()) enc.append (in[i+2]);
      unsigned int lifted = jis02131.lift (enc, 0, false, &decd);
      if (lifted>0 && decd.size()==1 && decd[0] != 0)
      {
        if (current != &jis02131)
        {
          sstring.append ((char) SS_ESC);
          sstring.append ((char) '$');
          sstring.append ((char) '(');
          sstring.append ((char) 'O');
          current = &jis02131;
        }
        got = decd[0];
        c1 = ((got>>8) & 0xff);
        c0 = (got & 0xff);
        sstring.append ((char) c1);
        sstring.append ((char) c0);
        i = i + lifted-1;
        continue;
      }
    }
    if (jis02132.isOK())
    {
      SV_UCS4 decd; SV_UCS4 enc;
      enc.append (in[i]);
      if (i+1 < input.size()) enc.append (in[i+1]);
      if (i+2 < input.size()) enc.append (in[i+2]);
      unsigned int lifted = jis02132.lift (enc, 0, false, &decd);
      if (lifted>0 && decd.size()==1 && decd[0] != 0)
      {
        if (current != &jis02132)
        {
          sstring.append ((char) SS_ESC);
          sstring.append ((char) '$');
          sstring.append ((char) '(');
          sstring.append ((char) 'P');
          current = &jis02132;
        }
        got = decd[0];
        c1 = ((got>>8) & 0xff);
        c0 = (got & 0xff);
        sstring.append ((char) c1);
        sstring.append ((char) c0);
        i = i + lifted-1;
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
SB_ISO2022_JP3::decode (const SString& input)
{
  const unsigned char* in = (unsigned char*) input.array();
  SUniMap* current=0;
  ucs4string.clear();
  ucs4string.ensure(input.size());

  for (unsigned i=0; i<input.size(); i++) 
  {
    /* to keep compatibility. */
    if (i+2 < input.size() && in[i]==SS_ESC && in[i+1] == '$' && in[i+2] == 'B')
    {
      current=&jis02131; // JIS X 0X208-1983 - compatibility JIS X 0X0213 P1
      i++; i++;
      continue;
    } 
    if (i+3 < input.size() && in[i]==SS_ESC && in[i+1] == '$' 
      && in[i+2] == '(' && in[i+3] == 'O')
    {
      current=&jis02131; // JIS X 0213-2000 PLANE 1
      i++; i++; i++;
      continue;
    } 
    if (i+3 < input.size() && in[i]==SS_ESC && in[i+1] == '$' 
      && in[i+2] == '(' && in[i+3] == 'P')
    {
      current=&jis02132; // JIS X 0213-2000 PLANE 2
      i++; i++; i++;
      continue;
    } 
    if (i+2 < input.size() && in[i]==SS_ESC && in[i+1] == '(' && in[i+2] == 'B')
    {
      current=0; // ASCII
      i++; i++;
      continue;
    }

    // It should not happen but it does.
    if (in[i] < ' ') current=0;
    if (current == 0) /* IRV - ASCII */
    {
      ucs4string.append ((SS_UCS4) in[i]);
      continue;
    }
    //  converter exists
    if (current->isOK() && i+1<input.size())
    {
      SV_UCS4 ucs4; 
      SV_UCS4 decd; 
      ucs4.append (((SS_UCS4)in[i] << 8) | (SS_UCS4)in[i+1]);
      unsigned int lifted = current->lift (ucs4, 0, true, &decd);
      if (lifted != 0 && decd.size() != 0 && decd[0] != 0)
      {
        expandYuditLigatures (&decd);
        ucs4string.append (decd);
        i++;
        continue;
      }
    }
    quoteUCS4 (in[i]);
    if (i+1<input.size()) quoteUCS4 (in[i+1]);
    i++;
  }
  return ucs4string;
}


/**
 * These methods guess the line delimiters for the input
 * The one without arguments is giving the 'first approximation'
 * It returns an inclusive list of all possibilities.
 */
const SStringVector&
SB_ISO2022_JP3::delimiters ()
{
  return realDelimiters;
}

/**
 * These methods guess the line delimiters for the input
 * The one without arguments is giving the 'first approximation'
 * It returns an exact list
 */
const SStringVector&
SB_ISO2022_JP3::delimiters (const SString& sample)
{
  return sampleDelimiters;
}
