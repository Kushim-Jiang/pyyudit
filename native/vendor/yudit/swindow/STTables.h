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
#ifndef  STTABLES_H
#define  STTABLES_H
/* 
 * TT_PLAT and TT_ENC from ttf.h from VFlib Library.
 * by Hirotsugu Kakugawa. Copyright: GNU.
 */

#define TT_ENC_ID_ANY                     -1
#define TT_ENC_ID_ISO_ASCII                0
#define TT_ENC_ID_ISO_10646                1
#define TT_ENC_ID_ISO_8859_1               2

#define TT_ENC_ID_MS_SYMBOL                0
#define TT_ENC_ID_MS_UNICODE               1
#define TT_ENC_ID_MS_SHIFT_JIS             2
#define TT_ENC_ID_MS_BIG5                  3
#define TT_ENC_ID_MS_RPC                   4
#define TT_ENC_ID_MS_WANSUNG               5
#define TT_ENC_ID_MS_JOHAB                 6
#define TT_ENC_ID_MS_SURROGATES            10

#define TT_ENC_ID_APPLE_DEFAULT            0
#define TT_ENC_ID_APPLE_UNICODE_1_1        1
#define TT_ENC_ID_APPLE_ISO_10646          2
#define TT_ENC_ID_APPLE_UNICODE_2_0        3

#define TT_ENC_ID_MAC_ROMAN                0
#define TT_ENC_ID_MAC_JAPANESE             1
#define TT_ENC_ID_MAC_TRADITIONAL_CHINESE  2
#define TT_ENC_ID_MAC_KOREAN               3
#define TT_ENC_ID_MAC_ARABIC               4
#define TT_ENC_ID_MAC_HEBREW               5
#define TT_ENC_ID_MAC_GREEK                6
#define TT_ENC_ID_MAC_RUSSIAN              7
#define TT_ENC_ID_MAC_RSYMBOL              8
#define TT_ENC_ID_MAC_DEVANAGARI           9
#define TT_ENC_ID_MAC_GURMUKHI             10
#define TT_ENC_ID_MAC_GUJARATI             11
#define TT_ENC_ID_MAC_ORIYA                12
#define TT_ENC_ID_MAC_BENGALI              13
#define TT_ENC_ID_MAC_TAMIL                14
#define TT_ENC_ID_MAC_TELUGU               15
#define TT_ENC_ID_MAC_KANNADA              16
#define TT_ENC_ID_MAC_MALAYALAM            17
#define TT_ENC_ID_MAC_SINHALESE            18
#define TT_ENC_ID_MAC_BURMESE              19
#define TT_ENC_ID_MAC_KHMER                20
#define TT_ENC_ID_MAC_THAI                 21
#define TT_ENC_ID_MAC_LAOTIAN              22
#define TT_ENC_ID_MAC_GEORGIAN             23
#define TT_ENC_ID_MAC_ARMENIAN             24
#define TT_ENC_ID_MAC_MALDIVIAN            25
#define TT_ENC_ID_MAC_SIMPLIFIED_CHINESE   25
#define TT_ENC_ID_MAC_TIBETAN              26
#define TT_ENC_ID_MAC_MONGOLIAN            27
#define TT_ENC_ID_MAC_GEEZ                 28
#define TT_ENC_ID_MAC_SLAVIC               29
#define TT_ENC_ID_MAC_VIETNAMESE           30
#define TT_ENC_ID_MAC_SINDHI               31
#define TT_ENC_ID_MAC_UNINTERP             32

#define TT_PLAT_ID_ANY         -1
#define TT_PLAT_ID_APPLE       0
#define TT_PLAT_ID_MACINTOSH   1
#define TT_PLAT_ID_ISO         2
#define TT_PLAT_ID_MICROSOFT   3
/*
 * The following definitions were derived from
 * Andrew Weeks's excellent font converter program.
 * Many thanks!
 */
#define SD_BYTE unsigned char
#define SD_CHAR signed char
#define SD_USHORT unsigned short
#define SD_SHORT signed short
#define SD_ULONG unsigned int
#define SD_LONG signed int
#define SD_FWORD SD_SHORT
#define SD_UFWORD SD_USHORT

#define ONOROFF 0x01
#define XSD_SHORT  0x02
#define YSD_SHORT  0x04
#define REPEAT  0x08
#define XSAME   0x10
#define YSAME   0x20
#define ARG_1_AND_2_ARE_WORDS           0x0001
#define ARGS_ARE_XY_VALUES                      0x0002
#define XY_BOUND_TO_GRID                        0x0004
#define WE_HAVE_A_SCALE                         0x0008
#define MORE_COMPONENTS                         0x0020
#define WE_HAVE_AN_X_AND_Y_SCALE        0x0040
#define WE_HAVE_A_TWO_BY_TWO            0x0080
#define WE_HAVE_INSTRUCTIONS            0x0100
#define USE_MY_METRICS                          0x0200

typedef struct longhormetric {
        SD_UFWORD  advanceWidth;
        SD_FWORD   lsb;
} LONGHORMETRIC;

typedef struct ttf_hhea {
        SD_BYTE    version[4];
        SD_SHORT   ascender, descender, lineGap;
        SD_USHORT  advnaceWidthMax;
        SD_SHORT   minLSB, minRSB, xMaxExtent;
        SD_SHORT   caretSlopeRise, caretSlopeRun;
        SD_SHORT   reserved[5];
        SD_SHORT   metricDataFormat;
        SD_USHORT  numberOfHMetrics;
} TTF_HHEA;

typedef struct ttf_dir_entry {
        char    tag[4];
        SD_ULONG   checksum;
        SD_ULONG   offset;
        SD_ULONG   length;
} TTF_DIR_ENTRY ;

typedef struct ttf_directory {
        SD_ULONG                   sfntVersion;
        SD_USHORT                  numTables;
        SD_USHORT                  searchRange;
        SD_USHORT                  entrySelector;
        SD_USHORT                  rangeShift;
        TTF_DIR_ENTRY   list;
} TTF_DIRECTORY ;

typedef struct ttf_name_rec {
        SD_USHORT  platformID;
        SD_USHORT  encodingID;
        SD_USHORT  languageID;
        SD_USHORT  nameID;
        SD_USHORT  stringLength;
        SD_USHORT  stringOffset;
} TTF_NAME_REC;

typedef struct ttf_name {
        SD_USHORT                  format;
        SD_USHORT                  numberOfNameRecords;
        SD_USHORT                  offset;
        TTF_NAME_REC    nameRecords;
} TTF_NAME ;

/*
 Flags is HEAD table - grossly ignored by yudit :)
Bit 0: Baseline for font at y=0;
Bit 1: Left sidebearing point at x=0;
Bit 2: Instructions may depend on point size; 
Bit 3: Force ppem to integer values for all internal scaler math;
may use fractional ppem sizes if this bit is clear; 
Bit 4: Instructions may alter advance width (the advance widths
might not scale linearly); 
Bits 5-10: These should be set according to Apple's specification
. However, they are not implemented in OpenType. 
Bit 11: Font data is 'lossless,' as a result of having been
compressed and decompressed with the Agfa MicroType
Express engine.
Bit 12: Font converted (produce compatible metrics)
Bit 13: Font optimised for ClearType
Bit 14: Reserved, set to 0
Bit 15: Reserved, set to 0 
*/
typedef struct ttf_head {
        SD_ULONG   version;
        SD_ULONG   fontRevision;
        SD_ULONG   checksumAdjust;
        SD_ULONG   magicNo;
        SD_USHORT  flags;
        SD_USHORT  unitsPerEm;
        SD_BYTE    created[8];
        SD_BYTE    modified[8];
        SD_FWORD   xMin, yMin, xMax, yMax;
        SD_USHORT  macStyle, lowestRecPPEM;
        SD_SHORT   fontDirection, indexToLocFormat, glyphDataFormat;
} TTF_HEAD ;


typedef struct ttf_kern {
        SD_USHORT  version, nTables;
} TTF_KERN ;

typedef struct ttf_kern_sub {
        SD_USHORT version, length, coverage;
        SD_USHORT nPairs, searchRange, entrySelector, rangeShift;
} TTF_KERN_SUB;

typedef struct ttf_kern_entry {
        SD_USHORT  left, right;
        SD_FWORD   value;
} TTF_KERN_ENTRY;

typedef struct ttf_cmap_fmt0 {
        SD_USHORT  format;
        SD_USHORT  length;
        SD_USHORT  version;
        SD_BYTE    glyphIdArray[256];
} TTF_CMAP_FMT0;

typedef struct ttf_cmap_fmt2_subheader {
    SD_USHORT  firstCode;
    SD_USHORT  entryCount;
    SD_SHORT   idDelta;
    SD_USHORT  idRangeOffset;
} TTF_CMAP_FMT2_SUBHEADER;

typedef struct ttf_cmap_fmt2 {
        SD_USHORT  format;
        SD_USHORT  length;
        SD_USHORT  language;
        SD_USHORT  subHeaderKeys[256];
        TTF_CMAP_FMT2_SUBHEADER  subHeaders[1];
} TTF_CMAP_FMT2;

typedef struct ttf_cmap_fmt4 {
        SD_USHORT  format;
        SD_USHORT  length;
        SD_USHORT  version;
        SD_USHORT  segCountX2;
        SD_USHORT  searchRange;
        SD_USHORT  entrySelector;
        SD_USHORT  rangeShift;
} TTF_CMAP_FMT4;

typedef struct ttf_cmap_fmt12_entry {
        SD_ULONG  startCharCode;
        SD_ULONG  endCharCode;
        SD_ULONG  startGlyphCode;
} TTF_CMAP_FMT12_ENTRY;

typedef struct ttf_cmap_fmt12 {
        SD_ULONG  format;
        SD_ULONG  length;
        SD_ULONG  language;
        SD_ULONG  nGroups;
        TTF_CMAP_FMT12_ENTRY entry[1]; /* nGroups */
} TTF_CMAP_FMT12;

typedef struct ttf_cmap_entry {
        SD_USHORT  platformID;
        SD_USHORT  encodingID;
        SD_ULONG   offset;
} TTF_CMAP_ENTRY;

typedef struct ttf_cmap {
        SD_USHORT                  version;
        SD_USHORT                  numberOfEncodingTables;
        TTF_CMAP_ENTRY  encodingTable[1];
} TTF_CMAP ;

typedef struct ttf_glyf {
        SD_SHORT   numberOfContours;
        SD_FWORD   xMin, yMin, xMax, yMax;
} TTF_GLYF ;

typedef struct ttf_maxp {
        SD_ULONG   version;
        SD_USHORT  numGlyphs, maxPoints, maxContours;
        SD_USHORT  maxCompositePoints, maxCompositeContours;
        SD_USHORT  maxZones, maxTwilightPoints, maxStorage;
        SD_USHORT  maxFunctionDefs, maxInstructionsDefs;
        SD_USHORT  maxSizeOfInstructions, maxComponentElements;
        SD_USHORT  maxComponentDepth;
} TTF_MAXP ;

typedef struct short_2 {
        SD_SHORT   upper;
        SD_USHORT  lower;
} SSFIX ;

typedef struct ttf_post_head {
        SD_ULONG   formatType;
        SSFIX   italicAngle;
        SD_FWORD   underlinePosition;
        SD_FWORD   underlineThickness;
        SD_ULONG   isFixedPitch;
        SD_ULONG   minMemType42;
        SD_ULONG   maxMemType42;
        SD_ULONG   minMemType1;
        SD_ULONG   maxMemType1;
        SD_USHORT  numGlyphs;
        SD_USHORT  glyphNameIndex;
} TTF_POST_HEAD ;

typedef struct ttf_oss2 {
    SD_USHORT version; /* 0x0002 */
    SD_SHORT  xAvgCharWidth; 
    SD_USHORT usWeightClass; 
    SD_USHORT usWidthClass; 
    SD_USHORT fsType; 
    SD_SHORT  ySubscriptXSize; 
    SD_SHORT  ySubscriptYSize; 
    SD_SHORT  ySubscriptXOffset; 
    SD_SHORT  ySubscriptYOffset; 
    SD_SHORT  ySuperscriptXSize; 
    SD_SHORT  ySuperscriptYSize; 
    SD_SHORT  ySuperscriptXOffset; 
    SD_SHORT  ySuperscriptYOffset; 
    SD_SHORT  yStrikeoutSize; 
    SD_SHORT  yStrikeoutPosition; 
    SD_SHORT  sFamilyClass; 
    SD_BYTE   panose[10]; 
    SD_ULONG  ulUnicodeRange1; /* Bits 0-31 */
    SD_ULONG  ulUnicodeRange2; /* Bits 32-63 */
    SD_ULONG  ulUnicodeRange3; /* Bits 64-95 */
    SD_ULONG  ulUnicodeRange4; /* Bits 96-127 */
    SD_CHAR   achVendID[4]; 
    SD_USHORT fsSelection; 
    SD_USHORT usFirstCharIndex; 
    SD_USHORT usLastCharIndex; 
    SD_SHORT  sTypoAscender; 
    SD_SHORT  sTypoDescender; 
    SD_SHORT  sTypoLineGap; 
    SD_USHORT usWinAscent; 
    SD_USHORT usWinDescent; 
    SD_ULONG  ulCodePageRange1; /* Bits 0-31 */
    SD_ULONG  ulCodePageRange2; /* Bits 32-63 */
    SD_SHORT  sxHeight; 
    SD_SHORT  sCapHeight; 
    SD_USHORT usDefaultChar; 
    SD_USHORT usBreakChar; 
    SD_USHORT usMaxContext; 
} TTF_OS2;

typedef struct _SS_CFF_Header {
    SD_BYTE major; // major version
    SD_BYTE minor; // minor version
    SD_BYTE hsize; // header size NameIndex will follow.
    SD_BYTE osize; // offset unit in bytes, both relative and absolute. 
} SS_CFF_Header;


#endif  /*STTABLES_H*/
