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
 
#include "stoolkit/sencoder/SBEncoder.h"
#include "stoolkit/SString.h"
#include "stoolkit/SStringVector.h"

/**
 * This is a sample (base) implementation of the core encoding class
 * This contains a quoting mechanism where things that can not be 
 * converted are converted into 9fffffxx
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-05-12
 */
SBEncoder::SBEncoder(const SStringVector& delim) : sampleDelimiters(delim)
{
  realDelimiters = sampleDelimiters;
}

SBEncoder::~SBEncoder ()
{
}

void
SBEncoder::clear()
{
  remaining.clear();
}

static char _HEXMAP[] = {'0', '1', '2', '3',
'4', '5', '6', '7', '8', '9',
'A', 'B', 'C', 'D', 'E', 'F'};

static char _hexmap[] = {'0', '1', '2', '3',
'4', '5', '6', '7', '8', '9',
'a', 'b', 'c', 'd', 'e', 'f'};
/**
 * Convert illegal byte to a SD_AS_LITERAL
 */
void
SBEncoder::quoteIllegalUCS4 (unsigned char in)
{
  ucs4string.append ((SS_UCS4) 0x9fffff00 | (SS_UCS4) in);
}
/**
 * append '=XX' hex quoted input to ucs4string
 */
void
SBEncoder::quoteUCS4 (unsigned char in)
{
  ucs4string.append ((SS_UCS4) '=');
  ucs4string.append ((SS_UCS4) _HEXMAP[((unsigned int) in >> 4) & 0xf]);
  ucs4string.append ((SS_UCS4) _HEXMAP[(unsigned int) in & 0xf]);
}

/**
 * append '\uxxxx' hex quoted input to ucs4string
 */
void
SBEncoder::quoteUCS4 (SS_UCS2 in)
{
  ucs4string.append ((SS_UCS4) '\\');
  ucs4string.append ((SS_UCS4) 'u');
  ucs4string.append ((SS_UCS4) _hexmap[(in>>12)&0xf]);
  ucs4string.append ((SS_UCS4) _hexmap[(in>>8)&0xf]);
  ucs4string.append ((SS_UCS4) _hexmap[(in>>4)&0xf]);
  ucs4string.append ((SS_UCS4) _hexmap[in&0xf]);
}

void
SBEncoder::quoteUCS4 (SS_UCS4 in)
{
  ucs4string.append ((SS_UCS4) '\\');
  ucs4string.append ((SS_UCS4) 'U');
  ucs4string.append ((SS_UCS4) _hexmap[(in>>28)&0xf]);
  ucs4string.append ((SS_UCS4) _hexmap[(in>>24)&0xf]);
  ucs4string.append ((SS_UCS4) _hexmap[(in>>20)&0xf]);
  ucs4string.append ((SS_UCS4) _hexmap[(in>>16)&0xf]);
  ucs4string.append ((SS_UCS4) _hexmap[(in>>12)&0xf]);
  ucs4string.append ((SS_UCS4) _hexmap[(in>>8)&0xf]);
  ucs4string.append ((SS_UCS4) _hexmap[(in>>4)&0xf]);
  ucs4string.append ((SS_UCS4) _hexmap[in&0xf]);
}

/**
 * append '\uxxxx' hex quoted input to ucs4string
 */
void
SBEncoder::quoteIllegalString (SS_UCS4 in)
{
  if (in < 0x10000)
  {
    sstring.append ((char) '\\');
    sstring.append ((char) 'u');
    sstring.append ((char) _hexmap[(in>>12)&0xf]);
    sstring.append ((char) _hexmap[(in>>8)&0xf]);
    sstring.append ((char) _hexmap[(in>>4)&0xf]);
    sstring.append ((char) _hexmap[in&0xf]);
  }
  else if ((in & 0x9fffff00) == 0x9fffff00)
  {
    sstring.append ((char)( in & 0xff));
  }
  else
  {
    sstring.append ((char) '\\');
    sstring.append ((char) 'U');
    sstring.append ((char) _hexmap[(in>>28)&0xf]);
    sstring.append ((char) _hexmap[(in>>24)&0xf]);
    sstring.append ((char) _hexmap[(in>>20)&0xf]);
    sstring.append ((char) _hexmap[(in>>16)&0xf]);
    sstring.append ((char) _hexmap[(in>>12)&0xf]);
    sstring.append ((char) _hexmap[(in>>8)&0xf]);
    sstring.append ((char) _hexmap[(in>>4)&0xf]);
    sstring.append ((char) _hexmap[in&0xf]);
  }
}

/**
 * append '\uxxxx' hex quoted input to ucs4string
 */
void
SBEncoder::quoteString (SS_UCS4 in)
{
  if (in < 0x10000)
  {
    sstring.append ((char) '\\');
    sstring.append ((char) 'u');
    sstring.append ((char) _hexmap[(in>>12)&0xf]);
    sstring.append ((char) _hexmap[(in>>8)&0xf]);
    sstring.append ((char) _hexmap[(in>>4)&0xf]);
    sstring.append ((char) _hexmap[in&0xf]);
  }
  else if ((in & 0x9fffff00) == 0x9fffff00)
  {
    sstring.append ((char) '=');
    sstring.append ((char) _hexmap[(in>>4)&0xf]);
    sstring.append ((char) _hexmap[in&0xf]);
  }
  else
  {
    sstring.append ((char) '\\');
    sstring.append ((char) 'U');
    sstring.append ((char) _hexmap[(in>>28)&0xf]);
    sstring.append ((char) _hexmap[(in>>24)&0xf]);
    sstring.append ((char) _hexmap[(in>>20)&0xf]);
    sstring.append ((char) _hexmap[(in>>16)&0xf]);
    sstring.append ((char) _hexmap[(in>>12)&0xf]);
    sstring.append ((char) _hexmap[(in>>8)&0xf]);
    sstring.append ((char) _hexmap[(in>>4)&0xf]);
    sstring.append ((char) _hexmap[in&0xf]);
  }
}

void
SBEncoder::quoteStringLE (SS_UCS4 in)
{
  if (in < 0x10000)
  {
    sstring.append ((char) 0);
    sstring.append ((char) '\\');
    sstring.append ((char) 0);
    sstring.append ((char) 'u');
    sstring.append ((char) 0);
    sstring.append ((char) _hexmap[(in>>12)&0xf]);
    sstring.append ((char) 0);
    sstring.append ((char) _hexmap[(in>>8)&0xf]);
    sstring.append ((char) 0);
    sstring.append ((char) _hexmap[(in>>4)&0xf]);
    sstring.append ((char) 0);
    sstring.append ((char) _hexmap[in&0xf]);
  }
  else if ((in & 0x9fffff00) == 0x9fffff00)
  {
    sstring.append ((char) 0);
    sstring.append ((char) '=');
    sstring.append ((char) 0);
    sstring.append ((char) _hexmap[(in>>4)&0xf]);
    sstring.append ((char) 0);
    sstring.append ((char) _hexmap[in&0xf]);
  }
  else
  {
    sstring.append ((char) 0);
    sstring.append ((char) '\\');
    sstring.append ((char) 0);
    sstring.append ((char) 'U');
    sstring.append ((char) 0);
    sstring.append ((char) _hexmap[(in>>28)&0xf]);
    sstring.append ((char) 0);
    sstring.append ((char) _hexmap[(in>>24)&0xf]);
    sstring.append ((char) 0);
    sstring.append ((char) _hexmap[(in>>20)&0xf]);
    sstring.append ((char) 0);
    sstring.append ((char) _hexmap[(in>>16)&0xf]);
    sstring.append ((char) 0);
    sstring.append ((char) _hexmap[(in>>12)&0xf]);
    sstring.append ((char) 0);
    sstring.append ((char) _hexmap[(in>>8)&0xf]);
    sstring.append ((char) 0);
    sstring.append ((char) _hexmap[(in>>4)&0xf]);
    sstring.append ((char) 0);
    sstring.append ((char) _hexmap[in&0xf]);
  }
}
void
SBEncoder::quoteStringBE (SS_UCS4 in)
{
  if (in < 0x10000)
  {
    sstring.append ((char) '\\');
    sstring.append ((char) 0);
    sstring.append ((char) 'u');
    sstring.append ((char) 0);
    sstring.append ((char) _hexmap[(in>>12)&0xf]);
    sstring.append ((char) 0);
    sstring.append ((char) _hexmap[(in>>8)&0xf]);
    sstring.append ((char) 0);
    sstring.append ((char) _hexmap[(in>>4)&0xf]);
    sstring.append ((char) 0);
    sstring.append ((char) _hexmap[in&0xf]);
    sstring.append ((char) 0);
  }
  else if ((in & 0x9fffff00) == 0x9fffff00)
  {
    sstring.append ((char) '=');
    sstring.append ((char) 0);
    sstring.append ((char) _hexmap[(in>>4)&0xf]);
    sstring.append ((char) 0);
    sstring.append ((char) _hexmap[in&0xf]);
    sstring.append ((char) 0);
  }
  else
  {
    sstring.append ((char) '\\');
    sstring.append ((char) 0);
    sstring.append ((char) 'U');
    sstring.append ((char) 0);
    sstring.append ((char) _hexmap[(in>>28)&0xf]);
    sstring.append ((char) 0);
    sstring.append ((char) _hexmap[(in>>24)&0xf]);
    sstring.append ((char) 0);
    sstring.append ((char) _hexmap[(in>>20)&0xf]);
    sstring.append ((char) 0);
    sstring.append ((char) _hexmap[(in>>16)&0xf]);
    sstring.append ((char) 0);
    sstring.append ((char) _hexmap[(in>>12)&0xf]);
    sstring.append ((char) 0);
    sstring.append ((char) _hexmap[(in>>8)&0xf]);
    sstring.append ((char) 0);
    sstring.append ((char) _hexmap[(in>>4)&0xf]);
    sstring.append ((char) 0);
    sstring.append ((char) _hexmap[in&0xf]);
    sstring.append ((char) 0);
  }
}

/**
 * This is encoding a unicode string into a bytestring
 * @param input is a unicode string.
 */
const SString&
SBEncoder::encode (const SV_UCS4& input)
{
  return sstring;
}

/**
 * Decode an input string into a unicode string.
 * @param input is a string.
 *   he output can be null, in this case a line is not
 *   read fully. If input size is zero output will be flushed.
 */
const SV_UCS4&
SBEncoder::decode (const SString& input)
{
  return ucs4string;
}

/**
 * These methods guess the line delimiters for the input
 * The one without arguments is giving the 'first approximation'
 * It returns an inclusive list of all possibilities.
 */
const SStringVector&
SBEncoder::delimiters ()
{
  return realDelimiters;
}

/**
 * These methods guess the line delimiters for the input
 * The one without arguments is giving the 'first approximation'
 * It returns an exact list
 */
const SStringVector&
SBEncoder::delimiters (const SString& sample)
{
  return sampleDelimiters;
}

/* for non-clustering it is remainder */
SString
SBEncoder::preEditBuffer() const
{
  return SString();
}
 /* for clustering */
SV_UCS4
SBEncoder::postEditBuffer () const
{
   return SV_UCS4();
}

/**
 * return key value map to see what decodes to what
 * @param key will contain the keys
 * @param value will contain the values
 * @param _size is the maximum size of returned arrays
 * @return the real size of the arrays.
 */
unsigned int
SBEncoder::getDecoderMap (SStringVector* key, SStringVector* value,
        unsigned int _size)
{
  key->clear();
  value->clear();
  return 0;
}
