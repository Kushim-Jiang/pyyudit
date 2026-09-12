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
 
#include "stoolkit/sencoder/SB_Johab.h"
#include "stoolkit/SString.h"
#include "stoolkit/SStringVector.h"

/**
 * JOHAB converter : KS C 5601-1992, Annex 3, supplementary encoding
 * rewritten code contributed by Jungshik Shin
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-05-12
 */
SB_Johab::SB_Johab() : SBEncoder ("\n,\r\n,\r"), ksc_5601_r ("ksc-5601-r")
{
  ok = ksc_5601_r.isOK();
}

SB_Johab::~SB_Johab ()
{
}

/**
 * return false if this generic encoder does not exist.
 */
bool
SB_Johab::isOK() const
{
  return ok;
}


/**
 * This is encoding a unicode string into a bytestring
 * @param input is a unicode string.
 */
const SString&
SB_Johab::encode (const SV_UCS4& input)
{
  const SS_UCS4* in = input.array();
  unsigned char  c1;
  unsigned char  c2;
  SS_UCS2   got;

  static const SS_UCS2 jamo_from_ucs[51] =
  { 0x8841, 0x8c41,
    0x8444,
    0x9041,
    0x8446, 0x8447,
    0x9441, 0x9841, 0x9c41,
    0x844a, 0x844b, 0x844c, 0x844d, 0x884e, 0x884f, 0x8450,
    0xa041, 0xa441, 0xa841,
    0x8454,
    0xac41, 0xb041, 0xb441, 0xb841, 0xbc41,
    0xc041, 0xc441, 0xc841, 0xca41, 0xd041,
    0x8461, 0x8481, 0x84a1, 0x84c1, 0x84e1,
    0x8541, 0x8561, 0x8581, 0x85a1, 0x85c1, 0x85e1,
    0x8641, 0x8661, 0x8681, 0x86a1, 0x86c1, 0x86e1,
    0x8741, 0x8761, 0x8781, 0x87a1
  };

  sstring.clear();
  sstring.ensure(input.size()*2);

  for (unsigned int i=0; i<input.size(); i++)
  {
    if (in[i] < 0x80)
    {
      sstring.append ((char) in[i]);
      continue;
    }
    if (in[i]>= 0xac00 && in[i] <= 0xd7af) 
    {
      SS_UCS4 ch = in[i]-0xac00;
      int l = ch / 588;  // 588 = 21 * 28
      int m = (ch / 28) % 21; 
      int t = ch % 28;
      
      got =   ( (l+2) << 10 )  |
      ((m + (m<5 ? 3 : (m<11 ? 5 : (m<17 ? 7 : 9)))) << 5) |
      (t + (t<17 ? 1 : 2))   | 0x8000;
      sstring.append ((char) ((got & 0xff00) >> 8) );
      sstring.append ((char) (got&0xff) );
      continue;
    }
    if (in[i] > 0x3130 && in[i] < 0x3164) 
    {
      got = jamo_from_ucs[in[i]-0x3131];
      sstring.append ((char) ((got & 0xff00) >>8) );
      sstring.append ((char) (got&0xff) );
      continue;
    }
    if (ksc_5601_r.isOK()&& (got=ksc_5601_r.encode ((SS_UCS4)in[i])) != 0)
    {
      c1 = (got >> 8) & 0x7f;
      c2 = got & 0x7f;
      if ( (in[i] >=0x4e00 && in[i] <=0x9fa5) ||
        (in[i] >= 0xf900 && in[i] <= 0xfa0b))
      {
        c1 -= 0x4a;
        c2 |= 0x80;
        got =  ((c1 / 2) << 8)  + 0xe000 + c2
            +  (c1 % 2 ? 0 
          : (c2 > 0xee ? 0x43 : 0x31) - 0xa1 );
      }
      else 
      {
        c1 -= 0x21;
        c2 |= 0x80;
        got = ((c1 / 2) << 8) + 0xd900 + c2
            + (c1 % 2 ? 0 
                    : (c2 > 0xee ? 0x43 : 0x31) - 0xa1 ); 
      }
      sstring.append ((char) ((got & 0xff00) >>8));
      sstring.append ((char) (got&0xff) );
      continue;
    }
    quoteString (in[i]);
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
SB_Johab::decode (const SString& input)
{
/* 
 * The table for Bit pattern to Hangul Jamo
 * 5 bits each are used to encode
 * leading consonants(19 + 1 filler),medial vowels(21 + 1 filler) 
 * and trailing consonants(27 + 1 filler). 
 *
 * KS C 5601-1992 Annex 3 Table 2 
 * 0 : Filler, -1: invalid, >= 1 : valid
*/
  static int lead[32] =
  {-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
   19, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  static int mid[32] =
  {-1, -1, 0, 1, 2, 3, 4, 5,
   -1, -1, 6, 7, 8, 9, 10, 11,
   -1, -1, 12, 13, 14, 15, 16, 17,
   -1, -1, 18, 19, 20, 21, -1, -1};
  static int trail[32] =
  {-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
   -1, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, -1, -1};
  static SS_UCS2 lead_to_ucs[19] =
  {
    0x3131, 0x3132, 0x3134, 0x3137, 0x3138, 0x3139, 0x3141, 0x3142,
    0x3143, 0x3145, 0x3146, 0x3147, 0x3148, 0x3149, 0x314a, 0x314b,
    0x314c, 0x314d, 0x314e
  };

  static SS_UCS2 trail_to_ucs[27] =
  {
    0, 0, 0x3133, 0, 0x3135, 0x3136, 0, 0,
    0x313a, 0x313b, 0x314c, 0x313d, 0x313e, 0x313f,
    0x3140, 0, 0, 0x3144, 0, 0, 0,
    0, 0, 0, 0, 0, 0
  };

  const unsigned char* in = (unsigned char*) input.array();
  ucs4string.clear();
  ucs4string.ensure(input.size());
  SS_UCS4   got;
  int       idx;

  for (unsigned i=0; i<input.size(); i++) 
  {
    if (in[i] < 0x7f)
    {
      ucs4string.append ((SS_UCS4) in[i]);
      continue;
    }

    if (in[i] > 0xf9 || in[i] == 0xdf || (in[i] > 0x7e &&
        in[i] < 0x84) || (in[i] > 0xd3 && in[i] < 0xd9))
    {
      quoteUCS4 ((unsigned char) in[i]);
      continue;
    }
  
    // Hangul 
    if (input.size()  > i+1 && in[i] <= 0xd3 && 
        ((in[i+1]> 0x40 && in[i+1] < 0x7f)
          || (in[i+1] > 0x80 && in[i+1] < 0xff)) )
    {
      int l,m,t;

      idx = (in[i] << 8) + in[i+1];
      l = lead[(idx & 0x7c00) >> 10];
      m = mid[(idx & 0x03e0) >> 5];
      t = trail[idx & 0x001f];

      if (l == -1 || m == -1 || t == -1) 
        got=0;
      // Syllable 
      else if (l > 0 && m > 0)
        got = ((l-1)*21 + (m-1))*28 + t + 0xac00;
      // Initial Consonant
      else if (l > 0 && m == 0 && t == 0)
        got = lead_to_ucs[l - 1];
      // Vowel
      else if (l == 0 && m > 0 && t == 0)
        got = 0x314e + m; // 0x314f + m - 1
      // Final Consonant
      else if (l == 0 && m == 0 && t > 0)
        got = trail_to_ucs[t - 1];
      else
        got = 0;
      if (got == 0)
      {
        quoteUCS4 ((unsigned char) in[i]);
        quoteUCS4 ((unsigned char) in[i+1]);
      }
      else
      {
        ucs4string.append (got);
      }
      i++;
      continue;
    }
    // Hanja & Symbol
    if (input.size() > i+1 && in[i] > 0xd8 &&
      ((in[i+1]> 0x30 && in[i+1] < 0x7f)
      || (in[i+1] > 0x90 && in[i+1] < 0xff)) )
    {
      // User Defined Area : Unused
      if ( in[i]==0xda && 
        in[i+1]>0xa0 && in[i+1]<0xd4 )
        got=0;
      // Symbol
      else  if (in[i] < 0xdf) 
      {
        idx = ( ( (in[i]-0xd9) * 2 
            + (in[i+1] > 0xa0 ? 1 : 0) + 0xa1 ) << 8 )
            + in[i+1] + (in[i+1] > 0xa0 ? 0 : 
              (in[i+1] > 0x90 ? 0x5e : 0x70) );   
        got=ksc_5601_r.decode((SS_UCS2)idx);
      }
      // Hanja
      else 
      {
        idx = ( ( (in[i]-0xe0) * 2 
            + (in[i+1] > 0xa0 ? 1 : 0) + 0xca ) << 8 )
            + in[i+1] + (in[i+1] > 0xa0 ? 0 : 
              (in[i+1] > 0x90 ? 0x5e : 0x70) );   
        got=ksc_5601_r.decode((SS_UCS2)idx);
      }
      if (got ==0)
      {
        quoteUCS4 ((unsigned char) in[i]);
        quoteUCS4 ((unsigned char) in[i+1]);
      }
      else
      {
        ucs4string.append (got);
      }
      i++;
      continue;
    } 

    if (in[i] > 0x7e)
    {
      quoteUCS4 ((unsigned char) in[i]);
      continue;
    }
    ucs4string.append ((SS_UCS4) in[i]);
  }
  return ucs4string;
}


/**
 * These methods guess the line delimiters for the input
 * The one without arguments is giving the 'first approximation'
 * It returns an inclusive list of all possibilities.
 */
const SStringVector&
SB_Johab::delimiters ()
{
  return realDelimiters;
}

/**
 * These methods guess the line delimiters for the input
 * The one without arguments is giving the 'first approximation'
 * It returns an exact list
 */
const SStringVector&
SB_Johab::delimiters (const SString& sample)
{
  return sampleDelimiters;
}
