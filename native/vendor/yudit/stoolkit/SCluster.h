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
 
#ifndef SCluster_h
#define SCluster_h
#include "stoolkit/STypes.h"

#define SD_YUDIT 0
/* ligature 0x8001xxxx */
#define SD_DEVANAGARI 1
/* ligature 0x8002xxxx */
#define SD_BENGALI 2
/* ligature 0x8003xxxx */
#define SD_GURMUKHI 3
/* ligature 0x8004xxxx */
#define SD_GUJARATI 4
/* ligature 0x8005xxxx */
#define SD_ORIYA 5
/* ligature 0x8006xxxx */
#define SD_TAMIL 6
/* ligature 0x8007xxxx */
#define SD_TELUGU 7
/* ligature 0x8008xxxx */
#define SD_KANNADA 8
/* ligature 0x8009xxxx */
#define SD_MALAYALAM 9
/* ligature 0x800axxxx */
#define SD_SINHALA 10
/* Hangul */
#define SD_HANGUL_JAMO 11

/* Precomposed Unicode JAMOS */
#define SD_HANGUL_PREC 12

/* Thai, Lao and Tibetan will need positioning */
#define SD_THAI 13
#define SD_LAO 14
#define SD_TIBETAN 15

/* complex combining ligature 
  a ligature that has combining marks in the middle  */
#define SD_COMBINING_LIGATURE 16

/* bengali Ra+Ya in the beginning of the word. */
#define SD_BENGALI_BEGIN 17

/* Old Hungarian */
#define SD_PUA_ROVAS 18

// AKA OLD HUNGARIAN from Unicode 7.0.0
#define SD_ROVASIRAS 19

#define SD_REGIONAL_INDICATOR_SYMBOL 20

#define SD_SCRIPT_MAX 21

/* Arabic and Syriac shapes A000[1234]000. */
/* if we are not this lucky we would need one for each */
#define SD_AS_SHAPES 0x2000

/* Escape 9fffffxx */
#define SD_AS_LITERAL 0x1fff

/* get the name of OTF font shaping feature name */
const char* getShapeCode (unsigned int icode);

/**
 * This one generates ligature on the non-displayable cluster 
 */
unsigned int getCluster (const SV_UCS4& ucs4, unsigned int index, 
   SV_UCS4* ret, int* finished=0);

SS_UCS4 addCombiningLigature (const SS_UCS4* unicode, unsigned int ul,
  const SS_UCS4* ligAndMarks, unsigned int cl);

void putLigatureUnicode (SS_UCS4 ligature, const SS_UCS4* code, 
 unsigned int size);

void putLigatureCluster (SS_UCS4 ligature, const SS_UCS4* code, 
 unsigned int size);

unsigned int getLigatureUnicode (SS_UCS4 ligature, SS_UCS4* buffer);
unsigned int getLigatureCluster (SS_UCS4 ligature, SS_UCS4* buffer);

bool isLigature (SS_UCS4 _comp);

/* get script name or null */
const char* getLigatureScript (SS_UCS4 comp);
int   getLigatureScriptCode (SS_UCS4 comp);

int getCharType (SS_UCS4 unchar);

#define SD_JAMO_X 0 /* non-jamo */
#define SD_JAMO_L 1 /* choseong:  leanding consonants or syllable initials */
#define SD_JAMO_V 2 /* jungseong: vowels or syllable-peak characters */
#define SD_JAMO_T 3 /* jongseong: trailing consonants or syllable final characters  */
int getJamoClass (SS_UCS4 uc);

/* get the index */
int getUnicodeScript (SS_UCS4 comp);
bool isCoveredScipt (SS_UCS4 comp, int scipt);

SS_UCS4 getHalant (int index);

SS_UCS4 getLRVowelLeft (SS_UCS4 u);
SS_UCS4 getLRVowelRight (SS_UCS4 u);
/**
 * Some encoders will give back values in yudit ligature range.
 * Expand those ligatures.
 */
void expandYuditLigatures (SV_UCS4* decd);

/* 1,2 is rovas, 3 is already a cluster */
int getRovasType (SS_UCS4 chr);

int getPUARovasType (SS_UCS4 chr);


#define SD_INDIC_CONSONANT_BELOW_BASE 0x1
#define SD_INDIC_HALANT 0x2
#define SD_INDIC_INDEP_VOWEL 0x3
#define SD_INDIC_LEFT_VOWEL 0x4
#define SD_INDIC_RIGHT_VOWEL 0x5
#define SD_INDIC_TOP_VOWEL 0x6
#define SD_INDIC_BOTTOM_VOWEL 0x7
#define SD_INDIC_SIGN 0x8
#define SD_INDIC_ZWJ 0x9
#define SD_INDIC_ZWNJ 0xa

#define SD_INDIC_NUKTA 0xc
#define SD_INDIC_MODIFIER 0xd
#define SD_INDIC_LEFT_RIGHT_VOWEL 0xe

#define SD_INDIC_CONSONANT_BASE 0xf
#define SD_INDIC_CONSONANT_POST_BASE 0x10
#define SD_INDIC_CONSONANT_DEAD 0x11

#endif /* SCluster_h */
