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
 
#include "stoolkit/sencoder/SB_BiDi.h"
#include "stoolkit/SString.h"
#include "stoolkit/SStringVector.h"
#include "stoolkit/STextData.h"

#define SS_ESC 27

/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-05-12
 */

/**
 * This is a converter to import data that is supposed to
 * be parsed with a bi-di parser.
 * Yudit uses a less nasty format.
 * Yudit's format is more consistent and almost
 * compliant with unicode standard, and I think it is less ambiguous
 * than the standard. SO you dont really need this class :)
 */

/**
 *-----------------------------------------------------------------------------
 *-----------------------------------------------------------------------------
 *
 * FOR IMPLEMENTORS:
 * You only need to write 2 routines. You dont need any tables!
 *
 *    1. Take a look at Yudit BiDi in doc/Yudit.bidi.txt
 *         You will
 *           -  SB_BiDi::decode to yudit format from utf-8
 *           -  SB_BiDi::encode from yudit format to utf-8
 *
 *    2. Take a look at fribidi. I have one implementation at 
 *          stoolkit/work. You dont need those huse tables. 
 *          you can get the category with:
 *              SS_UCS2 u2 = category.encode (ucs4[i]);
 *              if (u2 == 0x4) // Mark, Non-Spacing
 *          see mytool/mys/category.mys
 *
 * Sorry I was too busy so I did not implement this.
 * 
 * Gaspar Sinai <gaspar@yudit.org>
 * 2001-11-25.
 *-----------------------------------------------------------------------------
 *-----------------------------------------------------------------------------
 */       

/**
 * TODO: implement this.
 */
SB_BiDi::SB_BiDi() : SBEncoder ("\n,\r\n,\r"), category ("category"), interface(false)
{
  ok = false;
  /* FIXME: remove comment 
  ok = category.isOK();
  */
}

SB_BiDi::~SB_BiDi ()
{
}

/**
 * return false if this generic encoder does not exist.
 */
bool
SB_BiDi::isOK() const
{
  return ok;
}

/**
 * This is encoding a unicode string into a bytestring
 * @param input is a unicode string.
 */
const SString&
SB_BiDi::encode (const SV_UCS4& input)
{
  sstring = interface.encode (input);
  /* TODO: do the fuzz here. use category. LRO RLO and PDF needs to be
   * taken care. By default assume LRO.
   */
  return sstring;
}

/**
 * Decode an input string into a unicode string.
 * @param input is a string.
 *   he output can be null, in this case a line is not
 *   read fully. If input size is zero output will be flushed.
 */
const SV_UCS4&
SB_BiDi::decode (const SString& _input)
{
  ucs4string = interface.decode (_input);
  /* TODO: do the fuzz here. use category. */
  return ucs4string;
}

/**
 * These methods guess the line delimiters for the input
 * The one without arguments is giving the 'first approximation'
 * It returns an inclusive list of all possibilities.
 */
const SStringVector&
SB_BiDi::delimiters ()
{
  return realDelimiters;
}

/**
 * These methods guess the line delimiters for the input
 * The one without arguments is giving the 'first approximation'
 * It returns an exact list
 */
const SStringVector&
SB_BiDi::delimiters (const SString& sample)
{
  return sampleDelimiters;
}
