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

#include "stoolkit/SCharClass.h" 
#include "stoolkit/SUniMap.h"
#include "stoolkit/SBinHashtable.h"

const char* ssCharClass[] = {
  "Xx",   // 0 unknown
  "Lu", // 01 Lu Letter, Uppercase
  "Ll", // 02 Ll Letter, Lowercase
  "Lt", // 03 Lt Letter, Titlecase
  "Mn", // 04 Mn Mark, Non-Spacing
  "Mc", // 05 Mc Mark, Spacing Combining
  "Me", // 06 Me Mark, Enclosing
  "Nd", // 07 Nd Number, Decimal Digit
  "Nl", // 08 Nl Number, Letter
  "No", // 09 No Number, Other
  "Zs", // 0A Zs Separator, Space
  "Zl", // 0B Zl Separator, Line
  "Zp", // 0C Zp Separator, Paragraph
  "Cc", // 0D Cc Other, Control
  "Cf", // 0E Cf Other, Format
  "Cs", // 0F Cs Other, Surrogate
  "Co", // 10 Co Other, Private Use
  "Cn", // 10 Cn Other, Not assigned

// Informative Categories

  "Lm", // 12 Lm Letter, Modifier
  "Lo", // 13 Lo Letter, Other
  "Pc", // 14 Pc Punctuation, Connector
  "Pd", // 15 Pd Punctuation, Dash
  "Ps", // 16 Ps Punctuation, Open
  "Pe", // 17 Pe Punctuation, Close
  "Pi", // 18 Pi Punctuation, Initial quote(may behave like Ps or Pe depending on usage)
  "Pf", // 19 Pf Punctuation, Final quote (may behave like Ps or Pe depending on usage)
  "Po", // 1A Po Punctuation, Other
  "Sm", // 1B Sm Symbol, Math
  "Sc", // 1C Sc Symbol, Currency
  "Sk", // 1D Sk Symbol, Modifier
  "So" // 1E So Symbol, Other
};

const char* ssBiDiClass[] = {
  /* strong */
  "XX",
  "L", // Left-to-Right
  "LRE", // Left-to-Right Embedding
  "LRO", // Left-to-Right Override
  "R", // Right-to-Left
  "AL", // Right-to-Left Arabic
  "RLE", // Right-to-Left Embedding
  "RLO", // Right-to-Left Override

  /* weak */
  "PDF", // Pop Directional Format
  "EN",  // European Number
  "ES", // European Number Separator
  "ET", // European Number Terminator
  "AN", // Arabic Number
  "CS", // Common Number Separator
  "NSM", // Non-Spacing Mark
  "BN", // Boundary Neutral

  /* neutral */
  "B",  // Paragraph Separator
  "S", // Segment Separator
  "WS", // Whitespace
  "ON" // Other Neutrals
};

SD_CharClass
getCharClass(SS_UCS4 in)
{
  static SUniMap* ccMap = 0;
  if (ccMap==0)
  {
    ccMap = new SUniMap ("charclass");
    CHECK_NEW (ccMap);
  } 
  if (!ccMap->isOK()) return SD_CC_Xx;
  SString key((char*)&in, sizeof (in));
  // Bengali clusters may start with a virama...
  if (in==0x09cd) return SD_CC_Mc;
  // BLISSYMBOLICS65
  if (in >= 0xe010 && in <= 0xe019) return SD_CC_Nd;
  // ROVASIRAS
  if (in >= 0xee31 && in <= 0xee3f) return SD_CC_Nd;
  // ROVASIRAS
  if (in ==  0xee2f) return SD_CC_Zs;
  // BLISSYMBOLICS65
  if (in >= 0xe000 && in <=0xe0af) return SD_CC_Lo;
  // ROVASIRAS CAPS
  if (in >= 0xee00 && in <=0xee8b) return SD_CC_Lo;
  // ROVASIRAS SMALL
  if (in >= 0xeeb0 && in <=0xef3b) return SD_CC_Lo;

  unsigned int kindex = ccMap->getEncodePosition (in);
  return (SD_CharClass) ccMap->getEncodeValue(kindex);
}

SD_BiDiClass
getBiDiClass(SS_UCS4 in)
{
  static SUniMap* bcMap = 0;
  if (bcMap==0)
  {
    bcMap = new SUniMap ("bidiclass");
    CHECK_NEW (bcMap);
  } 
  if (!bcMap->isOK()) return SD_BC_XX;
  unsigned int kindex = bcMap->getEncodePosition (in);
  SS_UCS4 vle = bcMap->getEncodeValue(kindex);
  return (SD_BiDiClass) vle;
}

SS_UCS4
getMirroredCharacter (SS_UCS4 in)
{
  static SUniMap* mirrMap = 0;
  if (mirrMap==0)
  {
    mirrMap = new SUniMap ("mirroring");
    CHECK_NEW (mirrMap);
  } 
  if (!mirrMap->isOK()) return 0;
  return mirrMap->encode (in);
}
