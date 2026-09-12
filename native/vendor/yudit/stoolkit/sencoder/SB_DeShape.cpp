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
 
#include "stoolkit/sencoder/SB_DeShape.h"
#include "stoolkit/SString.h"
#include "stoolkit/SStringVector.h"
#include "stoolkit/STextData.h"

#define SS_ESC 27

/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-05-12
 * This is the counterpart of SB_Shape. It does just the
 * Opposite thing - it takes Presentation Forms and 
 * convert them back into normal characters - reverse of
 * Roman Czyborra's arabjoin.
 */
SB_DeShape::SB_DeShape() : SBEncoder ("\n,\r\n,\r"), shape ("shape"), interface(false)
{
  ok = shape.isOK();
}

SB_DeShape::~SB_DeShape ()
{
}

/**
 * return false if this generic encoder does not exist.
 */
bool
SB_DeShape::isOK() const
{
  return ok;
}

/**
 * This is encoding a unicode string into a bytestring
 * @param input is a unicode string.
 */
const SString&
SB_DeShape::encode (const SV_UCS4& input)
{
  return interface.encode (convert(input));
}

/**
 * Decode an input string into a unicode string.
 * @param input is a string.
 *   he output can be null, in this case a line is not
 *   read fully. If input size is zero output will be flushed.
 */
const SV_UCS4&
SB_DeShape::decode (const SString& _input)
{
  SV_UCS4 decd = interface.decode (_input);
  return convert (decd);
}

/**
 * Decode an input string into a unicode string.
 * @param input is a string.
 *   he output can be null, in this case a line is not
 *   read fully. If input size is zero output will be flushed.
 */
const SV_UCS4&
SB_DeShape::convert (const SV_UCS4& decd)
{
  for (unsigned int i=0; i<decd.size(); )
  {
    SV_UCS4 ret;
    unsigned int n = shape.lift (decd, i, true, &ret);
    /* the composition comes at the end  - if any */
    if (n>=i+1)
    {
      ucs4string.append (ret);
      i = n;
    }
    else
    {
      ucs4string.append (decd[i]);
      i++;
    }
  }
  return ucs4string;
}

/**
 * These methods guess the line delimiters for the input
 * The one without arguments is giving the 'first approximation'
 * It returns an inclusive list of all possibilities.
 */
const SStringVector&
SB_DeShape::delimiters ()
{
  return realDelimiters;
}

/**
 * These methods guess the line delimiters for the input
 * The one without arguments is giving the 'first approximation'
 * It returns an exact list
 */
const SStringVector&
SB_DeShape::delimiters (const SString& sample)
{
  return sampleDelimiters;
}
