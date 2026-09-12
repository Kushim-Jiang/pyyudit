/** *  Yudit Unicode Editor Source File
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

#ifndef SFontCFF_h
#define SFontCFF_h

#include "swindow/SCanvas.h"
#include "swindow/STTables.h"


class SFontCFF  {
public:
    SFontCFF (void);
    ~SFontCFF ();

    bool initWithCFF(SD_BYTE* aCff);
    bool initWithCFF2(SD_BYTE* aCff2);

    void drawGlyphCFF (SCanvas* canvas, const SS_Matrix2D& matrix,
      SS_GlyphIndex glyphno);

    bool getBBOXCFF (SS_GlyphIndex glyphno,
      int* xMin, int* yMin, int* xMax, int* yMax) const;

protected:
    SCanvas* canvas;
    SS_Matrix2D matrix;

    SBinVector<double> stack;
    bool parseCharStrings (SD_BYTE* strings, SD_ULONG len);
    // 0 ok 1 return -1 nok.
    int execute (SS_SINT_32 command);
    double lastX;
    double lastY;
    unsigned int subrCount;

    unsigned int stackLimit;
    unsigned int subrLimit;

    bool charStringsBegin;
    int  commandCounter;
    int stemCount; 
    bool hintStart; 
    bool hasGlyphWidth;
    double glyphWidth; // matrix not applied.

private: 
    SD_BYTE* getType2Glyph (SS_GlyphIndex glyphno, SD_ULONG* len) const;
    bool updateLocalSubr (SS_GlyphIndex glyphno);

    SD_BYTE* cff;
    SD_BYTE* cff2;

    SString firstName;

    SD_BYTE* firstDict;
    SD_ULONG firstDictLength;

    SD_BYTE* firstCharStringsIndex;
    SD_ULONG firstCharStringsLength;

    SD_BYTE* globalSubrIndex;
    SD_ULONG globalSubrLength;

    SD_BYTE* fontDictIndex;
    SD_ULONG fontDictLength;

    SD_BYTE* fontDictSelect;

    SD_BYTE* localSubrIndex;
    SD_ULONG localSubrLength;

    double transientArray[32];
};

// CFF

// header size -  32 padding.

/* 
  INDEX
  Card16    count;  // how much data is there in index, for cff2 it is Card32   
  / if count is zero there is no osize and offset.
  Card8     osize;  // Offset is 1 2 3 4 bytes.
  Offset 	offset[count+1]
  Card8 	data[]

  DICT
  
*/
// Indexes follow header.
// NameIndex, TopDICTIndex, StringIndex, GlobalSubrIndex

// any place
// Encodings

// any place
// Charsets

// FDSelect

// CharStringsIndex, FontDictIndex, PrivateDICT, LocalSubrINDEX

#define SS_CFF_COMMAND_hstem 0x0001
// reserved
#define SS_CFF_COMMAND_vstem 0x0003
#define SS_CFF_COMMAND_vmoveto 0x0004
#define SS_CFF_COMMAND_rlineto 0x0005
#define SS_CFF_COMMAND_hlineto 0x0006
#define SS_CFF_COMMAND_vlineto 0x0007
#define SS_CFF_COMMAND_rrcurveto 0x0008
// reserved
#define SS_CFF_COMMAND_callsubr 0x000a
#define SS_CFF_COMMAND_return 0x000b
// 0x0c escape
// reserved
#define SS_CFF_COMMAND_endchar 0x000e

#define SS_CFF_COMMAND_CFF2_vsindex 0x000f
#define SS_CFF_COMMAND_CFF2_blend 0x0010

// reserved
#define SS_CFF_COMMAND_hstemhm 0x0012
#define SS_CFF_COMMAND_hintmask 0x0013
#define SS_CFF_COMMAND_cntrmask 0x0014

#define SS_CFF_COMMAND_rmoveto 0x0015
#define SS_CFF_COMMAND_hmoveto 0x0016
#define SS_CFF_COMMAND_vstemhm 0x0017
#define SS_CFF_COMMAND_rcurveline 0x0018
#define SS_CFF_COMMAND_rlinecurve 0x0019
#define SS_CFF_COMMAND_vvcurveto 0x001a
#define SS_CFF_COMMAND_hhcurveto 0x001b
#define SS_CFF_COMMAND_shortint 0x001c
#define SS_CFF_COMMAND_callgsubr 0x001d
#define SS_CFF_COMMAND_vhcurveto 0x001e
#define SS_CFF_COMMAND_hvcurveto 0x001f

#define SS_CFF_COMMAND_hflex 0x0c22
#define SS_CFF_COMMAND_flex 0x0c23
#define SS_CFF_COMMAND_hflex1 0x0c24
#define SS_CFF_COMMAND_flex1 0x0c25
// 
#define SS_CFF_COMMAND_and 0x0c03
#define SS_CFF_COMMAND_or 0x0c04
#define SS_CFF_COMMAND_not 0x0c05
#define SS_CFF_COMMAND_abs 0x0c09
//
#define SS_CFF_COMMAND_add 0x0c0a
#define SS_CFF_COMMAND_sub 0x0c0b
#define SS_CFF_COMMAND_div 0x0c0c
//
#define SS_CFF_COMMAND_neg 0x0c0e
#define SS_CFF_COMMAND_eq 0x0c0f
//
#define SS_CFF_COMMAND_drop 0x0c12
//
#define SS_CFF_COMMAND_put 0x0c14
#define SS_CFF_COMMAND_get 0x0c15
#define SS_CFF_COMMAND_ifelse 0x0c16
#define SS_CFF_COMMAND_random 0x0c17
#define SS_CFF_COMMAND_mul 0x0c18
//
#define SS_CFF_COMMAND_sqrt 0x0c1a
#define SS_CFF_COMMAND_dup 0x0c1b
#define SS_CFF_COMMAND_exch 0x0c1c
#define SS_CFF_COMMAND_index 0x0c1d
#define SS_CFF_COMMAND_roll 0x0c1e
//



#endif /* SFontCFF_h */
