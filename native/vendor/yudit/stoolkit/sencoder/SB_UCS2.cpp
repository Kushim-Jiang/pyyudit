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
 
#include "stoolkit/sencoder/SB_UCS2.h"
#include "stoolkit/SString.h"
#include "stoolkit/SStringVector.h"

/*
 * This mark can not be possibly in an UCS2 string.
 * so, this can be the byte order.
 */
#define SS_UCS2_MARK 0xfeff

/**
 * Generic USC2 converter
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-05-12
 * 2028 2029 separators in all possible way
 */
SB_UCS2::SB_UCS2(void) : SBEncoder ("\040\050,\040\051,\050\040,\051\040")
{
  start = true;
  surrogate = true;
  decendian = 0;
  encendian = 2;
}

void
SB_UCS2::clear()
{
  start = true;
  SStringVector l("\040\050,\040\051,\050\040,\051\040");
  sampleDelimiters = l;
  realDelimiters = l;
}

static char BE_LINESEP[] = {0x20, 0x28};
static char BE_PARASEP[] = {0x20,0x29};
static char BE_NL[] = {0x00, '\n'};
static char BE_CR[] = {0x00, '\r'};
static char BE_CRNL[] = {0x00, '\r', 0x00, '\n'};

static char LE_LINESEP[] = {0x28, 0x20};
static char LE_PARASEP[] = {0x29, 0x20};
static char LE_NL[] = {'\n', 0x00};
static char LE_CR[] = {'\r', 0x00};
static char LE_CRNL[] = {'\r', 0x00, '\n', 0x00};

/**
 * Set the big endina if flag is true. otherwise little endian
 */
void
SB_UCS2::setBigEndian(bool is)
{
  SString linesep; 
  SString parasep; 
  SString nl; 
  SString cr; 
  SString crnl; 
  if (is)
  {
    decendian = 1;
    linesep.append (BE_LINESEP, sizeof(BE_LINESEP));
    parasep.append (BE_PARASEP, sizeof(BE_PARASEP));
    nl.append (BE_NL, sizeof(BE_NL));
    cr.append (BE_CR, sizeof(BE_CR));
    crnl.append (BE_CRNL, sizeof(BE_CRNL));
  }
  else
  {
    decendian = -1;
    linesep.append (LE_LINESEP, sizeof(LE_LINESEP));
    parasep.append (LE_PARASEP, sizeof(LE_PARASEP));
    nl.append (LE_NL, sizeof(LE_NL));
    cr.append (LE_CR, sizeof(LE_CR));
    crnl.append (LE_CRNL, sizeof(LE_CRNL));
  }
  sampleDelimiters.clear();
  sampleDelimiters.append (linesep);
  sampleDelimiters.append (parasep);
  sampleDelimiters.append (nl);
  sampleDelimiters.append (cr);
  sampleDelimiters.append (crnl);
}


SB_UCS2::SB_UCS2(SEndian e, bool _surrogate) : SBEncoder ("\040\050,\040\051,\050\040,\051\040")
{
  decendian = 0;
  start = true;
  if (e == LITTLE_END)
  {
    setBigEndian(false);
    realDelimiters = sampleDelimiters;
  }
  if (e == BIG_END)
  {
    setBigEndian(true);
    realDelimiters = sampleDelimiters;
  }
  encendian = decendian;
  if (encendian==0) encendian = 2;
  surrogate = _surrogate;
}

SB_UCS2::~SB_UCS2 ()
{
}

SB_UCS2::SEndian
SB_UCS2::setEndian(const SString& in)
{
  if (in.size() < 2) return AUTO_END;
  if (((SS_UCS2_MARK>>8)&(const unsigned char)(in[0]))==(SS_UCS2_MARK>>8) &&
      ((SS_UCS2_MARK&0xff) &(const unsigned char)(in[1]))==(SS_UCS2_MARK&0xff))
  {
    setBigEndian(false);
    return LITTLE_END;
  }
  if (((SS_UCS2_MARK>>8)&(const unsigned char)(in[1]))==(SS_UCS2_MARK>>8) &&
      ((SS_UCS2_MARK&0xff) &(const unsigned char)(in[0]))==(SS_UCS2_MARK&0xff))
  {
    setBigEndian(true);
    return BIG_END;
  }
  return AUTO_END;
}

/**
 * This is always a possible encoding.
 */
bool
SB_UCS2::isOK() const
{
  return true;
}

/**
 * This is encoding a unicode string into a bytestring
 * @param input is a unicode string.
 */
const SString&
SB_UCS2::encode (const SV_UCS4& input)
{
  const SS_UCS4* in = input.array();
  sstring.clear();
  sstring.ensure(input.size()*2);

  /*
   * Put marker
   */
  if (start && input.size()!=0)
  {
    start = false;
    if (encendian == 2) /* auto endian */
    {
      sstring.append ((char) (SS_UCS2_MARK&0xff));
      sstring.append ((char) ((SS_UCS2_MARK>>8)&0xff));
    }
  }

  for (unsigned int i=0; i<input.size(); i++)
  {
    SS_UCS4 in4 = in[i];
    /* make high surrogates */
    if (in4 > 0xffff)
    {
      /* if there is no surrogate support quote */
      if (in4 > 0x10ffff || !surrogate)
      {
        if (encendian < 0) // Little Endian
        {
          quoteStringLE (in4);
        }
        else // Big endian
        {
          quoteStringBE (in4);
        }
        continue;
      }

      in4 = in4 - 0x10000;
      SS_UCS4 in4h = ((in4 >> 10) & 0x3ff) + 0xd800;
      in4 = (in4 & 0x3ff) + 0xdc00;
      if (encendian < 0) // Little Endian
      {
        sstring.append ((char) ((in4h>>8)&0xff));
        sstring.append ((char) (in4h&0xff));
      }
      else // Big endian
      {
        sstring.append ((char) (in4h&0xff));
        sstring.append ((char) ((in4h>>8)&0xff));
      }
      /* low surrogates next */
    }
    if (encendian < 0) // Little Endian
    {
      sstring.append ((char) ((in4>>8)&0xff));
      sstring.append ((char) (in4&0xff));
    }
    else // Big encendian
    {
      sstring.append ((char) (in4&0xff));
      sstring.append ((char) ((in4>>8)&0xff));
    }
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
SB_UCS2::decode (const SString& input)
{
  const unsigned char* in = (unsigned char*) input.array();
  ucs4string.clear();
  ucs4string.ensure(input.size());
  SS_UCS4   got;

  for (unsigned i=0; i<input.size(); i=i+2) 
  {
    if (input.size()==i+1)
    {
      quoteUCS4 ((unsigned char) in[i]);
      continue;
    }
    if (decendian < 0)
    {
      got = (in[i] << 8) + in[i+1];
    }
    else
    {
      got = (in[i+1] << 8) + in[i];
    }
    /* Ignoredecendian  mark */
    if (SS_UCS2_MARK==got) continue;
    /* high surrogates */
    if (got >= 0xd800 && got <= 0xdbff)
    {
      if (!surrogate)
      {
        quoteUCS4 ((unsigned char)in[i]);
        quoteUCS4 ((unsigned char)in[i+1]);
        continue;
      }
      if (i+3 >= input.size())
      {
        quoteUCS4 ((unsigned char)in[i]);
        quoteUCS4 ((unsigned char)in[i+1]);
        continue;
      }
      i++; i++;
      SS_UCS4 got2;
      if (decendian < 0)
      {
        got2 = (in[i] << 8) + in[i+1];
      }
      else
      {
        got2 = (in[i+1] << 8) + in[i];
      }
      if (got2 < 0xdc00 || got2 > 0xdfff)
      {
        quoteUCS4 ((unsigned char)in[i-2]);
        quoteUCS4 ((unsigned char)in[i-1]);
        quoteUCS4 ((unsigned char)in[i]);
        quoteUCS4 ((unsigned char)in[i+1]);
        continue;
      }
      got = ((got&0x3ff) << 10) + (got2&0x3ff) + 0x10000;
    }
    /* low surrogates - came first ! */
    if (got >= 0xdc00 && got <= 0xdfff)
    {
      quoteUCS4 ((unsigned char)in[i]);
      quoteUCS4 ((unsigned char)in[i+1]);
      continue;
    }
    ucs4string.append (got);
    
  }
  return ucs4string;
}


/**
 * These methods guess the line delimiters for the input
 * The one without arguments is giving the 'first approximation'
 * It returns an inclusive list of all possibilities.
 */
const SStringVector&
SB_UCS2::delimiters ()
{
  return realDelimiters;
}

/**
 * These methods guess the line delimiters for the input
 * The one without arguments is giving the 'first approximation'
 * It returns an exact list
 */
const SStringVector&
SB_UCS2::delimiters (const SString& sample)
{
  setEndian (sample);
  return sampleDelimiters;
}
