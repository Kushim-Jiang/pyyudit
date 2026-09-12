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
#ifndef SCharClass_h
#define SCharClass_h

#include "stoolkit/STypes.h"

typedef enum 
{
  SD_CC_Xx=0,
  SD_CC_Lu,  // 01 Lu Letter, Uppercase
  SD_CC_Ll,  // 02 Ll Letter, Lowercase
  SD_CC_Lt,  // 03 Lt Letter, Titlecase
  SD_CC_Mn,  // 04 Mn Mark, Non-Spacing
  SD_CC_Mc,  // 05 Mc Mark, Spacing Combining
  SD_CC_Me,  // 06 Me Mark, Enclosing
  SD_CC_Nd,  // 07 Nd Number, Decimal Digit
  SD_CC_Nl,  // 08 Nl Number, Letter
  SD_CC_No,  // 09 No Number, Other
  SD_CC_Zs,  // 0A Zs Separator, Space
  SD_CC_Zl,  // 0B Zl Separator, Line
  SD_CC_Zp,  // 0C Zp Separator, Paragraph
  SD_CC_Cc,  // 0D Cc Other, Control
  SD_CC_Cf,  // 0E Cf Other, Format
  SD_CC_Cs,  // 0F Cs Other, Surrogate
  SD_CC_Co,  // 10 Co Other, Private Use
  SD_CC_Cn,  // 11 Cn Other, Not Assigned
  SD_CC_Lm,  // 12 Lm Letter, Modifier
  SD_CC_Lo,  // 13 Lo Letter, Other
  SD_CC_Pc,  // 14 Pc Punctuation, Connector
  SD_CC_Pd,  // 15 Pd Punctuation, Dash
  SD_CC_Ps,  // 16 Ps Punctuation, Open
  SD_CC_Pe,  // 17 Pe Punctuation, Close
  SD_CC_Pi,  // 18 Pi Punctuation, Initial quote
            // (may behave like Ps or Pe depending on usage)
  SD_CC_Pf,  // 19 Pf Punctuation, Final quote 
            // (may behave like Ps or Pe depending on usage)
  SD_CC_Po,  // 1A Po Punctuation, Other
  SD_CC_Sm,  // 1B Sm Symbol, Math
  SD_CC_Sc,  // 1C Sc Symbol, Currency
  SD_CC_Sk,  // 1D Sk Symbol, Modifier
  SD_CC_So,  // 1E So Symbol, Other
  SD_CC_MAX  // No more
} SD_CharClass;

/* BiDi class */
typedef enum 
{
  /* strong */
  SD_BC_XX=0,
  SD_BC_L, // Left-to-Right
  SD_BC_LRE, // Left-to-Right Embedding
  SD_BC_LRO, // Left-to-Right Override
  SD_BC_R, // Right-to-Left
  SD_BC_AL, // Right-to-Left Arabic
  SD_BC_RLE, // Right-to-Left Embedding
  SD_BC_RLO, // Right-to-Left Override

  /* weak */
  SD_BC_PDF, // Pop Directional Format
  SD_BC_EN,  // European Number
  SD_BC_ES, // European Number Separator
  SD_BC_ET, // European Number Terminator
  SD_BC_AN, // Arabic Number
  SD_BC_CS, // Common Number Separator
  SD_BC_NSM, // Non-Spacing Mark
  SD_BC_BN, // Boundary Neutral

  /* neutral */
  SD_BC_B,  // Paragraph Separator
  SD_BC_S, // Segment Separator
  SD_BC_WS, // Whitespace
  SD_BC_ON, // Other Neutrals
  SD_BC_MAX 

} SD_BiDiClass;

#define SD_CD_ZWSP 0x200B /* Zero width space */
#define SD_CD_ZWNJ 0x200C /* Zs */
#define SD_CD_ZWJ 0x200D  /* Cf */
#define SD_CD_ARABIC_TATWEEL 0x0640 
#define SD_CD_SYRIAC_LETTER_DALATH 0x0715 
#define SD_CD_SYRIAC_LETTER_DOTLESS_DALATH 0x0716 
#define SD_CD_SYRIAC_LETTER_RISH 0x072A 

#define SD_CD_CTRL 0
#define SD_CD_LF ((SS_UCS4)'\n')
#define SD_CD_FF ((SS_UCS4)'\f')
#define SD_CD_CR ((SS_UCS4)'\r')
#define SD_CD_TAB ((SS_UCS4)'\t')
#define SD_CD_LS 0x2028 /* line separator */
#define SD_CD_PS 0x2029 /* paragraph separator */

#define SD_CD_LRO 0x202D /* left- to-right override */
#define SD_CD_RLO 0x202E /* right-to-left override */
#define SD_CD_LRE 0x202A /* left-to-right embedding */
#define SD_CD_RLE 0x202B /* right-to-left embedding */
#define SD_CD_PDF 0x202C /* pop directional format */

#define SD_CD_LRM 0x200E /* LEFT-TO-RIGHT MARK */
#define SD_CD_RLM 0x200F /* RIGHT-TO-LEFT MARK */

/**
 * Line breaking characters in utf-8
 * NLF = one of SS_LB_DOS SS_LB_MAC SS_LB_UNIX SS_LB_NEL.
 */
#define SS_LB_DOS "\r\n"
#define SS_LB_MAC "\r"
#define SS_LB_UNIX "\n"
#define SS_LB_LS "\342\200\250"
#define SS_LB_PS "\342\200\251" /* PARAGRAPH BREAKING */
#define SS_LB_FF "\f"

#define SS_LB_LRO "\342\200\255"
#define SS_LB_RLO "\342\200\256"
#define SS_LB_LRE "\342\200\252"
#define SS_LB_RLE "\342\200\253"
#define SS_LB_PDF "\342\200\254"

/** 
 * These line breaking chars are not supported here now.
 */
#define SS_LB_NEL "\702\102"
#define SS_LB_P_VT "\013" /* PARAGRAPH BREAKING */
#define SS_LB_P_FF "\014" /* PARAGRAPH BREAKING */


extern const char* ssCharClass[SD_CC_MAX];
extern const char* ssBiDiClass[SD_BC_MAX];

SD_CharClass getCharClass(SS_UCS4 in);
SD_BiDiClass getBiDiClass(SS_UCS4 in);
SS_UCS4 getMirroredCharacter (SS_UCS4 in);

typedef enum {
   SS_PS_None=0,
   SS_PS_LF,
   SS_PS_CR,
   SS_PS_CRLF,
   SS_PS_PS
} SS_ParaSep;


typedef enum {
  SS_EmbedNone=0, SS_EmbedLeft, SS_EmbedRight
} SS_Embedding;

typedef enum {
  SS_DR_L, /* L-R character */
  SS_DR_R, /* R-L character */
  SS_DR_LE, /* inside L embedded */
  SS_DR_RE, /* inside R embedded */
  SS_DR_LO, /* inside L override */
  SS_DR_RO  /* inside R override */
} SS_DR_Dir;

#endif /*SCharClass_h*/
