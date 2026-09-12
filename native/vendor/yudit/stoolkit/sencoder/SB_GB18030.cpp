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
 
#include "stoolkit/sencoder/SB_GB18030.h"
#include "stoolkit/SString.h"
#include "stoolkit/SStringVector.h"

static SS_UCS4 pack (SS_UCS4 pi);
static SS_UCS4 unpack (SS_UCS4 pi);

#define SD_GB_ERROR 0xffffffff

/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * Encoder for Chinesee
 * @version: 2002-03-27
 */
SB_GB18030::SB_GB18030() : SBEncoder ("\n,\r\n,\r"), gb_18030 ("gb-18030")
{
  ok = gb_18030.isOK();
}

SB_GB18030::~SB_GB18030 ()
{
}

/**
 * return false if this generic encoder does not exist.
 */
bool
SB_GB18030::isOK() const
{
  return ok;
}

/**
 * This is encoding a unicode string into a bytestring
 * @param input is a unicode string.
 */
const SString&
SB_GB18030::encode (const SV_UCS4& input)
{
  const SS_UCS4* in = input.array();
  sstring.clear();
  sstring.ensure(input.size()*2);
  SS_UCS4      key;
  SS_UCS4      vle;
  unsigned int kindex;

  for (unsigned int i=0; i<input.size(); i++)
  {
    if (in[i] < 0x80)
    {
      sstring.append ((char) in[i]);
      continue;
    }
    /* surrogate should start with high */
    if (in[i] >= 0xdc00 && in[i] <= 0xdfff)
    {
      quoteString (in[i]);
      continue;
    }
    /* surrogates are  encoded as UCS4 */
    if (in[i] >= 0xd800 && in[i] <= 0xdbff)
    {
      if (!(i+1<input.size() && in[i+1] >= 0xdc00 && in[i+1] <= 0xdfff))
      {
        quoteString (in[i]);
        continue;
      }
      vle = ((in[i] & 0x3ff) << 10) + (in[i+1] & 0x3ff) + 0x10000;
      vle =  unpack (vle + 0x2E248 - 0x10000);
      i++;
    }
    else if (in[i] > 0xffff)
    {
      if (in[i] > 0x10ffff)
      {
        quoteString (in[i]);
        continue;
      }
      vle =  unpack (in[i] + 0x2E248 - 0x10000);
    }
    else
    {
      /* linear approximation */
      if (gb_18030.isOK())
      {
        kindex = gb_18030.getEncodePosition (in[i]);
        key = gb_18030.getEncodeKey(kindex);
        vle = gb_18030.getEncodeValue(kindex);
        if (vle > 0xffff)
        {
          vle = pack (vle);
          if (vle != SD_GB_ERROR)
          {
            vle += (in[i] -key);
            vle = unpack (vle);
          }
        }
        else
        {
          vle += (in[i] -key);
        }

      }
      else
      {
        vle = SD_GB_ERROR;
      }
    }
    if (vle == SD_GB_ERROR)
    {
      quoteString (in[i]);
      continue;
    }
    if (vle > 0xffff)
    {
      sstring.append ((char) ((unsigned char)((vle>>24) & 0xff)));
      sstring.append ((char) ((unsigned char)((vle>>16) & 0xff)));
    }
    /* 2 byte */
    if (vle > 0xff)
    {
      sstring.append ((char) ((unsigned char)((vle>>8) & 0xff)));
    }
    sstring.append ((char) ((unsigned char)((vle>>0) & 0xff)));
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
SB_GB18030::decode (const SString& input)
{
  const unsigned char* in = (unsigned char*) input.array();
  ucs4string.clear();
  ucs4string.ensure(input.size());

  SS_UCS4      key;
  SS_UCS4      vle;
  SS_UCS4      ikey;
  unsigned int kindex;

  for (unsigned i=0; i<input.size(); i++) 
  {
    if (in[i] < 0x80)
    {
       ucs4string.append ((SS_UCS4)input[i]);
       continue;
    }
    /* two byte: 0x81-0xfe +  0x40-0x7e, 0x80-0xfe */
    if (input.size() > i+1 
      && in[i] >= 0x81 && in[i] <= 0xfe
      && ( (in[i+1] >= 0x40 && in[i+1] <= 0x7e) 
           || (in[i+1] >= 0x80 && in[i+1] <= 0xfe)
         ) 
      && gb_18030.isOK())
    {
      ikey = (SS_UCS4) in[i++]; ikey = ikey << 8; 
      ikey += (SS_UCS4) in[i];

      kindex = gb_18030.getDecodePosition (ikey);
      key = gb_18030.getDecodeKey(kindex);
      vle = gb_18030.getDecodeValue(kindex);
      /* should be 2 byte key, and 2 byte value */
      if (vle > 0xffff || key > 0xffff || ikey < key)
      {
        quoteUCS4 (in[i-1]);
        quoteUCS4 (in[i]);
        continue;
      }
      ucs4string.append (vle + ikey - key);
      continue;
    } 
    if (input.size() > i+3 
      && in[i] >= 0x81 && in[i] <= 0xfe
      && in[i+1] >= 0x30 && in[i+1] <= 0x39
      && in[i+2] >= 0x81 && in[i+2] <= 0xfe
      && in[i+3] >= 0x30 && in[i+3] <= 0x39
      && gb_18030.isOK())
    {
      ikey = (SS_UCS4) in[i++]; ikey = ikey << 8; 
      ikey += (SS_UCS4) in[i++]; ikey = ikey << 8; 
      ikey += (SS_UCS4) in[i++]; ikey = ikey << 8; 
      ikey += (SS_UCS4) in[i]; 
      /* non-bmp */
      if (ikey > 0x8431A439) 
      {
        /* out of range */
        if (ikey>0xE3329A35 || ikey<0x90308130)
        {
          quoteUCS4 (in[i-3]);
          quoteUCS4 (in[i-2]);
          quoteUCS4 (in[i-1]);
          quoteUCS4 (in[i]);
          continue;
        }
        vle = 0x10000;
        key = 0x90308130;
      }
      else
      {
        kindex = gb_18030.getDecodePosition (ikey);
        key = gb_18030.getDecodeKey(kindex);
        vle = gb_18030.getDecodeValue(kindex);
      }
      /* should be 4 byte key */
      if (key <= 0xffff || ikey < key)
      {
        quoteUCS4 (in[i-3]);
        quoteUCS4 (in[i-2]);
        quoteUCS4 (in[i-1]);
        quoteUCS4 (in[i]);
        continue;
      }
      ucs4string.append (vle + pack(ikey) - pack(key));
      continue;
    } 
    quoteUCS4 (in[i]);
  }
  return ucs4string;
}


/**
 * These methods guess the line delimiters for the input
 * The one without arguments is giving the 'first approximation'
 * It returns an inclusive list of all possibilities.
 */
const SStringVector&
SB_GB18030::delimiters ()
{
  return realDelimiters;
}

/**
 * These methods guess the line delimiters for the input
 * The one without arguments is giving the 'first approximation'
 * It returns an exact list
 */
const SStringVector&
SB_GB18030::delimiters (const SString& sample)
{
  return sampleDelimiters;
}
/** 
 * pack a gb code into linear code. Return SD_GB_ERROR on fail.
 */
static SS_UCS4 
pack (SS_UCS4 up)
{
  SS_UCS4 value = (SS_UCS4)up;
  unsigned int k0 = (value >> 24) & 0xff; // 0x81..0xfe
  unsigned int k1 = (value >> 16) & 0xff; // 0x30..0x39
  unsigned int k2 = (value >> 8) & 0xff;   // 0x81..0xfe
  unsigned int k3 = (value >> 0) & 0xff;   // 0x30..0x39
  if (k0<0x81 || k0> 0xfe) return SD_GB_ERROR;
  if (k1<0x30 || k1> 0x39) return SD_GB_ERROR;
  if (k2<0x81 || k2> 0xfe) return SD_GB_ERROR;
  if (k3<0x30 || k3> 0x39) return SD_GB_ERROR;
  unsigned int num = (k0-0x81); num = num * 10; 
  num += (k1-0x30); num = num * 126; 
  num += (k2-0x81); num = num * 10; 
  num += (k3-0x30); 
  return ((SS_UCS4)num);

}

/**
 * unpack linear code into gb code. Return SD_GB_ERROR on fail.
 */
static SS_UCS4
unpack (SS_UCS4 p)
{
  SS_UCS4 num = p;
  unsigned int k3 = (num % 10)+0x30; num = num / 10;
  unsigned int k2 = (num % 126)+0x81; num = num / 126;
  unsigned int k1 = (num % 10)+0x30; num = num / 10;
  unsigned int k0 = (num % 126)+0x81; 
  num = (SS_UCS4) ((k0 << 24) + (k1 << 16) + (k2<<8) + k3);
  /* 0x10ffff */
  if (num > 0xE3329A35) return SD_GB_ERROR;
  return num;
}
