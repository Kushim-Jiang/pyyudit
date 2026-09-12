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
 
#ifndef STypes_h
#define STypes_h

#define SD_YUDIT_VERSION "3.1.0"

#define SB_YUDIT_MAX_SCALE 3.0

/* search this many subdirectories */
#define SB_YUDIT_SUBDIR_LEVEL 5

// Speed tests: 5: 655ms 8: 1394ms / 1000 complex kanji.
#define SS_RASTERIZER_OVERSAMPLE 8

// Get the SS_WORD16_t and u_int_32
#include <sys/types.h>
#include "stoolkit/SBinVector.h"
#include "stoolkit/SStringVector.h"
#include <string.h>

#ifdef HAVE_UTYPES
 typedef u_int8_t SS_WORD8;
 typedef u_int16_t SS_WORD16;
 typedef u_int32_t SS_WORD32;
 typedef int32_t  SS_SINT_32
 typedef int16_t  SS_SINT_16

#ifdef HAVE_LONG_LONG
  typedef u_int64_t SS_WORD64;
#else /* HAVE_LONG_LONG */
  typedef u_int32_t SS_WORD64;
#endif /* HAVE_LONG_LONG */

#else /* HAVE_UTYPES */
 typedef unsigned char SS_WORD8;
 typedef unsigned short SS_WORD16;
 typedef unsigned int SS_WORD32;
 typedef int  SS_SINT_32;
 typedef short  SS_SINT_16;

#ifdef HAVE_LONG_LONG
  typedef unsigned long long  SS_WORD64;
#else /* HAVE_LONG_LONG */
  typedef unsigned long  SS_WORD64;
#endif /* HAVE_LONG_LONG */

#endif /* HAVE_UTYPES */

typedef SS_WORD16 SS_UCS2;
typedef SS_WORD32 SS_UCS4;

typedef SBinVector<SS_UCS2> SV_UCS2;
typedef SBinVector<SS_UCS4> SV_UCS4;

/* processors calculate with integers fast */
typedef unsigned int SS_UINT;
typedef int SS_INT;

typedef SBinVector<SS_UINT> SV_UINT;
typedef SBinVector<SS_INT> SV_INT;

/*
 * SStringVector column0 = SStringTable[0];
 */
typedef SVector<SStringVector> SStringTable;

typedef int SAlignment;

typedef SS_WORD16 SS_GlyphIndex;
typedef SBinVector<SS_GlyphIndex> SV_GlyphIndex;

typedef  SBinVector<int> SSyntaxRow;
typedef  SBinVector<SSyntaxRow*> SSyntaxData;
typedef  SBinVector<SV_UCS4*> SUnicodeData;

#define SD_Left ((SAlignment)-1)
#define SD_Right ((SAlignment)1)
#define SD_Center ((SAlignment)0)

/* char can be only positive on some platforms */
#define SD_NOSHAPE ((char)0x7f)

/* This many lines are pre-expanded around cursor, for efficiencey */
#define SD_PRE_EXPAND 50
#define DEBUG_PARSER 0

#define SD_MATCH_EOD 0x7ffffff
// to provide timer based solution
#define SD_MATCH_AGAIN 0x7fffffe

#if defined NO_SNPRINTF && !defined snprintf 
// This may look unsafe but we never pass a buffer that is too small.
// For portablity I dont use ## __VA_ARGS__, there is always at least
// one argument in our codebase.
#define snprintf(str, size, fmt, ...) sprintf(str, fmt,  __VA_ARGS__)
#endif


#endif /* STypes_h */
