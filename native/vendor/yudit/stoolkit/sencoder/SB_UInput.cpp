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
 
#include "stoolkit/sencoder/SB_UInput.h"
#include "stoolkit/SString.h"
#include "stoolkit/SStringVector.h"
#include "stoolkit/STypes.h" 
#include <stdlib.h>

/**
 * This is a sample (base) implementation of the core encoding class
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-05-12
 * E2 80 A8 E2 80 A9 are paragraph and line seps in utf-8 (U+20A8, U+20A9) 
 */
SB_UInput::SB_UInput() : SBEncoder ("\n,\r\n,\r,\342\200\250,\342\200\251")
{
}

SB_UInput::~SB_UInput ()
{
}

/**
 * This is encoding a unicode string into a bytestring
 * @param input is a unicode string.
 * endecode is not quite reversible
 * it is producing \UXXXXXXXX and \uxxxx
 */
const SString&
SB_UInput::encode (const SV_UCS4& iucs)
{
  sstring.clear();
  for (unsigned int i=0; i<iucs.size(); i++)
  {
    quoteString (iucs[i]);
  }
  return  sstring;
}

/**
 * Decode an input string into a unicode string.
 * This is good for an input method.
 * @param input is a string.
 *   he output can be null, in this case a line is not
 *   read fully. If input size is zero output will be flushed.
 * uXXXX or UXXXXXXXX
 */
const SV_UCS4&
SB_UInput::decode (const SString& _input)
{
  ucs4string.clear();
  if (_input.size()==0)
  {
    for (unsigned int k=0; k<input.size(); k++)
    {
      ucs4string.append ((SS_UCS4) ((unsigned char)input[k]));
    }
    input.clear();
    return ucs4string;
  }
  input.append (_input);

  SString cin = input;
  cin.append ((char)0);
  const unsigned char* in = (const unsigned char*) cin.array();

  char* next;
  unsigned int rd = 0;

  /* see what we have */
  SS_UCS4 decoded;
  for (unsigned int i=0; i<input.size(); i++)
  {
    if (input.size() > i+4 && in[i] == 'u')
    {
      decoded = (SS_UCS4)  strtoul ((char*) &in[i+1],  &next, 16);
      // success we append even zeros
      if (in + i + 5 == (unsigned char*) next) 
      {
        ucs4string.append (decoded);
        rd += 5;
        i += 4;
        continue;
      }
    }
    else if (input.size() > i+8 && in[i] == 'U')
    {
      decoded = (SS_UCS4)  strtoul ((char*) &in[i+1],  &next, 16);
      // success we append even zeros
      if (in + i + 9 == (unsigned char*) next) 
      {
        ucs4string.append (decoded);
        rd += 9;
        i += 8;
        continue;
      }
    }
    unsigned int expected = 0;
    if (input.size() ==1 && (in[i] == 'u' || in[i] == 'U')) break;
    if (input.size() > 1 && in[i] == 'u')
    {
      expected = 5;
    }
    if (input.size() > 1 && in[i] == 'U')
    {
      expected = 9;
    }
    /* see if all can be good */
    if (input.size() < i+expected && _input.size())
    {
      unsigned int j;
      for (j=i+1; j<input.size(); j++)
      {
         if ((in[j] >= '0' && in[j] <='9')
            || (in[j] >= 'a' && in[j] <='f')
            || (in[j] >= 'A' && in[j] <='F'))
         {
           continue;
         }
         break;
      }
      if (j==input.size()) break;
    }
    decoded = (SS_UCS4) in[i];
    ucs4string.append (decoded);
    rd += 1;
  }
  while (rd--) input.remove(0);
  return ucs4string;
}

/**
 * clear input and output buffers.
 */
void
SB_UInput::clear ()
{
  //input.clear();
}

SString
SB_UInput::preEditBuffer() const
{
  return SString(input);
}

/**
 * These methods guess the line delimiters for the input
 * The one without arguments is giving the 'first approximation'
 * It returns an inclusive list of all possibilities.
 */
const SStringVector&
SB_UInput::delimiters ()
{
  return realDelimiters;
}

/**
 * These methods guess the line delimiters for the input
 * The one without arguments is giving the 'first approximation'
 * It returns an exact list
 */
const SStringVector&
SB_UInput::delimiters (const SString& sample)
{
  return sampleDelimiters;
}

/**
 * return key value map to see what decodes to what
 * @param key will contain the keys
 * @param value will contain the values
 * @param _size is the maximum size of returned arrays
 * @return the real size of the arrays.
 */
unsigned int
SB_UInput::getDecoderMap (SStringVector* key, SStringVector* value,
        unsigned int _size)
{
  key->clear();
  value->clear();
  char sk[64];
  char sv[64];
  for (unsigned int i=0; i<_size && i<128 - 32; i++)
  {
     snprintf (sk, 64, "u%04d", i+32);
     key->append (sk);
     sv[0] = (char)(i+32);
     sv[1] = 0;
     value->append (sv);
  }
  return 0x7fffffff;
}
