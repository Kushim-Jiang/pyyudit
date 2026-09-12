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

/*!
 * \file SRendClass.cpp
 * \brief Convert Yudit Character Types to OTF.
 * \author: Gaspar Sinai <gaspar@yudit.org>
 * \version: 2000-04-23
 */

#include "stoolkit/SRendClass.h"

#include "stoolkit/SCluster.h"

/*
 * \brief Get the inherent rendering class of the character.
 *
 * We use a somewhat different naming convention than
 * specified in OpenType documents.  Here is the Mapping:
 * 
 * The types in square brackets are handlesd in a 
 * hard-coded switch in here.
 *  
 * Cbase                    SD_INDIC_CONSONANT_BASE 
 * Cbellow                  SD_INDIC_CONSONANT_BELOW_BASE 
 * Cpost                    SD_INDIC_CONSONANT_POST_BASE 
 * Cdead                    SD_INDIC_CONSONANT_DEAD 
 * Mbelow[,VMbelow]         SD_INDIC_BOTTOM_VOWEL 
 * Mabove                   SD_INDIC_TOP_VOWEL
 * Mpost[,Mabove]           SD_INDIC_RIGHT_VOWEL 
 * VMpost[,VMabove,SMabove] SD_INDIC_MODIFIER 
 * SMabove[,SMbelow]        SD_INDIC_SIGN (only U+0952 is SMbelow)
 * VO                       SD_INDIC_INDEP_VOWEL  
 * Mpre                     SD_INDIC_LEFT_VOWEL
 * Halant                   SD_INDIC_HALANT
 * Nukta                    SD_INDIC_NUKTA
 *
 * Vsplit                   SD_INDIC_LEFT_RIGHT_VOWEL - need to split
 * None                     Extension
 *
 * The following types are not returned by this program:
 *
 * Cfirst                   First consonant
 * Clast                    Last consonant
 * Cpre                     Pre-base 
 * Chbase                   Cbase that can be in half form.
 * Cra                      The first non-reph-ra.
 * CMra                    The first non-reph-ra if there is post-base matra.
 * Creph                    Repha
 * CMreph                  Repha if there is post-base matra.
 * ZWJ                      Zero-Width Joiner
 * ZWNJ                     Zero-Width Non-Joiner
 * Any                      Anything
 */
SRendClass::RType
SRendClass::get (SS_UCS4 u)
{
  if (u >= 0x1100 && u <= 0x115f) return JamoL;
  if (u >= 0x1160 && u <= 0x11a2) return JamoV;
  if (u >= 0x11a8 && u <= 0x11f9) return JamoT;
  switch (u)
  {
  // Jamo can use these.
  case 0x302e:
  case 0x302f:
    return Mpre;
  case 0x0901:
  case 0x0902:
  case 0x0981:
  case 0x0A81:
  case 0x0B01:
  case 0x0A82:
  case 0x0A02:
  case 0x0A01: 
  case 0x0A03: // Not VMPost
    return VMabove; 
  case 0x0A70:
  case 0x0A71: // Not VMpost
    return SMabove; 
  case 0x0952:
    return SMbelow;
  case 0x0C3E:
  case 0x0C4A: // Not Mabove
    return Mpost; 
  case 0x0962:
  case 0x0963:
  case 0x09E2:
  case 0x09E3:  // Not Mbelow
    return VMbelow; 
  case 0x200D:
    return ZWJ;
  case 0x200C:
    return ZWNJ;
  case 0x0946:
  case 0x0947:
  case 0x0948:
  case 0x06C5:
  case 0x0AC5:
    return Mpost; // Not above.
  case 0x0CC3:
    return LMpost; // It can be proven that OTF fonts treat this as LMpost.
  }
  int ct = getCharType (u);
  switch (ct)
  {
  case SD_INDIC_CONSONANT_BASE: return Cbase;
  case SD_INDIC_CONSONANT_BELOW_BASE: return Cbelow;
  case SD_INDIC_CONSONANT_POST_BASE:  return Cpost;
  case SD_INDIC_CONSONANT_DEAD:  return Cdead;
  case SD_INDIC_BOTTOM_VOWEL: return Mbelow; 
  case SD_INDIC_TOP_VOWEL: return Mabove; 
  case SD_INDIC_RIGHT_VOWEL: return Mpost; 
  case SD_INDIC_MODIFIER:  return VMpost;
  case SD_INDIC_SIGN: return SMabove;
  case SD_INDIC_INDEP_VOWEL: return VO;
  case SD_INDIC_LEFT_VOWEL: return Mpre;
  case SD_INDIC_HALANT: return Halant;
  case SD_INDIC_NUKTA: return Nukta;
  case SD_INDIC_LEFT_RIGHT_VOWEL: return Vsplit;
  }
  /* khmm */
 return Any;
}

/*!
 * \brief Split the vowel into two parts.
 * \param u is the input character
 * \param left is the left-vowel output character
 * \param right is the right-vowel output character
 * \return true if the split has been successful.
 */
bool
SRendClass::split (SS_UCS4 u, SS_UCS4* left, SS_UCS4* right)
{
  // Unfortunately not everything is LR.
  switch (u)
  {
  case 0x0B48: // LEFT-TOP 4,6
    *left = 0x0B47; *right = 0x0B56;
    return true;
  case 0x0C48: // TOP-BOTTOM 6,7
    *left = 0x0C46; *right = 0x0C56;
    return true;
  case 0x0CC0: // TOP-RIGHT 6,5
    *left = 0x0CBF; *right = 0x0CD5;
    return true;
  case 0x0CC7: // TOP-RIGHT 6,5
    *left = 0x0CC6; *right = 0x0CD5;
    return true;
  case 0x0CC8: // TOP-RIGHT 6,5
    *left = 0x0CC6; *right = 0x0CD6;
    return true;
  case 0x0CCA: // TOP-RIGHT 6,5
    *left = 0x0CC6; *right = 0x0CC2;
    return true;
  case 0x0CCB: // TOP-RIGHT 6,5
    *left = 0x0CCA; *right = 0x0CD5;
    return true;
  case 0x0D4B: // LEFT-RIGHT 4,5 - why did I leave this out?
    *left = 0x0D47; *right = 0x0D3E;
    return true;
  // Tibetan: 
  case 0x0F43: // Any, Below
    *left = 0x0F42; *right = 0x0FB7;
    return true;
  case 0x0F4D: // Any, Below
    *left = 0x0F4C; *right = 0x0FB7;
    return true;
  case 0x0F52: // Any, Below
    *left = 0x0F51; *right = 0x0FB7;
    return true;
  case 0x0F57: // Any, Below
    *left = 0x0F56; *right = 0x0FB7;
    return true;
  case 0x0F5C: // Any, Below
    *left = 0x0F5B; *right = 0x0FB7;
    return true;
  case 0x0F69: // Any, Below
    *left = 0x0F40; *right = 0x0FB5;
    return true;
  case 0x0F73: // Below, Top
    *left = 0x0F71; *right = 0x0F72;
    return true;
  case 0x0F75: // Below, Below
    *left = 0x0F71; *right = 0x0F74;
    return true;
  case 0x0F76: // Below, Top
    *left = 0x0FB2; *right = 0x0F80;
    return true;
  case 0x0F77: // Below, Further split - usage is discouraged.
    *left = 0x0FB2; *right = 0x0F81;
    return true;
  case 0x0F78: // Below, Top
    *left = 0x0FB3; *right = 0x0F80;
    return true;
  case 0x0F79: // Below, Further split - usage is discouraged.
    *left = 0x0FB3; *right = 0x0F81;
    return true;
  case 0x0F81: // Below, Top
    *left = 0x0F71; *right = 0x0F80;
    return true;
  case 0x0F93: // Below, Below
    *left = 0x0F92; *right = 0x0FB7;
    return true;
  case 0x0F9D: // Below, Below
    *left = 0x0F9C; *right = 0x0FB7;
    return true;
  case 0x0FA2: // Below, Below
    *left = 0x0FA1; *right = 0x0FB7;
    return true;
  case 0x0FA7: // Below, Below
    *left = 0x0FA6; *right = 0x0FB7;
    return true;
  case 0x0FAC: // Below, Below
    *left = 0x0FAB; *right = 0x0FB7;
    return true;
   
  }
  if (get (u) != Vsplit) return false;
  *left = getLRVowelLeft (u);
  *right = getLRVowelRight (u);
  return true;
}

/*!
 * \brief Convert type to a string.
 * \return a string representing type.
 */
const char* 
SRendClass::string (RType type)
{
  switch (type)
  {
  case  Cfirst: return "Cfirst";
  case  Clast: return "Clast";    
  case  Cpre: return "Cpre";    
  case  Cbase: return "Cbase";
  case  Chbase: return "Chbase";
  case  Cbelow: return "Cbelow";
  case  Cpost: return "Cpost";
  case  Cdead: return "Cdead";
  case  Cra: return "Cra";
  case  CMra: return "CMra";
  case  Mpre: return "Mpre";
  case  Mabove: return "Mabove";
  case  Mbelow: return "Mbelow";
  case  Mpost: return "Mpost";
  case  VMbelow: return "VMbelow";
  case  VMabove: return "VMabove";
  case  VMpost: return "VMpost";
  case  LMpost: return "LMpost";
  case  SMabove: return "SMabove";
  case  SMbelow: return "SMbelow";
  case  VO: return "VO";
  case  Nukta: return "Nukta";
  case  Halant: return "Halant";
  case  Vsplit: return "Vsplit";
  case  Creph: return "Creph";
  case  CMreph: return "CMreph";
  case  ZWJ: return "ZWJ";
  case  ZWNJ: return  "ZWNJ";
  case  Any: return  "Any";
  case  JamoL: return  "JamoL";
  case  JamoV: return  "JamoV";
  case  JamoT: return  "JamoT";
  default: break;
  }
  return "Unknown";
}
