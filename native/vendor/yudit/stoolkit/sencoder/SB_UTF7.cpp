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
 
#include "stoolkit/sencoder/SB_UTF7.h"
#include "stoolkit/SString.h"
#include "stoolkit/SStringVector.h"

static unsigned char allowedLoose[] = 
	{"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789'(),-./:?!\"#$%&*;<=>@[]^_`{|}"};
static unsigned char base64Code[] =
	{"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"};

static unsigned char _allowedChars[0x80];
static unsigned char* allowedChars = 0;
static unsigned char _base64Decode[0x80];
static unsigned char* base64Decode = 0;
static void mdecode (SV_UCS4* out, const SString& u7);
static void mencode (SString* out, const SString& ucsin);

/**
 * This is a sample (base) implementation of the core encoding class
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-05-12
 * E2 80 A8 E2 80 A9 are paragraph and line seps in utf-8 (U+20A8, U+20A9) 
 */
SB_UTF7::SB_UTF7() : SBEncoder("\n,\r\n,\r,\342\200\250,\342\200\251")
{
  unsigned int    i;
  if (allowedChars==0)
  {
    allowedChars = _allowedChars;
    memset (allowedChars, 0, 0x80);
    for (i=0; i<sizeof (allowedLoose)-1; i++)
    {
      allowedChars[allowedLoose[i]]=1;
    }
  }
  if (base64Decode==0)
  {
    base64Decode = _base64Decode;
    memset (base64Decode, 0xff, 0x80);
    // -1 is because of null termination
    for (i=0; i<sizeof (base64Code) -1; i++)
    {
      base64Decode[base64Code[i]] = i;
    }
  }
}

SB_UTF7::~SB_UTF7 ()
{
}

/**
 * This is encoding a unicode string into a bytestring
 * @param input is a unicode string.
 */
const SString&
SB_UTF7::encode (const SV_UCS4& input)
{
  sstring.clear();
  sstring.ensure(input.size()*2);

  const SS_UCS4* in = input.array();

  for (unsigned int i=0; i<input.size(); i++)
  {
  	if (in[i] < 0x80 && allowedChars[in[i]]==1)
  	{
  		sstring.append ((char)in[i]);
  		continue;
  	}
  	if (in[i]=='+')
  	{
  		sstring.append ((char)'+');
  		sstring.append ((char)'-');
  		continue;
  	}
  	if (in[i] <= 0x20)
  	{
  		sstring.append ((char)in[i]);
  		continue;
  	}
  	sstring.append ((char)'+');
    SString tmp;
  	while (i<input.size() && (in[i] >= 0x80 || allowedChars[in[i]]==0))
  	{
  		if (in[i] <= 0x20) break;
        /* convert to surrogates */
        if (in[i] >= 0x10000 && in[i] <= 0x10ffff)
        {
          SS_UCS4 hi = (((in[i] - 0x10000) >> 10)&0x3ff) + 0xd800;
  		  tmp.append ((char)(hi >> 8));
  		  tmp.append ((char)(hi & 0xff));

          SS_UCS4 lo = ((in[i] - 0x10000) & 0x3ff) + 0xdc00;
  		  tmp.append ((char)(lo >> 8));
  		  tmp.append ((char)(lo & 0xff));
        }
        else
        {
  		  tmp.append ((char)(in[i] >> 8));
  		  tmp.append ((char)(in[i] & 0xff));
        }
  		i++;
  	}
  	mencode (&sstring, tmp);
#if LAZYUTF7
  	if (i == input.size() || base64Decode[in[i]] != 0xff) 
  	{
  		sstring.append ((char)'-');
  	}
#else
  	sstring.append ((char)'-');
#endif
    i--;
  }
  return sstring;
}

/**
 * Encode the input string into utf7
 */
static void
mencode (SString* out, const SString &ucsin)
{
  SString nin = ucsin;
  nin.append((char)0);
  const unsigned char* in = (unsigned char*) nin.array();
  unsigned char  uchar4[4];
    
  for (unsigned int i=0;i<ucsin.size();)
  {
    
    uchar4[0] = base64Code[in[i]>>2];
    out->append ((char)uchar4[0]);

    uchar4[1] = base64Code[((in[i] & 0x03)<<4) | (in[i+1] >> 4)];
    out->append ((char)uchar4[1]);
    if (i+1>=ucsin.size()) break;

    uchar4[2] = base64Code[((in[i+1] & 0x0f)<<2) | (in[i+2] >> 6)];
    out->append ((char)uchar4[2]);
    if (i+2>=ucsin.size()) break;

    uchar4[3] = base64Code[in[i+2] & 0x3f];
    out->append ((char)uchar4[3]);
    i += 3;
  }
}

/**
 * Decode an input string into a unicode string.
 * @param input is a string.
 *   he output can be null, in this case a line is not
 *   read fully. If input size is zero output will be flushed.
 */
const SV_UCS4&
SB_UTF7::decode (const SString& input)
{
  ucs4string.clear();
  ucs4string.ensure(input.size());

  const unsigned char* in = (unsigned char*) input.array();
  unsigned int i;
  for (i=0; i<input.size(); i++)
  {
  	if (input.size()-i > 1 && in[i] == '+' && in[i+1] == '-')
  	{
  		ucs4string.append ((SS_UCS4) '+');
  		i++;
  		continue;
  	}
  	if (in[i] != '+')
  	{
  		ucs4string.append ((SS_UCS4) in[i]);
  		continue;
  	}
  	i++;
    SString u7;
  	while (i<input.size() && (in[i] < 0x80 && base64Decode[in[i]] != 0xff))
  	{
  		u7.append ((char)in[i]);
  		i++;
  	}
  	// '-' should be absorbed
  	if (in[i] != '-') i--;
  	mdecode (&ucs4string, u7);
  }
  /* collapse surrogates */
  for (i=0; i+1<ucs4string.size(); i++)
  {
    SS_UCS4 hi = ucs4string[i];
    SS_UCS4 lo = ucs4string[i+1];
    if (hi >= 0xd800 && hi <= 0xdbff && lo >= 0xdc00 && lo <= 0xdfff)
    {
      SS_UCS4 vle = ((hi & 0x3ff) << 10)  + (lo & 0x3ff) + 0x10000;
      ucs4string.insert(i, vle);
      ucs4string.remove(i+1);
      ucs4string.remove(i+1);
    }
  }
  return ucs4string;
}

static void
mdecode (SV_UCS4* out, const SString& u7)
{
  SString s = u7;
  s.append ((char) base64Code[0]);
  s.append ((char) base64Code[0]);
  s.append ((char) base64Code[0]);
  const unsigned char* in = (unsigned char*) s.array();
  
  SS_UCS4    uch;
  unsigned char  cch1, cch2;

  // The buffer is already aligned
  for (unsigned int i=0; i<u7.size(); )
  {
    cch1 = (SS_UCS4) (base64Decode[in[i]] <<2)
      | (base64Decode[in[i+1]]>>4);

    cch2= (SS_UCS4) (base64Decode[in[i+1]] <<4)
      | (base64Decode[in[i+2]]>>2);

    uch = (cch1 << 8) |  cch2;
    if (uch!=0) out->append (uch);
    if (i+1>=u7.size()) break;

    cch1 = (SS_UCS4) (base64Decode[in[i+2]] <<6)
      | base64Decode[in[i+3]];
    i+=4;
    cch2 = (SS_UCS4) (base64Decode[in[i]] <<2)
      | (base64Decode[in[i+1]]>>4);

    uch = (cch1 << 8) | cch2;
    if (uch!=0) out->append (uch);
    if (i>=u7.size()) break;

    cch1= (SS_UCS4) (base64Decode[in[i+1]] <<4)
      | (base64Decode[in[i+2]]>>2);
    cch2 = (SS_UCS4) (base64Decode[in[i+2]] <<6)
      | base64Decode[in[i+3]];

    uch = (cch1 << 8) | cch2;
    if (uch!=0) out->append (uch);
    i+=4;
  }
}

/**
 * These methods guess the line delimiters for the input
 * The one without arguments is giving the 'first approximation'
 * It returns an inclusive list of all possibilities.
 */
const SStringVector&
SB_UTF7::delimiters ()
{
  return realDelimiters;
}

/**
 * These methods guess the line delimiters for the input
 * The one without arguments is giving the 'first approximation'
 * It returns an exact list
 */
const SStringVector&
SB_UTF7::delimiters (const SString& sample)
{
  return sampleDelimiters;
}
