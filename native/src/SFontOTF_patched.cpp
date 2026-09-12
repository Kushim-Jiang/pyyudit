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

#include "swindow/SFontTTF.h"
#include "swindow/STTables.h"
#include "yudit_trace.h"

/* Forward declaration for trace tag copy helper */
static void copy_raw_tag(char dest[5], const void* raw_tag);

#include "stoolkit/SString.h"
#include "stoolkit/SExcept.h"
/*
 * ntohl 
 */
#ifndef USE_WINAPI
#include <netinet/in.h>
#else
#include <winsock.h>
#endif

#include <stdio.h>
#include <ctype.h>

#include "stoolkit/SCluster.h"

#define SD_BYTE unsigned char
#define SD_CHAR signed char
#define SD_USHORT unsigned short
#define SD_SHORT signed short
#define SD_ULONG unsigned int
#define SD_LONG signed int
#define SD_FWORD SD_SHORT
#define SD_UFWORD SD_USHORT

/* 
 * If you have a font that has some substitution 
 * and it is not supported you can get notified if 
 * you set this to 1.
 */
#define PRINT_UNSUPPORTED 0
/*
 * For some (2) format the code is commented out
 * because there was no such font that had it,
 * Set this to 1 if you want to debug them.
 */
#define PRINT_UNDEBUGGED 0

/*
 * Recommended value 0
 */
#define USE_UNTESTED_CODE 0

/* OTF */
static const SString SS_TB_GSUB("GSUB");
static const SString SS_TB_GPOS("GPOS");
static const SString SS_TB_GDEF("GDEF");

/*----------------------------------------------------------------------------
 * True Type Font handling routines
 *--------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------
 * kern: Kerning Table
 *--------------------------------------------------------------------------*/
typedef struct _kern_head_ms {
  SD_USHORT  version;
  SD_USHORT  nTables;
} KERN_HEAD_MS;

typedef struct _kern_subtable_ms {
  SD_USHORT  version;
  SD_USHORT  length;
  SD_USHORT  coverage;
} KERN_SUBTABLE_MS;

/* format 0: horizontal */
typedef struct _kern_horizontal_ms {
  SD_USHORT  nPairs;
  SD_USHORT  searchRange;
  SD_USHORT  entrySelector;
  SD_USHORT  rangeShift;
} KERN_HORIZONTAL_MS;

typedef struct _kern_pairs_ms {
  SD_USHORT  left;
  SD_USHORT  right;
  SD_USHORT  value; /* FWORD */
} KERN_PAIRS_MS;

/*----------------------------------------------------------------------------
 * Open Type Font handling routines
 *--------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * GSUB: Glyph Substitution Table
 *--------------------------------------------------------------------------*/
 
typedef struct _OTF_Feature
{
  SD_USHORT offset;
  SD_USHORT count;
  SD_USHORT record[1];
} OTF_Feature;

typedef struct _OTF_FeatureRecord
{
  char    tag[4];
  SD_USHORT  offset;
} OTF_FeatureRecord;

typedef struct _OTF_FeatureList
{
  SD_USHORT count;
  OTF_FeatureRecord record[1];
} OTF_FeatureList;

typedef struct _OTF_Lookup
{
  SD_USHORT type;
  SD_USHORT flag;
  SD_USHORT count;
  SD_USHORT subtable[1];
} OTF_Lookup;

typedef struct _OTF_LookupList
{
  SD_USHORT count;
  SD_USHORT record[1];
} OTF_LookupList;

/*
 * OpenType "Extension" lookup subtable (GSUB type 7 / GPOS type 9).
 * It restates the real lookup type and points at the actual subtable.
 * Arial, Segoe UI, Noto and most modern fonts wrap *every* lookup this way,
 * so without unwrapping no GSUB/GPOS lookup ever matches.
 */
typedef struct _OTF_LookupExtension
{
  SD_USHORT format;              /* 1 */
  SD_USHORT extensionLookupType; /* real lookup type */
  SD_ULONG  extensionOffset;     /* 32-bit, from the start of this table */
} OTF_LookupExtension;

/**
 * The real lookup type of a lookup, with Extension wrappers unwrapped.
 */
static SD_USHORT
lookupEffectiveType (OTF_Lookup* ltable)
{
  SD_USHORT t = ntohs (ltable->type);
  if (t != 7 && t != 9) return t;   /* not an extension lookup */
  SD_USHORT offset = ntohs (ltable->subtable[0]);
  const OTF_LookupExtension* ext =
     (const OTF_LookupExtension*)((char*)ltable + offset);
  if (ntohs (ext->format) != 1) return t;
  return ntohs (ext->extensionLookupType);
}

/**
 * Offset of subtable `k` relative to `ltable`, with Extension wrappers
 * resolved.  Every consumer does `(char*)ltable + offset`, so folding the
 * 32-bit extension offset in here makes the unwrapping transparent.
 *
 * NOTE: the result must be held in a 32-bit variable - extension subtables
 * routinely live more than 64k away from the lookup that references them.
 */
static SD_ULONG
lookupSubtableOffset (OTF_Lookup* ltable, unsigned int k)
{
  SD_ULONG offset = ntohs (ltable->subtable[k]);
  SD_USHORT t = ntohs (ltable->type);
  if (t != 7 && t != 9) return offset;   /* not an extension lookup */
  const OTF_LookupExtension* ext =
     (const OTF_LookupExtension*)((char*)ltable + offset);
  if (ntohs (ext->format) != 1) return offset;
  return offset + ntohl (ext->extensionOffset);
}

typedef struct _OTF_RangeRecord
{
  SD_USHORT start;
  SD_USHORT end;
  SD_USHORT index;
} OTF_RangeRecord;

typedef struct _OTF_CoverageFormat2
{
  SD_USHORT format;
  SD_USHORT count;
  OTF_RangeRecord record[1];
} OTF_CoverageFormat2;

typedef struct _OTF_CoverageFormat1
{
  SD_USHORT format;
  SD_USHORT count;
  SD_USHORT id[1]; /* numerical order */
} OTF_CoverageFormat1;

/* Just header */
typedef struct _OTF_CoverageFormat
{
  SD_USHORT format;
  SD_USHORT count;
} OTF_CoverageFormat;

/*
 * IF I have more time I will debug this 
 */
typedef struct _OTF_LangSys
{
  SD_USHORT  lookupOrder; /* 0 for 1.0 */
  SD_USHORT  reqFeatureIndex;
  SD_USHORT  featureCount;
  SD_USHORT  featureIndex[1]; /* or more  - index in feature */
} OTF_LangSys;

typedef struct _OTF_LangSysRecord
{
  char        tag[4];
  SD_USHORT      offsetFromScript;
} OTF_LangSysRecord;

typedef struct  _OTF_Script
{
  SD_USHORT             defaultLangSys; /* may be 0 */
  SD_USHORT             langSysCount;
  OTF_LangSysRecord  langSysRecord[1]; /* or more */
} OTF_Script;

typedef struct _OTF_ScriptRecord
{
  char               tag[4];
  SD_USHORT             offset;
} OTF_ScriptRecord;

typedef struct _OTF_ScriptList 
{
  SD_USHORT              scriptCount;
  OTF_ScriptRecord    records[1];
} OTF_ScriptList;

/* Table for Single Substitution */

/* Just header */
typedef struct _OTF_SingleSubstFormat
{
  SD_USHORT format;
  SD_USHORT coverage;
} OTF_SingleSubstFormat;

typedef struct _OTF_SingleSubstFormat1
{
  SD_USHORT format;
  SD_USHORT coverage;
  SD_SHORT  deltaGlyphID;
} OTF_SingleSubstFormat1;

typedef struct _OTF_SingleSubstFormat2
{
  SD_USHORT format;
  SD_USHORT coverage;
  SD_USHORT count;
  SD_USHORT substitute[1]; /* count */
} OTF_SingleSubstFormat2;

/* Table for Multiple Substitution  */
typedef struct _OTF_LigatureSusbstFormat1
{
  SD_USHORT format;
  SD_USHORT coverage;
  SD_USHORT count;
  SD_USHORT offset[1];
} OTF_LigatureSusbstFormat1;

typedef struct _SubstLookupRecord
{
  SD_USHORT sequenceIndex; /* Index into current sequence */
  SD_USHORT lookupListIndex; /* Lookup to apply to that */
} SubstLookupRecord;

/* Table for ChainContextSubstFormat3. They come in this order.  */
typedef struct _OTF_ChainContextSubstFormat3_Backtrack
{
  SD_USHORT format;
  SD_USHORT backtrackGlyphCount;
  SD_USHORT coverage[1]; /* real size: backtrackGlyphCount */
} OTF_ChainContextSubstFormat3_Backtrack;

typedef struct _OTF_ChainContextSubstFormat3_Input
{
  SD_USHORT inputGlyphCount;
  SD_USHORT coverage[1]; /* real size: inputGlyphCount */
} OTF_ChainContextSubstFormat3_Input;

typedef struct _OTF_ChainContextSubstFormat3_Lookahead
{
  SD_USHORT lookaheadGlyphCount;
  SD_USHORT coverage[1]; /* real size: lookadeadGlyphCount */
} OTF_ChainContextSubstFormat3_Lookahead;

typedef struct _OTF_ChainContextSubstFormat3_Subst
{
  SD_USHORT substGlyphCount;
  SubstLookupRecord substLookupRecord[1]; /* real size: substGlyphCount */
} OTF_ChainContextSubstFormat3_Subst;

/* Table for ChainContextSubstFormat3. They come in this order.  */
typedef struct _OTF_ChainContextSubstFormat2_Backtrack
{
  SD_USHORT backtrackGlyphCount;
  SD_USHORT coverage[1]; /* real size: backtrackGlyphCount */
} OTF_ChainContextSubstFormat2_Backtrack;

typedef struct _OTF_ChainContextSubstFormat2_Input
{
  SD_USHORT inputGlyphCount;
  SD_USHORT coverage[1]; /* real size: inputGlyphCount - 1 */
} OTF_ChainContextSubstFormat2_Input;

typedef struct _OTF_ChainContextSubstFormat2_Lookahead
{
  SD_USHORT lookaheadGlyphCount;
  SD_USHORT coverage[1]; /* real size: lookadeadGlyphCount */
} OTF_ChainContextSubstFormat2_Lookahead;

typedef struct _OTF_ChainContextSubstFormat2_Subst
{
  SD_USHORT substGlyphCount;
  SubstLookupRecord substLookupRecord[1]; /* real size: substGlyphCount */
} OTF_ChainContextSubstFormat2_Subst;

typedef struct _OTF_ChainSubClassSet
{
   SD_USHORT chainSubClassRuleCnt;
    // Backtrack + input + lookahead + subst 
   SD_USHORT chainSubClassRule[1]; // chainSubClassCnt.
} OTF_ChainSubClassSet;

typedef struct _OTF_SubClassRule
{
   SD_USHORT glyphCount;
   SD_USHORT substCount;
   SD_USHORT clazz[1]; // size of glyphCount -1
   // followed by SubstLookupRecord
} OTF_SubClassRule;

typedef struct _OTF_SubClassSet
{
   SD_USHORT subClassRuleCnt;
    // Backtrack + input + lookahead + subst 
   SD_USHORT subClassRule[1]; // chainSubClassCnt.
} OTF_SubClassSet;

typedef struct _OTF_ClassDefFormat1
{
  SD_USHORT format;  // 1.
  SD_USHORT startGlyph;
  SD_USHORT glyphCount;
  SD_USHORT classValueArray[1];  // glyphCount
} OTF_ClassDefFormat1;

typedef struct _OTF_ClassRangeRecord
{
  SD_USHORT startGlyph;
  SD_USHORT endGlyph;
  SD_USHORT classValue;
} OTF_ClassRangeRecord;

typedef struct _OTF_ClassDefFormat2
{
  SD_USHORT            format;  // 2.
  SD_USHORT            classRangeCount;
  OTF_ClassRangeRecord classRangeRecord[1];  // classRangeCount
} OTF_ClassDefFormat2;

typedef struct _OTF_ChainContextSubstFormat2
{
  SD_USHORT format;
  SD_USHORT coverage;
  SD_USHORT backtrackClassDef;
  SD_USHORT inputClassDef;
  SD_USHORT lookaheadClassDef;
  SD_USHORT chainSubClassSetCnt;
  SD_USHORT chainSubClassSet[1]; // chainSubClassCnt.
} OTF_ChainContextSubstFormat2;

typedef struct _OTF_ContextSubstFormat2
{
  SD_USHORT format;
  SD_USHORT coverage;
  SD_USHORT classDef;
  SD_USHORT subClassSetCnt;
  SD_USHORT subClassSet[1]; // chainSubClassCnt.
} OTF_ContextSubstFormat2;

typedef struct _OTF_Ligature
{
  SD_USHORT glyph;
  SD_USHORT count;
  SD_USHORT component[1];
} OTF_Ligature;

typedef struct _OTF_LigatureSet
{
  SD_USHORT count;
  SD_USHORT offset[1];
} OTF_LigatureSet;

typedef struct _OTF_AlternateSet
{
  SD_USHORT glyphCount;
  SD_USHORT alternate[1];
} OTF_AlternateSet;

typedef struct _OTF_AlternateSubstFormat1
{
  SD_USHORT format;
  SD_USHORT coverage;
  SD_USHORT alternateCount;
  SD_USHORT alternateSet[1];
} OTF_AlternateSubstFormat1;

typedef struct gsub_head {
	SD_ULONG   version;
	SD_USHORT 	scriptList;
	SD_USHORT	featureList;
	SD_USHORT	lookupList;
} GSUB_HEAD;

/*----------------------------------------------------------------------------
 * GDEF: Glyph Definition Table
 *--------------------------------------------------------------------------*/
typedef struct gdef_head {
  SD_ULONG  version;
  SD_USHORT glyphClassDef;
  SD_USHORT attachList;
  SD_USHORT ligatureCaretList;
  SD_USHORT markAttachClassDef;
} GDEF_HEAD;

typedef struct _OTF_AttachPoint {
  SD_USHORT pointCount;
  SD_USHORT pointIndex[1];
} OTF_AttachPoint;

typedef struct _OTF_AttachList {
  SD_USHORT coverage;
  SD_USHORT glyphCount;
  SD_USHORT attachPoint[1];
} OTF_AttachList;

/*----------------------------------------------------------------------------
 * GPOS: Glyph Positioning Table
 *--------------------------------------------------------------------------*/
typedef struct gpos_head {
  SD_ULONG  version;
  SD_USHORT scriptList;
  SD_USHORT featureList;
  SD_USHORT lookupList;
} GPOS_HEAD;

/* 
 * FIXME: I could not find any spec 
 * I just guessed this much be the Achnor 
 * Structure.
 */
typedef struct _OTF_Anchor
{
  SD_USHORT format; /* this can define many formats */
  SD_SHORT x;
  SD_SHORT y;
  /* till here is it format one */
  SD_USHORT anchorPoint; /* index to glyph contour pint */
  /* till here is it format two */
  SD_USHORT xDeviceTable; /* index to device table can be null */
  SD_USHORT yDeviceTable; /* index to device table can be null */
} OTF_Anchor;

typedef struct _OTF_MarkRecord
{
  SD_USHORT   markClass;
  SD_USHORT   markAnchor; /* offset from MarkArray */
} OTF_MarkRecord;

typedef struct _OTF_MarkArray
{
  SD_USHORT      markCount;
  OTF_MarkRecord markRecord[1]; /* basecount */
} OTF_MarkArray;


/*
 * FIXME: I am using this for Mark2Recored too.
 * Spec does not say it is good or bad 
 */
typedef struct _OTF_BaseRecord
{
  SD_USHORT  baseAnchor[1]; /* This is not one but class size  */
} OTF_BaseRecord;

/*
 * FIXME: I am using this for d Mark2Array too.
 * Spec does not say it is good or bad 
 */
typedef struct _OTF_BaseArray
{
  SD_USHORT baseCount;
  OTF_BaseRecord baseRecord[1]; /* basecount */
} OTF_BaseArray;

/* Table for Mark to Base Substitution  */
typedef struct _OTF_MarkBasePosFormat1
{
  SD_USHORT format;
  SD_USHORT markCoverage;
  SD_USHORT baseCoverage;
  SD_USHORT classCount;
  SD_USHORT markArray; /* offset from beginning of the table */
  SD_USHORT baseArray; /* offset from beginning of the table */
} OTF_MarkBasePosFormat1;

/* Table for Mark to Mark Substitution  */
typedef struct _OTF_MarkMarkPosFormat1
{
  SD_USHORT format;
  SD_USHORT mark1Coverage;
  SD_USHORT mark2Coverage;
  SD_USHORT classCount;
  SD_USHORT mark1Array; /* offset from beginning of the table */
  SD_USHORT mark2Array; /* offset from beginning of the table */
} OTF_MarkMarkPosFormat1;

/* Pair Adjustment */
typedef struct _OTF_PairAdjustmentFormat1
{
  SD_USHORT format;
  SD_USHORT coverage;
  SD_USHORT valueFormat1;
  SD_USHORT valueFormat2;
  SD_USHORT pairSetCount;
  SD_USHORT pairSetOffset; /* offset from beginning of the table. */
} OTF_PairAdjustmentFormat1;

/* Chained Adjustment */
typedef struct _OTF_ChainedAdjustmentFormat1
{
  SD_USHORT format;
  SD_USHORT coverage;
  SD_USHORT chainPosRuleSetCount;
  SD_USHORT chainPosRuleSet[1]; /* offset from beginning of the table. */
} OTF_ChainedAdjustmentFormat1;

/* Chained Adjustment */
typedef struct _OTF_ChainedAdjustmentFormat2
{
  SD_USHORT format;
} OTF_ChainedAdjustmentFormat2;

// We will support only these
#define SD_X_PLACEMENT 1
#define SD_Y_PLACEMENT 2
#define SD_X_ADVANCE 4
#define SD_Y_ADVANCE 8

typedef struct _OTF_Class2Record
{
  SD_SHORT value1;
  SD_SHORT value2;
} OTF_Class2Record;

typedef struct _OTF_Class1Record
{
  OTF_Class2Record class2Record[1]; /* Class2Count */
} OTF_Class1Record;

/* Pair Adjustment */
typedef struct _OTF_PairAdjustmentFormat2
{
  SD_USHORT format;
  SD_USHORT coverage;
  SD_USHORT valueFormat1;
  SD_USHORT valueFormat2;
  SD_USHORT classDef1;
  SD_USHORT classDef2;
  SD_USHORT class1Count;
  SD_USHORT class2Count;
  OTF_Class1Record classRecord[1]; // class1Count.
} OTF_PairAdjustmentFormat2;

/*  structure for a sequence of basic Jamos  and the corresponding 
    Jamo cluster used in 'mslvt' TTF's.*/

#define MAX_BAS_JAMOS  3  // max. # of basic jamos forming a cluster jamo

typedef struct
{
  SS_UCS4 seq[MAX_BAS_JAMOS];
  SS_UCS4 liga;
} _OTF_mslvtJamo;



static unsigned int  getOTFFeature (OTF_Feature* feat, 
  OTF_LookupList* lookuplist, unsigned int substtype,
  const SString& fontname, const SS_GlyphIndex* chars,
   unsigned int length, SS_GlyphIndex* out, const char* feature_tag = 0);

static unsigned int
doChainContextSubstitution (const SString& name, 
  OTF_LookupList* lookupList, OTF_Lookup* ltable,
  const SS_GlyphIndex* chars, unsigned int liglen, 
  SS_GlyphIndex* outchar);

static unsigned int
doContextSubstitution (const SString& name, 
  OTF_LookupList* lookupList, OTF_Lookup* ltable,
  const SS_GlyphIndex* chars, unsigned int liglen, 
  SS_GlyphIndex* outchar);

static unsigned int
doAlternateSubstitution (const SString& name, OTF_Lookup* ltable,
  const SS_GlyphIndex* chars, unsigned int liglen, SS_GlyphIndex* outchar);

static unsigned int
doSingleSubstitution (const SString& name, OTF_Lookup* ltable,
  const SS_GlyphIndex* chars, unsigned int liglen, SS_GlyphIndex* outchar);

static unsigned int
doLigatureSubstitution (const SString& name, OTF_Lookup* ltable,
  const SS_GlyphIndex* chars, unsigned int liglen, SS_GlyphIndex* outchar);

static bool
processGPOSFeature (const SString& name, 
  OTF_Feature* feat, int substtype, OTF_LookupList* lookupList, 
  const SS_GlyphIndex* gv, unsigned int gvsize, 
  int* xpos, int* ypos, const char* feature_tag = 0);

static bool
doChainedPos (const SString& name, 
  OTF_LookupList* lookupList, OTF_Lookup* ltable,
  const SS_GlyphIndex* gvarray, unsigned int gvsize,
  int* xpos, int* ypos);

static bool
doPairAdjustment (const SString& name, OTF_Lookup* ltable, 
  const SS_GlyphIndex* gvarray, unsigned int gvsize,
  int* xpos, int* ypos);

static bool
doMarkToBase (const SString& name, OTF_Lookup* ltable, 
  const SS_GlyphIndex* gvarray, unsigned int gvsize,
  int* xpos, int* ypos);

static bool
doMarkToMark (const SString& name, OTF_Lookup* ltable, 
  const SS_GlyphIndex* gvarray, unsigned int gvsize,
  int* xpos, int* ypos);

static OTF_LangSys* getNextOTFLanguageSystem (const SString fontname, 
  GSUB_HEAD* gsubh, const SString& _script, unsigned int* from);

static SD_USHORT
getCoverageIndex  (OTF_CoverageFormat* coverageFormat, SS_GlyphIndex gi);

// OTF_ClassDefFormat1,
// OTF_ClassDefFromat2
static SD_USHORT glyphClass (char* def, SD_USHORT glyph);


static int 
get_jamo_class2(SS_UCS4 uc);

static int 
jamo_srch_repl(_OTF_mslvtJamo *cluster, SS_UCS4 *in, int *len);

static bool 
mslvtXform (const SS_UCS4* in, SS_UCS4* med, int *len);

/**
 * return true if this font can do OTF ligatures
 */
bool
SFontTTF::hasOTFLigatures()
{
  return (tables[SS_TB_GSUB] != 0);
}

/**
 * Use feature 1 Single Substitution and substitute glyph 
 * @param feature is one of
 *  <ul>
 *   <li> init </li>
 *   <li> isol </li>
 *   <li> fina </li>
 *   <li> medi </li>
 *  </ul>
 * @return 0 on failure.
 */
SS_GlyphIndex
SFontTTF::substituteOTFGlyph (const char* feature, SS_GlyphIndex g)
{
  SS_GlyphIndex out = 0;
  unsigned int ligs = getOTFLigature ("arab", feature, &g, 1, &out, 1);
  if (ligs) return out;
  ligs = getOTFLigature ("syrc", feature, &g, 1, &out, 1);
  if (ligs) 
  {
     return out;
  }
  return 0;
}

SString debugTag ;
/**
 * Opent Type fonts may have GSUB tables that define
 *     Glyph substitutions.
 * @param _script is the script code of the table
 *    <ul>
 *      <li> arab - Arabic </li>
 *      <li> deva - Devanagari </li>
 *      <li> beng - Bengali </li>
 *      <li> guru - Gurmukhi </li>
 *      <li> gujr - Gujarati </li>
 *      <li> orya - Oriya </li>
 *      <li> taml - Tamil </li>
 *      <li> telu - Telugu </li>
 *      <li> knda - Kannada </li>
 *      <li> mlym - Malayalam </li>
 *      <li> sinh - Shinhala </li>
 *      <li> jamo - Jamo (Hangul) </li>
 *      <li> thai - Thai </li>
 *      <li> lao  - Lao (padded with space at the end)</li>
 *      <li> tibt - Tibetan </li>
 *    </ul>
 * @param featurelist is a comma separated list of features.
 *      If the list starts with '!', like in '!blws,psts" it
 *      means that the features (blws,psts) should not be used.
 * @param chars are the input glyphs.
 * @param liglen is the length of chars array
 * @param out is an array for output glyphs.
 *      for all substitutions it is size 1 except for 
 *      Chaining Context Substitution, where the size of this
 *      array must be liglen.
 * @param substtype in one of
 *   <ul>
 *      <li> 1 Single Substitution </li>
 *      <li> 4 Lingature Substitution </li>
 *      <li> 6 Chaining Context Substitution </li>
 *   </ul>
 * @return 0 if no substitutions have been made or 
 *      length of ligatures substituted. 
 *      The result is always 1 glyph except for Chaining Context Substitution
 *      where is is always liglen. 
 */
unsigned int 
SFontTTF::getOTFLigature (const char* _script, const char* _featurelist,
    const SS_GlyphIndex* chars, unsigned int liglen, 
    SS_GlyphIndex* out, unsigned int substtype)
{
  *out = 0;
  GSUB_HEAD* gsubh = (GSUB_HEAD*) tables[SS_TB_GSUB];
  if (gsubh == 0 || ntohl (gsubh->version) != 0x00010000)
  {
    return 0;
  }
  bool nonfeature = false;
  SBinHashtable<int> features;
  if (_featurelist != 0 && _featurelist[0] != 0 && _featurelist[1] != 0)
  {
    SString f = SString(_featurelist);
    if (f[0] == '!')
    {
      nonfeature = true;
      f.remove (0);
    }
    SStringVector v(f);
    for (unsigned int i=0; i<v.size(); i++)
    {
      SString s(v[i]);
      if (s.size()==4)
      {
        features.put (s, 1);
      }
    }
  }

  int ofeat = ntohs(gsubh->featureList);
  OTF_FeatureList*  featureList = (OTF_FeatureList*) ((SD_BYTE*)gsubh + ofeat);
  SD_USHORT fcount = ntohs (featureList->count);

  int olookup = ntohs(gsubh->lookupList);
  OTF_LookupList*  lookupList = (OTF_LookupList*) ((SD_BYTE*)gsubh + olookup);
  if (_script == 0)
  {
    for (unsigned int i=0; i< fcount; i++)
    {
       SString tag (featureList->record[i].tag, 4);
       debugTag = tag;
       SD_USHORT loffset = ntohs (featureList->record[i].offset);
       /* got or not omitted */
       if (_featurelist)
       {
         if (nonfeature)
         {
            if (features.get(tag)) continue;
         }
         else
         {
            if (!features.get(tag)) continue;
         }
       }
       OTF_Feature* feat = (OTF_Feature*) ((char*)featureList+loffset);
       unsigned int ind = getOTFFeature (feat, lookupList, 
         substtype, name, chars, liglen, out, tag.array());
       if (ind)
       {
         return ind;
       }
    }
    return 0;
  }
  unsigned int next = 0; 
  SString script (_script);
  OTF_LangSys* lsys = 0;
  while ((lsys = getNextOTFLanguageSystem (name, gsubh, script, &next))!=0)
  {
    SD_USHORT fcount = ntohs (lsys->featureCount);
    /* index lookupList through lsys->featureIndex */
    for (unsigned int i=0; i< fcount; i++)
    {
      unsigned int index = ntohs (lsys->featureIndex[i]) ;
      SString tag (featureList->record[index].tag, 4);
      if (_featurelist)
      {
        if (nonfeature)
        {
          if (features.get(tag)) continue;
        }
        else
        {
          if (!features.get(tag)) continue;
        }
      }

      SD_USHORT loffset = ntohs (featureList->record[index].offset);
      /* tags will have mystic ligature names and stuff like that - 
         don't check */
      OTF_Feature* feat = (OTF_Feature*) ((char*)featureList+loffset);
      char tag_buf[5]; copy_raw_tag(tag_buf, featureList->record[index].tag);
      unsigned int ind = getOTFFeature (feat, lookupList, 
         substtype, name, chars, liglen, out, tag_buf);
      if (ind)
      {
        return ind;
      }
    }
  }
  return 0;
}

/**
 * Copy a 4-byte OTF tag from raw font table bytes to a null-terminated buffer.
 */
static void copy_raw_tag(char dest[5], const void* raw_tag) {
    const unsigned char* src = (const unsigned char*)raw_tag;
    dest[0] = (char)src[0];
    dest[1] = (char)src[1];
    dest[2] = (char)src[2];
    dest[3] = (char)src[3];
    dest[4] = '\0';
    /* Filter out non-printable chars */
    for (int i = 0; i < 4; i++) {
        if (dest[i] < 0x20 || dest[i] > 0x7e) dest[i] = '?';
    }
}

/**
 * @param feat is the feature to go through
 * @param lookuplist is the list of lookups.
 * @param substtype is 4 for ligature substitution.
 * @param name is the fontname - used in error printouts.
 * @param chars is the glyphindeces  in a ligature
 * @param liglen is the glyphindeces count
 * @param outchar is the output glyph. For chaining substitution
 *   is should be at least length of liglen.
 * @return the number of glyphs that got substituted 
 */
static unsigned int
getOTFFeature (OTF_Feature* feat, OTF_LookupList* lookupList,
  unsigned int substtype, const SString& name, 
  const SS_GlyphIndex* chars, unsigned int liglen, SS_GlyphIndex* outchar,
  const char* feature_tag)
{
  SD_USHORT nfcount = ntohs (feat->count);
  for (unsigned int j=0; j<nfcount; j++)
  {
    SD_USHORT rec = ntohs (feat->record[j]);
    SD_USHORT lrec = ntohs (lookupList->record[rec]);
    OTF_Lookup * ltable = (OTF_Lookup*)((char*)lookupList + lrec); 
    SD_USHORT type = lookupEffectiveType (ltable);
#if PRINT_UNSUPPORTED
    if (type !=1 && type != 4 && type != 6 && type != 5)
    {
      fprintf (stderr, "UNSUPPORTED %*.*s GSUB type:%u.\n", 
          SSARGS(name), type);
    }
#endif
    if (type != substtype) continue;

    /* Trace: emit lookup start */
    yudit_trace_emit(YUDIT_TRACE_LOOKUP_START, "GSUB", rec, feature_tag, 0);

    unsigned int ret = 0;
    switch (type)
    {
    case 1: 
      ret = doSingleSubstitution (name, ltable, chars, liglen, outchar);
      break;
    case 4: 
      ret = doLigatureSubstitution (name, ltable, chars, liglen, outchar);
      break;
    case 5: 
      ret = doContextSubstitution (name, 
             lookupList, ltable, chars, liglen, outchar);
      break;
    case 6: 
      ret = doChainContextSubstitution (name, 
             lookupList, ltable, chars, liglen, outchar);
    }

    /* Trace: emit lookup end (matched if ret > 0) */
    if (ret == 0)
    {
      yudit_trace_emit(YUDIT_TRACE_LOOKUP_SKIPPED, "GSUB", rec, feature_tag, 0);
    }
    yudit_trace_emit(YUDIT_TRACE_LOOKUP_END, "GSUB", rec, feature_tag, ret > 0);

    if (ret) return ret;
 }
 return 0;
}

/**
 * Perform Substitution type 6, Chaining Contextual Substitution.
 * @param name is the name of this font.
 * @param lookuplist is the whole lookuplist
 * @param ltable is this lookup table.
 * @param chars is the input characters.
 * @param liglen is the length of the input characters.
 * @param outchar is the output array. The length must be liglen.
 */
static unsigned int
doContextSubstitution (const SString& name, 
  OTF_LookupList* lookupList, OTF_Lookup* ltable,
  const SS_GlyphIndex* chars, unsigned int liglen, 
  SS_GlyphIndex* outchar)
{
  SD_USHORT ltcount = ntohs (ltable->count);
  for (unsigned int k=0; k<ltcount; k++)
  {
    SD_ULONG offset = lookupSubtableOffset (ltable, k);
    SD_USHORT *pformat = (SD_USHORT*) ((char*)ltable +  offset);
    SD_USHORT cformat = ntohs (*pformat);
    // Class Based Context Subst.
    if (cformat == 2)
    {
      OTF_ContextSubstFormat2 *f2 =  
        (OTF_ContextSubstFormat2*) ( (char*)ltable +  offset);
      SD_USHORT coffset = ntohs (f2->coverage);
      OTF_CoverageFormat* cf = (OTF_CoverageFormat*) 
              ((char*)f2 +  coffset);
      SD_USHORT cdoffset = ntohs (f2->classDef);
      SD_USHORT ccount = ntohs (f2->subClassSetCnt);

      for (unsigned int i=0; i<ccount; i++)
      {
        SD_USHORT coffset = ntohs (f2->subClassSet[i]);
        if (coffset == 0) continue;

        OTF_SubClassSet * sset = 
             (OTF_SubClassSet*) ((char*) f2 + coffset);
        SD_USHORT rcount = ntohs (sset->subClassRuleCnt);
        for (unsigned int j=0; j<rcount; j++)
        {
          SD_USHORT roffset = ntohs (sset->subClassRule[j]);
          OTF_SubClassRule *srule =  
            (OTF_SubClassRule*) ((char*) sset + roffset);
          SD_USHORT gcount = ntohs(srule->glyphCount);

          if (gcount == 0)
          {
             continue;
          }
          SD_USHORT scount = ntohs(srule->substCount);
          SubstLookupRecord * slrec = (SubstLookupRecord*) 
            &srule->clazz [gcount-1];
          if ((unsigned int)gcount>(unsigned int) liglen)
          {
            continue;
          }
          //----------------------------------------------------------------
          // Context - 2
          //----------------------------------------------------------------
          unsigned int index = 0;
          SD_USHORT coverageIndex = getCoverageIndex (cf, chars[index++]);
          if (coverageIndex == 0xffff)
          {
             continue;
          }
          bool ok = true;
          /* cont */
          for (unsigned int ii=1; ii<gcount; ii++)
          {
            SD_USHORT c1 = ntohs (srule->clazz[ii-1]);
            SD_USHORT c2 = glyphClass((char*)f2 + cdoffset, 
                  chars[index++]);
            if (c2==0xffff || c2 != c1)
            {
              ok = false; 
              break;
            }
          }
          if (!ok) continue;
          unsigned int inlength = gcount;
          unsigned int m;
          for (m=0; m<liglen; m++)
          {
             outchar[m] = chars[m];
          }
          // Perform prescribed substitutions
          for (m=0; m<(unsigned int)scount; m++)
          {
            SubstLookupRecord* sr = &slrec[m]; 
            SD_USHORT sqi = ntohs (sr->sequenceIndex);
            SD_USHORT loi = ntohs (sr->lookupListIndex);

            SD_USHORT mlrec = ntohs (lookupList->record[loi]);
            OTF_Lookup * mltable = (OTF_Lookup*)((char*)lookupList + mlrec); 
            SD_USHORT mtype = ntohs (mltable->type);
            unsigned int slen=0;
            SS_GlyphIndex g;
            unsigned int at = (unsigned int) sqi;
            // Sanity
            if (at > inlength) break;
            unsigned int lsize = inlength - at;
            switch (mtype)
            {
            case 1: 
              slen = doSingleSubstitution (name, mltable, 
                      &outchar[at], lsize, &g);
              break;
            case 3: 
              slen = doAlternateSubstitution (name, mltable, 
                      &outchar[at], lsize, &g);
              break;
            case 4: 
              slen = doLigatureSubstitution (name, mltable, 
                      &outchar[at], lsize, &g);
              break;
            }
            if (slen == 0) continue;
            if (slen > lsize) continue;
            outchar[at] = g;
            /* move down */
            for (unsigned int v=at+1; v+slen-1 < liglen; v++)
            {
              outchar[v] = outchar[v+slen-1];
            }
            inlength = inlength - slen +1;
          }
          return inlength + liglen - gcount;
        }
      }
      return 0;
    }
    else  // todo other stuff if there is such a font.
    {
#if PRINT_UNSUPPORTED
      fprintf (stderr, "UNSUPPORTED %*.*s GSUB type:%u format=%u.\n", 
          SSARGS(name), 5, cformat);
#endif
    }
  }
  return 0;
}
/**
 * Perform Substitution type 6, Chaining Contextual Substitution.
 * @param name is the name of this font.
 * @param lookuplist is the whole lookuplist
 * @param ltable is this lookup table.
 * @param chars is the input characters.
 * @param liglen is the length of the input characters.
 * @param outchar is the output array. The length must be liglen.
 */
static unsigned int
doChainContextSubstitution (const SString& name, 
  OTF_LookupList* lookupList, OTF_Lookup* ltable,
  const SS_GlyphIndex* chars, unsigned int liglen, 
  SS_GlyphIndex* outchar)
{
  SD_USHORT ltcount = ntohs (ltable->count);
  for (unsigned int k=0; k<ltcount; k++)
  {
    SD_ULONG offset = lookupSubtableOffset (ltable, k);
    SD_USHORT *pformat = (SD_USHORT*) ((char*)ltable +  offset);
    SD_USHORT cformat = ntohs (*pformat);

    // Coverage Based Chaining Context Glyph Substitution 
    if (cformat == 3) 
    {
      OTF_ChainContextSubstFormat3_Backtrack *bformat =  
        (OTF_ChainContextSubstFormat3_Backtrack*) (
         (char*)ltable +  offset);
      SD_USHORT bcount = ntohs(bformat->backtrackGlyphCount);

      OTF_ChainContextSubstFormat3_Input * iformat = 
        (OTF_ChainContextSubstFormat3_Input*) &bformat->coverage[bcount];
      SD_USHORT icount = ntohs(iformat->inputGlyphCount);

      OTF_ChainContextSubstFormat3_Lookahead * lformat = 
        (OTF_ChainContextSubstFormat3_Lookahead*) &iformat->coverage[icount];
      SD_USHORT lcount = ntohs(lformat->lookaheadGlyphCount);

      OTF_ChainContextSubstFormat3_Subst * sformat = 
        (OTF_ChainContextSubstFormat3_Subst*) &lformat->coverage[lcount];
      SD_USHORT scount = ntohs(sformat->substGlyphCount);

      if ((unsigned int)bcount+icount+lcount>(unsigned int) liglen)
      {
        continue;
      }
      bool ok = true;
      unsigned int index = 0;
      /* Backtrack */
      for (unsigned int bi=0; bi<bcount; bi++)
      {
        SD_USHORT coffset = ntohs (bformat->coverage[bcount-bi-1]);
        OTF_CoverageFormat* cf = (OTF_CoverageFormat*) 
              ((char*)bformat +  coffset);
        SD_USHORT coverageIndex = getCoverageIndex (cf, chars[index++]);
        if (coverageIndex==0xffff)
        {
          ok = false; 
          break;
        }
      }
      if (!ok) continue;
      /* Input */
      for (unsigned int ii=0; ii<icount; ii++)
      {
        SD_USHORT coffset = ntohs (iformat->coverage[ii]);
        /* this is also measured from bformat */
        OTF_CoverageFormat* cf = (OTF_CoverageFormat*) 
              ((char*)bformat +  coffset);
        SD_USHORT coverageIndex = getCoverageIndex (cf, chars[index++]);
        if (coverageIndex==0xffff)
        {
          ok = false; 
          break;
        }
      }
      if (!ok) continue;
      /* Lookahead */
      for (unsigned int li=0; li<lcount; li++)
      {
        SD_USHORT coffset = ntohs (lformat->coverage[li]);
        /* this is also measured from bformat */
        OTF_CoverageFormat* cf = (OTF_CoverageFormat*) 
              ((char*)bformat +  coffset);
        SD_USHORT coverageIndex = getCoverageIndex (cf, chars[index++]);
        if (coverageIndex==0xffff)
        {
          ok = false; 
          break;
        }
      }
      if (!ok) continue;
      // OTF_ChainContextSubstFormat3_Subst sformat
      unsigned int m;
      unsigned int om=0;
      unsigned int im=0;
      for (m=0; m<(unsigned int)bcount; m++)
      {
        outchar[om++] = chars[im++];
      }
      for (m=0; m<(unsigned int)icount; m++)
      {
        outchar[om+m] = chars[im++];
      }
      /* Substitute from outchar[om..scount] - assume scount < icount */
      unsigned int inlength = icount;
      for (m=0; m<(unsigned int)scount; m++)
      {
        SubstLookupRecord* sr = (SubstLookupRecord*)
          &sformat->substLookupRecord[m];
        SD_USHORT sqi = ntohs (sr->sequenceIndex);
        SD_USHORT loi = ntohs (sr->lookupListIndex);
        /* 
         * Perform substitution loi at outchar[om+sqi].
         * If is is just a single glyph substitution
         * replace outchar[om+sqi].
         * If n glyphs are made into 1 then
         * reduce inlen with n-1, and move data.
         */
        SD_USHORT mlrec = ntohs (lookupList->record[loi]);
        OTF_Lookup * mltable = (OTF_Lookup*)((char*)lookupList + mlrec); 
        SD_USHORT mtype = ntohs (mltable->type);
        unsigned int slen=0;
        SS_GlyphIndex g;
        unsigned int at = (unsigned int) sqi;
        // Sanity.
        if (at > inlength) break;
        unsigned int lsize = inlength - at;
        switch (mtype)
        {
        case 1: 
          slen = doSingleSubstitution (name, mltable, 
                  &outchar[om+at], lsize, &g);
          break;
        case 3: 
          slen = doAlternateSubstitution (name, mltable, 
                  &outchar[om+at], lsize, &g);
          break;
        case 4: 
          slen = doLigatureSubstitution (name, mltable, 
                  &outchar[om+at], lsize, &g);
        }
        if (slen == 0) continue;
        if (slen > lsize) continue;
        outchar[om+at] = g;
        /* move down */
        for (unsigned int v=at+1; v+slen-1 < inlength; v++)
        {
          outchar[om+v] = outchar[om+v+slen-1];
        }
        inlength = inlength - slen +1;
      }
      om += inlength;
      /* do it till the end of ligature. */
      for (m=(unsigned int)bcount+icount; m<(unsigned int)liglen; m++)
      {
        outchar[om++] = chars[im++];
      }
      return om;
    }
    // Class Based Chaining Context Glyph Substitution 
    else  if (cformat == 2)
    {
      OTF_ChainContextSubstFormat2 *f2 =  
        (OTF_ChainContextSubstFormat2*) ( (char*)ltable +  offset);
      SD_USHORT coffset = ntohs (f2->coverage);
      OTF_CoverageFormat* cf = (OTF_CoverageFormat*) 
              ((char*)f2 +  coffset);
      SD_USHORT bcoffset = ntohs (f2->backtrackClassDef);
      SD_USHORT icoffset = ntohs (f2->inputClassDef);
      SD_USHORT lcoffset = ntohs (f2->lookaheadClassDef);
      SD_USHORT ccount = ntohs (f2->chainSubClassSetCnt);

      for (unsigned int i=0; i<ccount; i++)
      {
        SD_USHORT coffset = ntohs (f2->chainSubClassSet[i]);
        if (coffset == 0) continue;

        OTF_ChainSubClassSet * sset = 
             (OTF_ChainSubClassSet*) ((char*) f2 + coffset);

        SD_USHORT rcount = ntohs (sset->chainSubClassRuleCnt);
        for (unsigned int j=0; j<rcount; j++)
        {
          SD_USHORT roffset = ntohs (sset->chainSubClassRule[j]);

          OTF_ChainContextSubstFormat2_Backtrack *bformat =  
            (OTF_ChainContextSubstFormat2_Backtrack*) ((char*) sset + roffset);
          SD_USHORT bcount = ntohs(bformat->backtrackGlyphCount);
    
          OTF_ChainContextSubstFormat2_Input * iformat = 
            (OTF_ChainContextSubstFormat2_Input*) &bformat->coverage[bcount];
          SD_USHORT icount = ntohs(iformat->inputGlyphCount);

          if (icount == 0)
          {
             continue;
          }
    
          OTF_ChainContextSubstFormat2_Lookahead * lformat = 
            (OTF_ChainContextSubstFormat2_Lookahead*) &iformat->coverage[icount-1];
          SD_USHORT lcount = ntohs(lformat->lookaheadGlyphCount);
    
          OTF_ChainContextSubstFormat2_Subst * sformat = 
            (OTF_ChainContextSubstFormat2_Subst*) &lformat->coverage[lcount];
          SD_USHORT scount = ntohs(sformat->substGlyphCount);

          if ((unsigned int)bcount+icount+lcount>(unsigned int) liglen)
          {
            continue;
          }
          //----------------------------------------------------------------
          // Chain context - 2
          //----------------------------------------------------------------
          bool ok = true;
          unsigned int index = 0;

          /* Backtrack */
          for (unsigned int bi=0; bi<bcount; bi++)
          {
            SD_USHORT backtrack = ntohs (bformat->coverage[bcount-bi-1]);
            if (bcoffset == 0) 
            {
              ok = false; 
              break;
            }
            SD_USHORT clazz = glyphClass((char*)f2 + bcoffset, 
                  chars[index++]);
            if (clazz==0xffff || clazz != backtrack)
            {
              ok = false; 
              break;
            }
          }
          if (!ok) continue;
          SD_USHORT coverageIndex = getCoverageIndex (cf, chars[index++]);
          if (coverageIndex == 0xffff)
          {
             continue;
          }
          /* Input */
          for (unsigned int ii=1; ii<icount; ii++)
          {
            SD_USHORT input = ntohs (iformat->coverage[ii-1]);
            if (icoffset == 0) 
            {
              ok = false; 
              break;
            }
            SD_USHORT clazz = glyphClass((char*)f2 + icoffset, 
                chars[index++]);
            if (clazz==0xffff || clazz != input)
            {
              ok = false; 
              break;
            }
          }
          if (!ok) continue;
          /* Lookahead */
          for (unsigned int li=0; li<lcount; li++)
          {
            SD_USHORT lookahead = ntohs (lformat->coverage[li]);
            if (lcoffset == 0) 
            {
              ok = false; 
              break;
            }
            SD_USHORT clazz = glyphClass((char*)f2 + lcoffset,  
                   chars[index++]);
            if (clazz==0xffff || clazz != lookahead)
            {
              ok = false; 
              break;
            }
          }
          if (!ok) continue;

          // OTF_ChainContextSubstFormat3_Subst sformat
          unsigned int m;
          unsigned int om=0;
          unsigned int im=0;
          for (m=0; m<(unsigned int)bcount; m++)
          {
            outchar[om++] = chars[im++];
          }
          for (m=0; m<(unsigned int)icount; m++)
          {
            outchar[om+m] = chars[im++];
          }
          /* Substitute from outchar[om..scount] - assume scount < icount */
          unsigned int inlength = icount;
          for (m=0; m<(unsigned int)scount; m++)
          {
            SubstLookupRecord* sr = (SubstLookupRecord*)
              &sformat->substLookupRecord[m];
            SD_USHORT sqi = ntohs (sr->sequenceIndex);
            SD_USHORT loi = ntohs (sr->lookupListIndex);
            /* 
             * Perform substitution loi at outchar[om+sqi].
             * If is is just a single glyph substitution
             * replace outchar[om+sqi].
             * If n glyphs are made into 1 then
             * reduce inlen with n-1, and move data.
             */
            SD_USHORT mlrec = ntohs (lookupList->record[loi]);
            OTF_Lookup * mltable = (OTF_Lookup*)((char*)lookupList + mlrec); 
            SD_USHORT mtype = ntohs (mltable->type);
            unsigned int slen=0;
            SS_GlyphIndex g;
            unsigned int at = (unsigned int) sqi;
            // Sanity.
            if (at > inlength) break;
            unsigned int lsize = inlength - at;
            switch (mtype)
            {
            case 1: 
              slen = doSingleSubstitution (name, mltable, 
                      &outchar[om+at], lsize, &g);
              break;
            case 3: 
              slen = doAlternateSubstitution (name, mltable, 
                      &outchar[om+at], lsize, &g);
              break;
            case 4: 
              slen = doLigatureSubstitution (name, mltable, 
                      &outchar[om+at], lsize, &g);
            }
            if (slen == 0) continue;
            if (slen > lsize) continue;
            outchar[om+at] = g;
            /* move down */
            for (unsigned int v=at+1; v+slen-1 < inlength; v++)
            {
              outchar[om+v] = outchar[om+v+slen-1];
            }
            inlength = inlength - slen +1;
          }
          om += inlength;
          /* do it till the end of ligature. */
          for (m=(unsigned int)bcount+icount; m<(unsigned int)liglen; m++)
          {
            outchar[om++] = chars[im++];
          }
          return om;
        }
        //----------------------------------------------------------------
        // Chain context - 2
        //----------------------------------------------------------------
      }
    }
    else 
    {
#if PRINT_UNSUPPORTED
      fprintf (stderr, "UNSUPPORTED %*.*s GSUB type:%u format=%u.\n", 
          SSARGS(name), 6, cformat);
#endif
    }
  } 
  return 0;
}

/**
 * Perform Substitution type 1, Single Substitution.
 * @param name is the name of this font.
 * @param ltable is this lookup table.
 * @param chars is the input characters.
 * @param liglen is the length of the input characters.
 * @param outchar is the output array. The length is 1.
 */
static unsigned int
doSingleSubstitution (const SString& name, OTF_Lookup* ltable,
  const SS_GlyphIndex* chars, unsigned int liglen, SS_GlyphIndex* outchar)
{
  SD_USHORT ltcount = ntohs (ltable->count);
  for (unsigned int k=0; k<ltcount; k++)
  {
    SD_ULONG offset = lookupSubtableOffset (ltable, k);
    OTF_SingleSubstFormat *lformat =  
        (OTF_SingleSubstFormat*) ((char*)ltable +  offset);

    SD_USHORT coffset = ntohs (lformat->coverage);

    /* This should be less than ccount after coverage is found */
    OTF_CoverageFormat* coverageFormat = (OTF_CoverageFormat*)
       ((char*)lformat +  coffset);

    SD_USHORT coverageIndex = getCoverageIndex (coverageFormat, chars[0]);
    if (coverageIndex == 0xffff)
    {
       continue; /* no coverage */
    }
    SD_USHORT cformat = ntohs (lformat->format);
    if (cformat == 1)
    {
       OTF_SingleSubstFormat1* lformat1 =
         (OTF_SingleSubstFormat1*) ((char*)ltable +  offset);
       SD_SHORT deltagid = ntohs (lformat1->deltaGlyphID);
       return (chars[0] + deltagid);
    }
    else if (cformat == 2) 
    {
       OTF_SingleSubstFormat2* lformat2 =
         (OTF_SingleSubstFormat2*) ((char*)ltable +  offset);
       SD_USHORT count2 =  ntohs (lformat2->count);
       if (coverageIndex >= count2) continue; 
       SD_USHORT glyph = ntohs (lformat2->substitute[coverageIndex]);
       *outchar = (SS_GlyphIndex) glyph;
       return 1;
    }
    else
    {
#if PRINT_UNSUPPORTED
      fprintf (stderr, "UNSUPPORTED %*.*s GSUB type:%u format=%u.\n", 
          SSARGS(name), 1, cformat);
#endif
    }
  }
  return 0;
}

/**
 * Perform Substitution type 3, Altenate Substitution.
 * @param name is the name of this font.
 * @param ltable is this lookup table.
 * @param chars is the input characters.
 * @param liglen is the length of the input characters.
 * @param outchar is the output array. The length is 1.
 */
static unsigned int
doAlternateSubstitution (const SString& name, OTF_Lookup* ltable,
  const SS_GlyphIndex* chars, unsigned int liglen, SS_GlyphIndex* outchar)
{
  SD_USHORT ltcount = ntohs (ltable->count);
  for (unsigned int k=0; k<ltcount; k++)
  {
    SD_ULONG offset = lookupSubtableOffset (ltable, k);
    OTF_AlternateSubstFormat1 *lformat =  
        (OTF_AlternateSubstFormat1*) ((char*)ltable +  offset);
    SD_USHORT cformat = ntohs (lformat->format);
    if (cformat==1)
    {
      SD_USHORT acount = ntohs (lformat->alternateCount);
      SD_USHORT coffset = ntohs (lformat->coverage);
      /* This should be less than ccount after coverage is found */
      OTF_CoverageFormat* coverageFormat = (OTF_CoverageFormat*)
         ((char*)lformat +  coffset);
      SD_USHORT coverageIndex = getCoverageIndex (coverageFormat, chars[0]);
      if (coverageIndex >= acount)
      {
        continue; /* no coverage */
      }
      SD_USHORT aoffset = htons (lformat->alternateSet[coverageIndex]);
      OTF_AlternateSet* aset = (OTF_AlternateSet*) ((char*)lformat + aoffset);
      SD_USHORT gcount = htons (aset->glyphCount);
      if (gcount > 0)
      {
        SD_USHORT ch = htons (aset->alternate[0]);
//fprintf (stderr, "XXXX Alternate %04X -> %04X\n", chars[0], ch);
        *outchar = (SS_GlyphIndex) ch;
        return 1;
      }
    }
    else
    {
#if PRINT_UNSUPPORTED
      fprintf (stderr, "UNSUPPORTED %*.*s GSUB type:%u format=%u.\n", 
          SSARGS(name), 3, cformat);
#endif
    }
  }
  return 0;
}

/**
 * Perform Substitution type 4, Ligature Substitution.
 * @param name is the name of this font.
 * @param ltable is this lookup table.
 * @param chars is the input characters.
 * @param liglen is the length of the input characters.
 * @param outchar is the output array. The length is 1.
 */
static unsigned int
doLigatureSubstitution (const SString& name, OTF_Lookup* ltable,
  const SS_GlyphIndex* chars, unsigned int liglen, SS_GlyphIndex* outchar)
{
  SD_USHORT ltcount = ntohs (ltable->count);
  // HELP Is this good or bad?
  //  if ((flag & 0x0e)!=0)  continue;
  for (unsigned int k=0; k<ltcount; k++)
  {
    SD_ULONG offset = lookupSubtableOffset (ltable, k);
    OTF_LigatureSusbstFormat1 *lformat =  
        (OTF_LigatureSusbstFormat1*) ((char*)ltable +  offset);
    SD_USHORT cformat = ntohs (lformat->format);
    if (cformat != 1) 
    {
#if PRINT_UNSUPPORTED
      fprintf (stderr, "UNSUPPORTED %*.*s GSUB type:%u format=%u.\n", 
          SSARGS(name), 4, cformat);
#endif
      continue;
    }
    SD_USHORT coffset = ntohs (lformat->coverage);
    /* get the coverage */
    SD_USHORT ccount = ntohs (lformat->count);
    /* This should be less than ccount after coverage is found */
    OTF_CoverageFormat* coverageFormat = (OTF_CoverageFormat*)
       ((char*)lformat +  coffset);

    SD_USHORT coverageIndex = getCoverageIndex (coverageFormat, chars[0]);
    if (coverageIndex >= ccount)
    {
       continue; /* no coverage */
    }
    /* don't  loop - get coverageIndex */
    SD_USHORT cinoffset = ntohs (lformat->offset[coverageIndex]);
    OTF_LigatureSet *lset = (OTF_LigatureSet *)  
     ((SD_BYTE*) lformat + cinoffset);
    SD_USHORT count = ntohs (lset->count);
    for (unsigned int m=0; m<count; m++)
    {
      SD_USHORT offset = ntohs (lset->offset[m]);
      OTF_Ligature* lig = (OTF_Ligature*) 
        ((SD_BYTE*) lset + offset);
      SD_USHORT compcount = ntohs (lig->count);
      if (compcount > liglen)
      {
        continue;
      }
      bool found = true;
      for (unsigned int n=0; n+1<compcount; n++)
      {
         SD_USHORT component = ntohs (lig->component[n]);
         if (chars[n+1] != component) found = false;
      }
      if (found)
      {
         SD_USHORT glyph = ntohs (lig->glyph);
         *outchar = (SS_GlyphIndex) glyph;
         return (unsigned int) compcount;
      }
    }
  }
  return 0;
}

/**
 * Get a coverage index 
 * @param coverageFormat is the header format
 */
static SD_USHORT
getCoverageIndex  (OTF_CoverageFormat* coverageFormat, SS_GlyphIndex gi)
{
  SD_USHORT coverageIndex = 0xffff;
  /* go through coverage tables - they should be sorted */
  SD_USHORT format = ntohs (coverageFormat->format);
  if (format==1)
  {
    OTF_CoverageFormat1* format1 = (OTF_CoverageFormat1*) coverageFormat;
    SD_USHORT gcount = ntohs (format1->count);
    /* Do binary search here   set coverageIndex */
    unsigned int    top, bottom, mid;
    top = gcount; bottom = 0;
    unsigned int c0 = gi;
    SD_USHORT id = c0 +1;
    while (top > bottom)
    {
      mid = (top+bottom)/2;
      id = ntohs (format1->id[mid]);
      if (c0 == id) { top = mid; break; }
      if (c0 < id) { top = mid; continue; }
      bottom = mid + 1;
    }
    if (top < gcount && c0 == id) coverageIndex = top;
  }
  else if (format==2)
  {
    OTF_CoverageFormat2* format2 = (OTF_CoverageFormat2*) coverageFormat;
    /* I could not find any format2 fonts-this part is untested */
    SD_USHORT gcount = ntohs (format2->count);
    /* Do binary search here. */
    for (unsigned int l=0; l<gcount; l++)
    {
      SD_USHORT index = ntohs (format2->record[l].index);
      SD_USHORT start = ntohs (format2->record[l].start);
      SD_USHORT end = ntohs (format2->record[l].end);
      if (gi >= start && gi <= end)
      {
        coverageIndex = index + gi - start;
        break;
      }
    }
  }
  else
  {
#if PRINT_UNSUPPORTED
      fprintf (stderr, "UNSUPPORTED GSUB coverage format=%u.\n", format);
#endif
  }
  return coverageIndex;
}

// OTF_ClassDefFormat1,
// OTF_ClassDefFromat2
SD_USHORT
glyphClass (char* def, SD_USHORT glyph)
{
  OTF_ClassDefFormat1* f1 = (OTF_ClassDefFormat1*) def;
  SD_USHORT format = htons (f1->format);
  SD_USHORT rvle = 0xffff;
  if (format == 1)
  {
    SD_USHORT startGlyph = htons (f1->startGlyph);
    SD_USHORT glyphCount = htons (f1->glyphCount);
    if (glyph < startGlyph || glyph >= startGlyph + glyphCount) return rvle;
    rvle = htons (f1->classValueArray[glyph-startGlyph]);
    return rvle;
  }
  else if (format == 2)
  {
    OTF_ClassDefFormat2* f2 = (OTF_ClassDefFormat2*) def;
    SD_USHORT classRangeCount = htons (f2->classRangeCount);
    for (unsigned int i=0; i<classRangeCount; i++)
    {
      OTF_ClassRangeRecord *rec = (OTF_ClassRangeRecord*) 
             &f2->classRangeRecord[i];
      SD_USHORT startGlyph = htons(rec->startGlyph);
      SD_USHORT endGlyph = htons(rec->endGlyph);
      if (glyph >= startGlyph && glyph <= endGlyph)
      {
        rvle = htons (rec->classValue);
        return rvle;
      }
    }
    return rvle;
  }
  else
  {
#if PRINT_UNSUPPORTED
    fprintf (stderr, "UNSUPPORTED glyph-class format=%u.\n", format);
#endif
  }
  return rvle;
}

/**
 * Iterate to the next language system.
 * @param gsubh is the GSUB_HEAD header
 * @param _script is the OTF standard name of script.
 *  like "taml"
 * @patam from is the input output iterator counter 
 *       - first it is needed to be set to zero.
 */
static OTF_LangSys*
getNextOTFLanguageSystem (const SString fontname, GSUB_HEAD* gsubh, 
  const SString& script, unsigned int* from)
{
  /* Look for script. */
  int offset = ntohs(gsubh->scriptList);
  OTF_ScriptList*  scriptList = (OTF_ScriptList*) ((SD_BYTE*)gsubh + offset);
  SD_USHORT numscripts = ntohs (scriptList->scriptCount);
  
  for (unsigned int i=*from; i<numscripts; i++)
  {
    SString tag (scriptList->records[i].tag, 4);
    SD_USHORT sl = ntohs (scriptList->records[i].offset);
    /* we found the script */
    if (tag==script)
    {
      OTF_Script* stable = (OTF_Script*) ((char*)scriptList + sl);
      SD_USHORT defaultLangsys = ntohs (stable->defaultLangSys);
      SD_USHORT langSysCount = ntohs (stable->langSysCount);
      OTF_LangSys* lsys = 0;

      /* Use the default language system, falling back to the first record
       * when a font does not define one.  The deprecated `lookupOrder` field
       * is deliberately ignored: real fonts - Segoe UI among them - leave it
       * non-zero, and HarfBuzz ignores it as well.  The previous code also
       * mis-cast the default LangSys offset to a LangSysRecord and ended up
       * returning a bogus table. */
      if (defaultLangsys != 0)
      {
        lsys = (OTF_LangSys*) ((char*) stable + defaultLangsys);
      }
      else if (langSysCount > 0)
      {
        OTF_LangSysRecord* lr = &stable->langSysRecord[0];
        lsys = (OTF_LangSys*) ((char*) stable + ntohs (lr->offsetFromScript));
      }
      *from = i+1;
      return lsys;
    }
  }
  *from = numscripts;
  return 0;
}

#define LC_TMPPOS  0xf000 // temp. block for leading consonants
#define VO_TMPPOS  0xf100 // temp. block for vowels
#define TC_TMPPOS  0xf200 // temp. block for trailinng consonants
#define LC_OFFSET  (LC_TMPPOS-0x1100)
#define VO_OFFSET  (VO_TMPPOS-0x1160)
#define TC_OFFSET  (TC_TMPPOS-0x11a8)


/**
 * The map from sequences of leading consonants forming consonant clusters
 * not encoded in U+1100 block to  temporary code points in the 0xf000 block
 */

static _OTF_mslvtJamo LC_Clusters[]=
{
  {{0xf005, 0xf000, 0xf000}, 0xf06a}, // U+1105 U+1100 U+1100 => lc # 0x6a
  {{0xf005, 0xf003, 0xf003}, 0xf06c}, // U+1105 U+1103 U+1103 => lc # 0x6c
  {{0xf005, 0xf007, 0xf007}, 0xf06f}, // U+1105 U+1107 U+1107 => lc # 0x6f
  {{0xf005, 0xf007, 0xf00b}, 0xf070}, // U+1105 U+1107 U+110b => lc # 0x70
  {{0xf007, 0xf009, 0xf010}, 0xf077}, // U+1107 U+1109 U+1110 => lc # 0x77
  {{0xf009, 0xf009, 0xf007}, 0xf07a}, // U+1109 U+1109 U+1107 => lc # 0x7a
  {{0xf00c, 0xf00c, 0xf012}, 0xf07d}, // U+110c U+110c U+1112 => lc # 0x7d
  {{0xf005, 0xf001, 0x0000}, 0xf06a}, // U+1105 U+1101        => lc # 0x6a
  {{0xf005, 0xf004, 0x0000}, 0xf06c}, // U+1105 U+1104        => lc # 0x6c
  {{0xf005, 0xf008, 0x0000}, 0xf06f}, // U+1105 U+1108        => lc # 0x6f
  {{0xf005, 0xf02b, 0x0000}, 0xf070}, // U+1105 U+112b        => lc # 0x70
  {{0xf00a, 0xf007, 0x0000}, 0xf07a}, // U+110a U+1107        => lc # 0x7a
  {{0xf00d, 0xf012, 0x0000}, 0xf07d}, // U+110d U+1112        => lc # 0x7d
  {{0xf000, 0xf003, 0x0000}, 0xf060}, // U+1100 U+1103        => lc # 0x60
  {{0xf002, 0xf009, 0x0000}, 0xf061}, // U+1102 U+1109        => lc # 0x61
  {{0xf002, 0xf00c, 0x0000}, 0xf062}, // U+1102 U+110c        => lc # 0x62
  {{0xf002, 0xf012, 0x0000}, 0xf063}, // U+1102 U+1112        => lc # 0x63
  {{0xf003, 0xf005, 0x0000}, 0xf064}, // U+1103 U+1105        => lc # 0x64
  {{0xf003, 0xf006, 0x0000}, 0xf065}, // U+1103 U+1106        => lc # 0x65
  {{0xf003, 0xf007, 0x0000}, 0xf066}, // U+1103 U+1107        => lc # 0x66
  {{0xf003, 0xf009, 0x0000}, 0xf067}, // U+1103 U+1109        => lc # 0x67
  {{0xf003, 0xf00c, 0x0000}, 0xf068}, // U+1103 U+110c        => lc # 0x68
  {{0xf005, 0xf000, 0x0000}, 0xf069}, // U+1105 U+1100        => lc # 0x69
  {{0xf005, 0xf003, 0x0000}, 0xf06b}, // U+1105 U+1103        => lc # 0x6b
  {{0xf005, 0xf006, 0x0000}, 0xf06d}, // U+1105 U+1106        => lc # 0x6d
  {{0xf005, 0xf007, 0x0000}, 0xf06e}, // U+1105 U+1107        => lc # 0x6e
  {{0xf005, 0xf009, 0x0000}, 0xf071}, // U+1105 U+1109        => lc # 0x71
  {{0xf005, 0xf00c, 0x0000}, 0xf072}, // U+1105 U+110c        => lc # 0x72
  {{0xf005, 0xf00f, 0x0000}, 0xf073}, // U+1105 U+110f        => lc # 0x73
  {{0xf006, 0xf000, 0x0000}, 0xf074}, // U+1106 U+1100        => lc # 0x74
  {{0xf006, 0xf003, 0x0000}, 0xf075}, // U+1106 U+1103        => lc # 0x75
  {{0xf006, 0xf009, 0x0000}, 0xf076}, // U+1106 U+1109        => lc # 0x76
  {{0xf007, 0xf00f, 0x0000}, 0xf078}, // U+1107 U+110f        => lc # 0x78
  {{0xf007, 0xf012, 0x0000}, 0xf079}, // U+1107 U+1112        => lc # 0x79
  {{0xf00b, 0xf005, 0x0000}, 0xf07b}, // U+110b U+1105        => lc # 0x7b
  {{0xf00b, 0xf012, 0x0000}, 0xf07c}, // U+110b U+1112        => lc # 0x7c
  {{0xf010, 0xf010, 0x0000}, 0xf07e}, // U+1110 U+1110        => lc # 0x7e
  {{0xf011, 0xf012, 0x0000}, 0xf07f}, // U+1111 U+1112        => lc # 0x7f
  {{0xf012, 0xf009, 0x0000}, 0xf080}, // U+1112 U+1109        => lc # 0x80
  {{0xf059, 0xf059, 0x0000}, 0xf081}, // U+1159 U+1159        => lc # 0x81
  {{0,      0,      0},  0}
};

/**  
 *   The map from sequences of medial vowels forming vowel clusters
 *   not encoded in U+1100 block to  temporary code points in the 0xf100 block
 */

static _OTF_mslvtJamo VO_Clusters[]=
{
  {{0xf109, 0xf103, 0xf115}, 0xf147}, // U+1169 U+1163 U+1175 => vowel # 0x47
  {{0xf109, 0xf10e, 0xf13e}, 0xf149}, // U+1169 U+116e U+119e => vowel # 0x49
  {{0xf10d, 0xf101, 0xf115}, 0xf14b}, // U+116d U+1161 U+1175 => vowel # 0x4b
  {{0xf10e, 0xf115, 0xf115}, 0xf14e}, // U+116e U+1175 U+1175 => vowel # 0x4e
  {{0xf112, 0xf101, 0xf115}, 0xf14f}, // U+1172 U+1161 U+1175 => vowel # 0x4f
  {{0xf113, 0xf105, 0xf115}, 0xf153}, // U+1173 U+1165 U+1175 => vowel # 0x53
  {{0xf115, 0xf103, 0xf109}, 0xf155}, // U+1175 U+1163 U+1169 => vowel # 0x55
  {{0xf115, 0xf103, 0xf115}, 0xf156}, // U+1175 U+1163 U+1175 => vowel # 0x56
  {{0xf115, 0xf107, 0xf115}, 0xf158}, // U+1175 U+1167 U+1175 => vowel # 0x58
  {{0xf115, 0xf109, 0xf13e}, 0xf159}, // U+1175 U+1169 U+119e => vowel # 0x59
  {{0xf115, 0xf115, 0xf115}, 0xf15c}, // U+1175 U+1175 U+1175 => vowel # 0x5c
  {{0xf13e, 0xf105, 0xf115}, 0xf15e}, // U+119e U+1165 U+1175 => vowel # 0x5e
  {{0xf101, 0xf113, 0x0000}, 0xf143}, // U+1161 U+1173        => vowel # 0x43
  {{0xf103, 0xf10e, 0x0000}, 0xf144}, // U+1163 U+116e        => vowel # 0x44
  {{0xf107, 0xf103, 0x0000}, 0xf145}, // U+1167 U+1163        => vowel # 0x45
  {{0xf109, 0xf103, 0x0000}, 0xf146}, // U+1169 U+1163        => vowel # 0x46
  {{0xf109, 0xf104, 0x0000}, 0xf147}, // U+1169 U+1164        => vowel # 0x47
  {{0xf109, 0xf107, 0x0000}, 0xf148}, // U+1169 U+1167        => vowel # 0x48
  {{0xf10d, 0xf101, 0x0000}, 0xf14a}, // U+116d U+1161        => vowel # 0x4a
  {{0xf10d, 0xf102, 0x0000}, 0xf14b}, // U+116d U+1162        => vowel # 0x4b
  {{0xf10d, 0xf105, 0x0000}, 0xf14c}, // U+116d U+1165        => vowel # 0x4c
  {{0xf10e, 0xf107, 0x0000}, 0xf14d}, // U+116e U+1167        => vowel # 0x4d
  {{0xf112, 0xf102, 0x0000}, 0xf14f}, // U+1172 U+1162        => vowel # 0x4f
  {{0xf112, 0xf109, 0x0000}, 0xf150}, // U+1172 U+1169        => vowel # 0x50
  {{0xf113, 0xf101, 0x0000}, 0xf151}, // U+1173 U+1161        => vowel # 0x51
  {{0xf113, 0xf105, 0x0000}, 0xf152}, // U+1173 U+1165        => vowel # 0x52
  {{0xf113, 0xf106, 0x0000}, 0xf153}, // U+1173 U+1166        => vowel # 0x53
  {{0xf113, 0xf109, 0x0000}, 0xf154}, // U+1173 U+1169        => vowel # 0x54
  {{0xf115, 0xf104, 0x0000}, 0xf156}, // U+1175 U+1164        => vowel # 0x56
  {{0xf115, 0xf107, 0x0000}, 0xf157}, // U+1175 U+1167        => vowel # 0x57
  {{0xf115, 0xf10d, 0x0000}, 0xf15a}, // U+1175 U+116d        => vowel # 0x5a
  {{0xf115, 0xf112, 0x0000}, 0xf15b}, // U+1175 U+1172        => vowel # 0x5b
  {{0xf13e, 0xf101, 0x0000}, 0xf15d}, // U+119e U+1161        => vowel # 0x5d
  {{0xf13e, 0xf106, 0x0000}, 0xf15e}, // U+119e U+1166        => vowel # 0x5e
  {{0,      0,      0},  0}
};

/** 
 * The map from sequences of trailing consonants  forming consonant clusters
 * not encoded in U+1100 block to  temporary code points in the 0xf200 block
 */

static _OTF_mslvtJamo TC_Clusters[]=
{
  {{0xf206, 0xf206, 0xf210}, 0xf25b}, // U+11ae U+11ae U+11b8 => tc # 0x5b
  {{0xf206, 0xf212, 0xf200}, 0xf25e}, // U+11ae U+11ba U+11a8 => tc # 0x5e
  {{0xf207, 0xf200, 0xf200}, 0xf262}, // U+11af U+11a8 U+11a8 => tc # 0x62
  {{0xf207, 0xf200, 0xf21a}, 0xf263}, // U+11af U+11a8 U+11c2 => tc # 0x63
  {{0xf207, 0xf207, 0xf217}, 0xf264}, // U+11af U+11af U+11bf => tc # 0x64
  {{0xf207, 0xf20f, 0xf21a}, 0xf265}, // U+11af U+11b7 U+11c2 => tc # 0x65
  {{0xf207, 0xf210, 0xf206}, 0xf266}, // U+11af U+11b8 U+11ae => tc # 0x66
  {{0xf207, 0xf210, 0xf219}, 0xf267}, // U+11af U+11b8 U+11c1 => tc # 0x67
  {{0xf207, 0xf251, 0xf21a}, 0xf269}, // U+11af U+11f9 U+11c2 => tc # 0x69
  {{0xf20f, 0xf203, 0xf203}, 0xf26c}, // U+11b7 U+11ab U+11ab => tc # 0x6c
  {{0xf20f, 0xf210, 0xf212}, 0xf26e}, // U+11b7 U+11b8 U+11ba => tc # 0x6e
  {{0xf210, 0xf207, 0xf219}, 0xf271}, // U+11b8 U+11af U+11c1 => tc # 0x71
  {{0xf210, 0xf212, 0xf206}, 0xf274}, // U+11b8 U+11ba U+11ae => tc # 0x74
  {{0xf212, 0xf210, 0xf214}, 0xf278}, // U+11ba U+11b8 U+11bc => tc # 0x78
  {{0xf212, 0xf212, 0xf200}, 0xf279}, // U+11ba U+11ba U+11a8 => tc # 0x79
  {{0xf212, 0xf212, 0xf206}, 0xf27a}, // U+11ba U+11ba U+11ae => tc # 0x7a
  {{0xf243, 0xf210, 0xf214}, 0xf281}, // U+11eb U+11b8 U+11bc => tc # 0x81
  {{0xf215, 0xf210, 0xf210}, 0xf289}, // U+11bd U+11b8 U+11b8 => tc # 0x89
  {{0xf215, 0xf215, 0xf215}, 0xf28a}, // U+11bd U+11bd U+11bd => tc # 0x8a
  {{0xf200, 0xf203, 0x0000}, 0xf252}, // U+11a8 U+11ab        => tc # 0x52
  {{0xf200, 0xf210, 0x0000}, 0xf253}, // U+11a8 U+11b8        => tc # 0x53
  {{0xf200, 0xf216, 0x0000}, 0xf254}, // U+11a8 U+11be        => tc # 0x54
  {{0xf200, 0xf217, 0x0000}, 0xf255}, // U+11a8 U+11bf        => tc # 0x55
  {{0xf200, 0xf21a, 0x0000}, 0xf256}, // U+11a8 U+11c2        => tc # 0x56
  {{0xf203, 0xf203, 0x0000}, 0xf257}, // U+11ab U+11ab        => tc # 0x57
  {{0xf203, 0xf207, 0x0000}, 0xf258}, // U+11ab U+11af        => tc # 0x58
  {{0xf203, 0xf216, 0x0000}, 0xf259}, // U+11ab U+11be        => tc # 0x59
  {{0xf206, 0xf206, 0x0000}, 0xf25a}, // U+11ae U+11ae        => tc # 0x5a
  {{0xf206, 0xf210, 0x0000}, 0xf25c}, // U+11ae U+11b8        => tc # 0x5c
  {{0xf206, 0xf212, 0x0000}, 0xf25d}, // U+11ae U+11ba        => tc # 0x5d
  {{0xf206, 0xf215, 0x0000}, 0xf25f}, // U+11ae U+11bd        => tc # 0x5f
  {{0xf206, 0xf216, 0x0000}, 0xf260}, // U+11ae U+11be        => tc # 0x60
  {{0xf206, 0xf218, 0x0000}, 0xf261}, // U+11ae U+11c0        => tc # 0x61
  {{0xf207, 0xf201, 0x0000}, 0xf262}, // U+11af U+11a9        => tc # 0x62
  {{0xf207, 0xf248, 0x0000}, 0xf268}, // U+11af U+11f0        => tc # 0x68
  {{0xf207, 0xf214, 0x0000}, 0xf26a}, // U+11af U+11bc        => tc # 0x6a
  {{0xf20f, 0xf203, 0x0000}, 0xf26b}, // U+11b7 U+11ab        => tc # 0x6b
  {{0xf20f, 0xf20f, 0x0000}, 0xf26d}, // U+11b7 U+11b7        => tc # 0x6d
  {{0xf20f, 0xf215, 0x0000}, 0xf26f}, // U+11b7 U+11bd        => tc # 0x6f
  {{0xf210, 0xf206, 0x0000}, 0xf270}, // U+11b8 U+11ae        => tc # 0x70
  {{0xf210, 0xf20f, 0x0000}, 0xf272}, // U+11b8 U+11b7        => tc # 0x72
  {{0xf210, 0xf210, 0x0000}, 0xf273}, // U+11b8 U+11b8        => tc # 0x73
  {{0xf210, 0xf215, 0x0000}, 0xf275}, // U+11b8 U+11bd        => tc # 0x75
  {{0xf210, 0xf216, 0x0000}, 0xf276}, // U+11b8 U+11be        => tc # 0x76
  {{0xf212, 0xf20f, 0x0000}, 0xf277}, // U+11ba U+11b7        => tc # 0x77
  {{0xf212, 0xf23e, 0x0000}, 0xf278}, // U+11ba U+11e6        => tc # 0x78
  {{0xf213, 0xf200, 0x0000}, 0xf279}, // U+11bb U+11a8        => tc # 0x79
  {{0xf213, 0xf206, 0x0000}, 0xf27a}, // U+11bb U+11ae        => tc # 0x7a
  {{0xf212, 0xf243, 0x0000}, 0xf27b}, // U+11ba U+11eb        => tc # 0x7b
  {{0xf212, 0xf215, 0x0000}, 0xf27c}, // U+11ba U+11bd        => tc # 0x7c
  {{0xf212, 0xf216, 0x0000}, 0xf27d}, // U+11ba U+11be        => tc # 0x7d
  {{0xf212, 0xf218, 0x0000}, 0xf27e}, // U+11ba U+11c0        => tc # 0x7e
  {{0xf212, 0xf21a, 0x0000}, 0xf27f}, // U+11ba U+11c2        => tc # 0x7f
  {{0xf243, 0xf210, 0x0000}, 0xf280}, // U+11eb U+11b8        => tc # 0x80
  {{0xf243, 0xf23e, 0x0000}, 0xf281}, // U+11eb U+11e6        => tc # 0x81
  {{0xf214, 0xf20f, 0x0000}, 0xf282}, // U+11bc U+11b7        => tc # 0x82
  {{0xf214, 0xf212, 0x0000}, 0xf283}, // U+11bc U+11ba        => tc # 0x83
  {{0xf214, 0xf21a, 0x0000}, 0xf284}, // U+11bc U+11c2        => tc # 0x84
  {{0xf248, 0xf200, 0x0000}, 0xf285}, // U+11f0 U+11a8        => tc # 0x85
  {{0xf248, 0xf217, 0x0000}, 0xf286}, // U+11f0 U+11bf        => tc # 0x86
  {{0xf248, 0xf21a, 0x0000}, 0xf287}, // U+11f0 U+11c2        => tc # 0x87
  {{0xf215, 0xf210, 0x0000}, 0xf288}, // U+11bd U+11b8        => tc # 0x88
  {{0xf219, 0xf212, 0x0000}, 0xf28b}, // U+11c1 U+11ba        => tc # 0x8b
  {{0xf219, 0xf218, 0x0000}, 0xf28c}, // U+11c1 U+11c0        => tc # 0x8c
  { {0,      0,      0},  0}
};

/** 
 * transforms the content of in[] to a  more convenient form
 * for mapping to glyphs of mslvt fonts and store the result in med[].
 * More specifically, sequences of basic Jamos for which 
 * precomposed Jamo glyphs are available in mslvt TTF's are replaced 
 * with Jamo cluster code points defined in PUA. In findJamoGlyph(),
 * these PUA code points are converted to glyph code points
 * in mslvt TTF's which are in turn converted to glyph indices.
 * Also, put a new length in *len after the replacement.
 */

static bool
mslvtXform (const SS_UCS4* in,
  SS_UCS4* med, int *len)
{

  int i;
  for (i=0; i < *len; i++)
    switch(getJamoClass(in[i]))
    {
      case SD_JAMO_L: 
        med[i]=in[i]+LC_OFFSET;
        break;
      case SD_JAMO_V: 
        med[i]=in[i]+VO_OFFSET;
        break;
      case SD_JAMO_T: 
        med[i]=in[i]+TC_OFFSET;
        break;
      default:
        med[i]=in[i];
    }

  for (i=0; LC_Clusters[i].seq[0]; i++)
    jamo_srch_repl(&LC_Clusters[i],med,len); 

  for (i=0; VO_Clusters[i].seq[0]; i++)
    jamo_srch_repl(&VO_Clusters[i],med,len); 

  for (i=0; TC_Clusters[i].seq[0]; i++)
    jamo_srch_repl(&TC_Clusters[i],med,len); 

  return true;
}

/**
 * search for cluster->seq in 'in' and replace it with cluster->liga in place. 
 * returns the difference in length between before and after the replacement.
 */

static int 
jamo_srch_repl(_OTF_mslvtJamo *cluster,
  SS_UCS4 *in, int *len)
{
  int i,j;
  bool matched=false;
  int mstart=0;
  int mlen=0;

  for (i=0; i<*len; i++)
  {
    matched=true;
    for (j=0; i+j < *len && j<MAX_BAS_JAMOS && cluster->seq[j] ;j++)
      if ( in[i+j] != cluster->seq[j] ) 
      {
        matched=false;
        break;
      }
    if ( i+j==*len && j<MAX_BAS_JAMOS && cluster->seq[j] )
      matched=false;
    if (matched) 
    {
      mstart=i;
      mlen=j;
      break;
    }
  }

  if ( !matched ) return 0;

  in[mstart]=cluster->liga;
  for (i=mstart+mlen; i<*len; i++)
    in[i-mlen+1]=in[i];

  *len=*len-mlen+1;
  return mlen-1;
}

/**
 *  return jamo_class of shifted code points for extended Jamos
 *  used in mslvt fonts. 
 */
static int 
get_jamo_class2(SS_UCS4 uc)
{
  switch(uc & 0xff00)
  {
    case LC_TMPPOS: return SD_JAMO_L;
    case VO_TMPPOS: return SD_JAMO_V;
    case TC_TMPPOS: return SD_JAMO_T;
    default: return SD_JAMO_X;
  }
}


/**
 * This is mslvt.otp from Jin-Hwan Cho <chofchof@ktug.or.kr>.
 * Extended by Jungshik Shin <jshin@mailaps.org> to support
 * additional Jamo clusters not encoded in U+1100 Jamo block
 * as precomposed Jamos.
 */

/**
 * table of choseong(Leading consonant) - till 115f 
 * followed by 34 additional consonant clusters for which separate glyphs
 * exist in O*.ttf fonts
 */
static SS_UCS4 tableL[130] = {
  1,  2,  4, 12, 14, 20, 36, 42, 46, 62, 70, 85,100,102,108,113,
114,116,120,  5,  6,  7,  8, 13, 23, 26, 34, 35, 39, 41, 43, 44,
 45, 47, 48, 49, 50, 51, 52, 54, 55, 57, 58, 60, 61, 63, 64, 65,
 66, 67, 68, 69, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83,
 84, 86, 87, 89, 90, 91, 92, 93, 94, 95, 96, 97, 99,101,104,105,
106,107,109,110,111,112,117,119,122,123,  0,  0,  0,  0,  0,  0,
  3,  9, 10, 11, 15, 16, 17, 18, 19, 21, 22, 24, 25, 27, 28, 29,
 30, 31, 32, 33, 37, 38, 40, 53, 56, 59, 71, 88, 98,103,115,118,
121, 124
};

/**
 * table of jungseong(Vowel) - 0x1160 (Vowel filler) excluded.
 * Glyphs for 28 additional vowel clusters (not given separate
 * code points in U+1100 block) are available in O*ttf fonts.
 * Total count: 94 = 66 (in U+1100 block) + 28 (extra.)
 */
static SS_UCS4 tableV[94] = {
   0,  4,  5,  9, 10, 14, 15, 19, 20, 21, 22, 32, 33, 42, 45, 47,
  51, 53, 63, 70, 72,  1,  2,  6,  7, 11, 12, 13, 17, 18, 25, 26,
  28, 29, 31, 36, 37, 39, 40, 41, 43, 44, 46, 49, 50, 54, 56, 57,
  58, 59, 61, 62, 68, 69, 71, 73, 74, 79, 82, 84, 86, 87, 89, 91,
  92, 93,  3,  8, 16, 23, 24, 27, 30, 34, 35, 38, 48, 52, 55, 60,
  64, 65, 66, 67, 75, 76, 77, 78, 80, 81, 83, 85, 88, 90
};

/**
 * table of jongseong(Trailing consonant).
 * glyphs for 59 additional trailing consonant clusters (not given separate
 * code points in U+1100 blocks) are available in O*ttf fonts.
 * Total count: 141 = 82 (in U+1100 block) + 59 (extra.)
 */
static SS_UCS4 tableT[141] = {
   0,  1,  5, 10, 17, 20, 21, 32, 33, 42, 46, 52, 57, 58, 59, 63,
  78, 84, 91, 98,109,123,127,128,129,130,135,  3,  6, 11, 13, 15,
  16, 19, 22, 25, 35, 37, 38, 39, 40, 43, 44, 48, 50, 51, 53, 54,
  56, 60, 64, 67, 69, 71, 72, 73, 75, 76, 77, 80, 88, 89, 90, 92,
  93, 94, 96,106,110,111,114,115,117,119,120,131,134,136,137,138,
 139,140,  2,  4,  7,  8,  9, 12, 14, 18, 23, 24, 26, 27, 28, 29,
  30, 31, 34, 36, 41, 45, 47, 49, 55, 61, 62, 65, 66, 68, 70, 74,
  79, 81, 82, 83, 85, 86, 87, 95, 97, 99,100,101,102,103,104,105,
 107,108,112,113,116,118,121,122,124,125,126,132,133
};

// Which of six glyphs to use for choseong(L) depends on 
// the following vowel and whether or not jongseong(T) is present
// in a syllable.

//shape Number of choseong(L) w.r.t. jungseong(V) without jongseong(T)
static SS_UCS4 tableNLV[94] = {
  0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 2, 2, 1, 1,
  1, 2, 2, 1, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1,
  1, 2, 2, 1, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 1,
  1, 1, 2, 1, 2, 2, 1, 0, 0, 3, 3, 3, 0, 2, 1, 2,
  1, 2, 3, 3, 0, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2,
  3, 3, 3, 2, 3, 0, 0, 0, 3, 3, 2, 0, 2, 2
};

// shape Number of choseong(L) w.r.t. jungseong(V) with jongseong(T)
static SS_UCS4 tableNLVT[94] = {
  1, 1, 1, 1, 1, 1, 1, 1, 5, 4, 4, 4, 5, 5, 4, 4,
  4, 5, 5, 4, 1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
  4, 5, 5, 4, 4, 4, 5, 4, 4, 4, 4, 4, 5, 4, 4, 4,
  4, 4, 5, 4, 5, 5, 4, 1, 1, 4, 4, 4, 1, 5, 4, 5,
  4, 5, 1, 1, 1, 1, 1, 1, 5, 4, 4, 4, 4, 4, 4, 5,
  4, 4, 4, 5, 4, 1, 1, 1, 4, 4, 5, 1, 4, 4
};

// shape Number of jongseong(T) w.r.t. jungseong(V)
// Which of four glyphs to use for jongseong(T) depends on 
// the preceding vowel.
static SS_UCS4 tableNTV[94] = {
  0, 2, 0, 2, 1, 2, 1, 2, 3, 0, 2, 1, 3, 3, 1, 2,
  1, 3, 3, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2,
  2, 3, 3, 0, 2, 1, 3, 1, 0, 2, 1, 2, 3, 0, 1, 2,
  1, 2, 3, 1, 3, 3, 1, 2, 2, 1, 1, 1, 1, 3, 1, 3,
  1, 3, 0, 1, 0, 0, 0, 2, 3, 0, 2, 1, 1, 2, 2, 3,
  0, 0, 0, 3, 0, 2, 2, 2, 1, 0, 1, 2, 1, 1
};


/**
 * This is a JAMO specific code. It tries to find and cache 
 * Jamo.
 * @param in are the input jamos
 * @param len is the length of input jamos
 * @param out is filled with JAMO glyphIndeces
 * @return true if JAMO conversion was successful
 */
bool
SFontTTF::findJamoGlyphs (const SS_UCS4* in, 
  unsigned int len, SV_GlyphIndex* out)
{
  if (hardWire != SS_MSLVT) return false;

  if (len < 2) 
#if 0
  { // deal with irregular cases of stand-alone Jamos
    int jclass=getJamoClass(in[0]);
    if (jclass == SD_JAMO_X) return false;

    SS_UCS4 result;
    switch (jclass)
    {
      case SD_JAMO_L:
        result = tableL[in[0]- 0x1100]*6 + 0x4e00;
        break;
      case SD_JAMO_V:
        result = tableV[in[0]- 0x1161]*2 + 0x5102;
        break;
      case SD_JAMO_T:
        result = tableT[in[0]- 0x11a8]*4 + 0x5207;
        break;
      default:
        return false;
    }
    SUniMap umap = charEncoder;
    if (!umap.isOK())
    {
       return false;
    }
    SS_GlyphIndex gi =  findGlyph(result);
    if (gi==0) 
    {
       out->clear(); 
       return false;
    }
    out->append (gi);
    return true;
  }
#else
    return false;
#endif

  /* VC++ 6.0 does not like this. */
  SS_UCS4* med = new SS_UCS4[len]; 
  CHECK_NEW (med);

  if ( ! mslvtXform(in,med,(int *) &len) )
  {
    delete[] med;
    return false;
  }

  SS_UCS4 tone = med[len-1];
  if (tone == 0x302e || tone == 0x302f)
  {
    if (len > 4 || len < 3)
    { 
      delete[] med;
      return false;
    }
  }
  else
  {
    tone = 0;
    if (len > 3)
    {
      delete[] med;
      return false;
    }
  }

  bool is3 = (tone==0 && len==3) || (tone!=0 && len==4);

  int jclass0 = get_jamo_class2(med[0]);
  int jclass1 = get_jamo_class2(med[1]);
  int jclass2 = is3 ? get_jamo_class2(med[2]) : SD_JAMO_T;

  if (jclass0 != SD_JAMO_L || jclass1 != SD_JAMO_V || jclass2 != SD_JAMO_T)
  {
    delete[] med;
    return false;
  }

  SV_UCS4 result;
  /* For TTF tone comes last */
  if (tone)
  {
    result.append (tone);
  }

  // Now that med[0..2] are identified as L,V, and T, it's safe to 
  // shift them back to U+1100 block although their ranges overlap
  // each other.
  med[0]-=LC_OFFSET;
  med[1]-=VO_OFFSET;
  med[2]-=TC_OFFSET;

  if (med[1] == 0x1160) /* filler */
  {
    if (!is3)
    {
      result.append (tableL[med[0]-0x1100]*6 + 0x4e00);
    }
    else /* len == 3 */
    {
      result.append (tableL[med[0]-0x1100]*6 + 0x4e05);
      result.append (tableT[med[2]-0x11a8]*4 + 0x5207);
    }
  }
  else 
  {
    if (!is3)
    {
      result.append (tableL[med[0]-0x1100]*6 + 
             tableNLV[med[1]-0x1161] + 0x4e00);
      result.append (tableV[med[1]-0x1161]*2 + 0x5102);
    }
    else /* len == 3 */
    {
      result.append (tableL[med[0]-0x1100]*6 + 
             tableNLVT[med[1]-0x1161] + 0x4e00);
      result.append (tableV[med[1]-0x1161]*2 + 0x5103);
      result.append (tableT[med[2]-0x11a8] * 4 
              + tableNTV[med[1]-0x1161] + 0x5204);
    }
  }

  for (unsigned int i=0; i<result.size(); i++)
  {
    SS_GlyphIndex gi =  findGlyph(result[i]);
    if (gi==0) 
    {
       /* There is no corresponding mark for the tone - just skip it */
       if (result[i] == 0x302e || result[i]==0x302f) continue;
       out->clear(); 
       return false;
    }
    out->append (gi);
  }
  return true;
}

/**
 * Find the glyphs for south indian scripts
 * and return them. Put the positions of characters
 * in mark2BaseList under key.
 * @param scriptcode is one of SD_LAO, SD_THAI, SD_TIBETAN 
 * @param in are the input characters
 * @param len is the length of input characters
 * @param out is filled with  glyphIndeces
 * @return true if conversion was successful
 */
bool
SFontTTF::findSouthIndicGlyphs (const SString& key, unsigned int scriptcode, 
  const char* script, const SS_UCS4* chars, unsigned int liglen, 
  SV_GlyphIndex* out)
{
  /* find all glyphs */
  unsigned int i;
  SS_GlyphIndex* gv = new SS_GlyphIndex[liglen];
  CHECK_NEW (gv);
  for (i=0; i<liglen; i++)
  {
    gv[i] = findGlyph (chars[i]);
    if (gv[i]==0)
    {
      delete[] gv;
      return false;
    }
  }

  /* find ligatures */
  bool base = false;
  liglen = getOTFLigatures (gv, liglen, script, "liga", 0, 0, &base);
  if (liglen == 0) return false; /* never */
  if (liglen==1)
  {
    out->append (gv[0]);
    delete[] gv;
    return true;
  }
  /* find the positions of these glyphs */
  storeMarkPositions (key, gv, liglen); 

  /* copy glyphs */
  for (i=0; i<liglen; i++)
  {
    out->append (gv[i]);
  }
  delete[] gv;
  return true;
}

/**
 * Store the positions of MarkToBase substitutions.
 * @param key is the key for this form.
 *  such a key can be constructed this way:
 *  SString key ((char*)&in, sizeof (SS_UCS4));
 * @param gv is the glyph array
 * @param liglen is the length of the array
 */
bool 
SFontTTF::storeMarkPositions (const SString& key, 
  const SS_GlyphIndex* gv, unsigned int liglen)
{
  SV_INT positions;
  bool haspos = false;
  int cxy = 0; int ix = 0; int iy = 0;
  int currentw = 0;
  positions.append (currentw); /* x = 0; y = 0 */
  unsigned int i;

  SS_GlyphIndex theBase = gv[0]; 
  for (i=1; i<liglen; i++)
  {
    unsigned int w = getGlyphWidth (theBase);
    getOTFMarkToBase (theBase, gv[i], &ix, &iy);
    if (ix!=0 || iy!=0)
    {
      haspos = true;
#if DEBUG_POSITIONS
    fprintf (stderr, "MarkToBase[%u.%u]=%d,%d. BaseWidth=%d Position=%d\n", 
       gv[i-1], gv[i], ix, iy, w, ix + currentw + w);
#endif
      ix = ix + currentw;
      /* dont increment currentw, base is previous  */
    }
    else
    {
      /* move to next position */
      currentw += w;
      ix = currentw;
      /* this might be a base */
      theBase = gv[i];
    }
    /* store positions in compressed format */
    if (ix > 0xfffe)
    {
      /* bad luck - it usually wont happen 
         unless the cluster is really long */
      currentw = 0xfffe;
      ix = 0xfffe;
    }
#if DEBUG_POSITIONS
    fprintf (stderr, "PUT MarkToBase[%u.%u]=%d,%d.\n", gv[i-1], gv[i], ix, iy);
#endif
    cxy = (iy << 16) & 0xffff0000;
    cxy = cxy | (ix & 0xffff);
    positions.append (cxy);
  }
  /* adjust mark to marks */
  theBase = gv[0]; 
  currentw = 0;
  for (i=1; i<liglen; i++)
  {
    /* False means from end. Taking mark size as zero. */
    getOTFMarkToMark (gv[i-1], gv[i], &ix, &iy);

    if (ix!=0 || iy!=0)
    {
#if DEBUG_POSITIONS
    fprintf (stderr, "MarkToMark[%u->%u]=%d,%d\n", gv[i-1], gv[i], ix, iy);
#endif
      int mxy = positions[positions.size()-1];
      haspos = true;

      int pix = mxy & 0xffff;
      if (pix > 0x7fff) pix -= 0x10000 ;

      int piy = (mxy >> 16) & 0xffff;
      if (piy > 0x7fff) piy -= 0x10000;

      /* 
       * Modify mark position. 
       * TODO: Do we need to shift everything that comes after mark?  
       */
      ix = pix + ix;
      iy = piy + iy;
      /* store positions in compressed format */
      if (ix > 0xfffe)
      {
        /* bad luck - it usually wont happen 
           unless the cluster is really long */
        currentw = 0xfffe;
        ix = 0xfffe;
      }
      cxy = (iy << 16) & 0xffff0000;
      cxy = cxy | (ix & 0xffff);
      positions.replace (i, cxy);
      haspos = true;
    }
  }

  /* save positions */
  if (haspos)
  {
    mark2BaseList.put (key, positions);
#if DEBUG_POSITIONS
    for (unsigned int k = 0; k<positions.size(); k++)
    {
      int xydiff = positions[k];
      int xdiff = xydiff & 0xffff;
      if (xdiff > 0x7fff) xdiff -= 0x10000 ;
      int ydiff = (xydiff >> 16) & 0xffff;
      if (ydiff > 0x7fff) ydiff -= 0x10000;
      fprintf (stderr, "PutPosition[%u]=%d,%d\n", k, xdiff, ydiff);
    }
#endif
  }
  /* fallback to kerning */
  /* width will be found */
  char2Width.put (key, currentw);
  return haspos;
}

/**
 * @param baseGl is the base glyph
 * @param markGlyph is the mark that needs to get positioned. 
 * @param ix holds the x offset for the mark.
 * @param iy holds the y offset for the mark.
 */
void
SFontTTF::getOTFMarkToBase (SS_GlyphIndex baseGl, 
  SS_GlyphIndex markGlyph, int* ix, int* iy) 
{
  SS_GlyphIndex gv[2];
  gv[0] = baseGl;
  gv[1] = markGlyph;

  /* Relative to base */
  int x[2]; x[1] = 0;
  int y[2]; y[1] = 0;

  getPositions (4, gv, 2, 0, 0, x, y); /* 4 is mark-to-base */

  *ix = x[1];
  *iy = y[1];
  return;
}

/**
 * @param m0 is the mark glyph
 * @param m1 is the mark that needs to get positioned. 
 * @param ix holds the x offset for the mark.
 * @param iy holds the y offset for the mark.
 */
void
SFontTTF::getOTFMarkToMark (SS_GlyphIndex m0, 
  SS_GlyphIndex m1, int* ix, int* iy) 
{
  SS_GlyphIndex gv[2];
  gv[0] = m0;
  gv[1] = m1;

  /* Relative to mark */
  int x[2]; x[1] = 0;
  int y[2]; y[1] = 0;

 getPositions (6, gv, 2, 0, 0, x, y); /* 4 is mark-to-mark */

  *ix = x[1];
  *iy = y[1];
  return;
}

/**
 * Get the positions
 * @param basewidth is used only if advanced is set.
 */
bool
SFontTTF::getPositions(int feature, const SS_GlyphIndex* gv,
  unsigned int gvsize, const char* _featurelist, 
  const char* _script, int* xpos, int* ypos)
{

  GPOS_HEAD* gposh = (GPOS_HEAD*) tables[SS_TB_GPOS];
  if (gposh == 0 || ntohl (gposh->version) != 0x00010000)
  {
    return false;
  }
  /* filtering features */
  bool nonfeature = false;
  SBinHashtable<int> features;
  if (_featurelist != 0 && _featurelist[0] != 0 && _featurelist[1] != 0)
  {
    SString f = SString(_featurelist);
    if (f[0] == '!')
    {
      nonfeature = true;
      f.remove (0);
    }
    SStringVector v(f);
    for (unsigned int i=0; i<v.size(); i++)
    {
      SString s(v[i]);
      if (s.size()==4)
      {
        features.put (s, 1);
      }
    }
  }

  int ofeat = ntohs(gposh->featureList);
  OTF_FeatureList*  featureList = (OTF_FeatureList*) ((SD_BYTE*)gposh + ofeat);
  SD_USHORT fcount = ntohs (featureList->count);
  int olookup = ntohs(gposh->lookupList);
  OTF_LookupList*  lookupList = (OTF_LookupList*) ((SD_BYTE*)gposh + olookup);
  //SD_USHORT lcount = ntohs (lookupList->count);

  if (_script == 0)
  {
    for (unsigned int i=0; i< fcount; i++)
    {
       SString tag (featureList->record[i].tag, 4);
       debugTag = tag;
       SD_USHORT loffset = ntohs (featureList->record[i].offset);
       /* got or not omitted */
       if (_featurelist)
       {
         if (nonfeature)
         {
           if (features.get(tag)) continue;
         }
         else
         {
           if (!features.get(tag)) continue;
         }
       }
       OTF_Feature* feat = (OTF_Feature*) ((char*)featureList+loffset);
       char tag_buf[5]; copy_raw_tag(tag_buf, featureList->record[i].tag);
       if (processGPOSFeature(name, feat, feature,
             lookupList, gv, gvsize, xpos, ypos, tag_buf)) return true;
    }
    return false;
  }
  unsigned int next = 0; /* iterator - just in case we have same lang twice */
  SString script (_script);
  OTF_LangSys* lsys = 0;
  /* HACK! getNextOTFLanguageSystem is using GSUB_HEAD* scriptList only
    this is why we can cast our GPOS* to GSUB* */
  while ((lsys = getNextOTFLanguageSystem (name, 
       (GSUB_HEAD*) gposh, script, &next))!=0)
  {
    SD_USHORT fcount = ntohs (lsys->featureCount);
    /* index lookupList through lsys->featureIndex */
    for (unsigned int i=0; i< fcount; i++)
    {
      unsigned int index = ntohs (lsys->featureIndex[i]) ;
      SString tag (featureList->record[index].tag, 4);
      if (_featurelist)
      {
        if (nonfeature)
        {
          if (features.get(tag)) continue;
        }
        else
        {
          if (!features.get(tag)) continue;
        }
      }
      SD_USHORT loffset = ntohs (featureList->record[index].offset);
      /* tags will have mystic ligature names and stuff like that - 
         don't check */
      OTF_Feature* feat = (OTF_Feature*) ((char*)featureList+loffset);
      char tag_buf[5]; copy_raw_tag(tag_buf, featureList->record[index].tag);
      if (processGPOSFeature(name, feat, feature,
              lookupList, gv, gvsize, xpos, ypos, tag_buf)) return true;
    }
  }
  return false;
}


/**
 * @param name is the fontname - used in error printouts.
 * @param lookuplist is the list of lookups.
 * @param substtype is 4 for MarkToBase substitution.
 * @param gv is the glyphindeces for positions
 * @param gvsize is the size of gv
 * @param xpos is the x position array same size as gv
 * @param ypos is the y position array same size as gv
 * @return true if positions were gained , and fill in
 *     xpos and ypos in that case.
 */
static bool
processGPOSFeature (const SString& name, OTF_Feature* feat, 
  int substtype, OTF_LookupList* lookupList, 
  const SS_GlyphIndex* gvarray, unsigned int gvsize,
  int* xpos, int* ypos, const char* feature_tag)
{
  /* faster */
  if (gvsize<2) return false;

  SD_USHORT nfcount = ntohs (feat->count);
  for (unsigned int j=0; j<nfcount; j++)
  {
    SD_USHORT rec = ntohs (feat->record[j]);
    SD_USHORT lrec = ntohs (lookupList->record[rec]);
    OTF_Lookup * ltable = (OTF_Lookup*)((char*)lookupList + lrec); 
    SD_USHORT type = lookupEffectiveType (ltable);
    if (type != substtype)
    {
      if (type != 2 && type != 4 && type != 6 && type != 8)
      {
#if PRINT_UNSUPPORTED
      fprintf (stderr, "UNSUPPORTED %*.*s GPOS type:%u.\n", 
          SSARGS(name), type);
#endif
      }
      continue;
    }

    /* Trace: emit lookup start */
    yudit_trace_emit(YUDIT_TRACE_LOOKUP_START, "GPOS", rec, feature_tag, 0);

    bool ret = false;
    switch (type)
    {
    case 2:
      ret = doPairAdjustment  (name, ltable, gvarray, gvsize, xpos, ypos);
      break;
    case 4:
      ret = doMarkToBase  (name, ltable, gvarray, gvsize, xpos, ypos);
      break;
    case 6:
      ret = doMarkToMark  (name, ltable, gvarray, gvsize, xpos, ypos);
      break;
    case 8:
      ret = doChainedPos  (name, lookupList, ltable, gvarray, gvsize, xpos, ypos);
    }

    /* Trace: emit lookup end */
    if (!ret)
    {
      yudit_trace_emit(YUDIT_TRACE_LOOKUP_SKIPPED, "GPOS", rec, feature_tag, 0);
    }
    yudit_trace_emit(YUDIT_TRACE_LOOKUP_END, "GPOS", rec, feature_tag, ret);

    if (ret) return ret;
  }
  return false;
}

static bool
doChainedPos (const SString& name, 
  OTF_LookupList* lookupList, OTF_Lookup* ltable,
  const SS_GlyphIndex* chars, unsigned int liglen,
  int* xpos, int* ypos)
{
  SD_USHORT ltcount = ntohs (ltable->count);
  for (unsigned int k=0; k<ltcount; k++)
  {
    SD_ULONG offset = lookupSubtableOffset (ltable, k);
    OTF_ChainedAdjustmentFormat1 *lformat1 =  
        (OTF_ChainedAdjustmentFormat1*) ((char*)ltable +  offset);
    /* check what we can */
    SD_USHORT cformat = ntohs (lformat1->format);
    if (cformat==2)
    {
      // Same as chain context substitution - reuse the code.
      // Ugly, but works.
      OTF_ChainContextSubstFormat2 *f2 =  
        (OTF_ChainContextSubstFormat2*) ( (char*)ltable +  offset);
      SD_USHORT coffset = ntohs (f2->coverage);
      OTF_CoverageFormat* cf = (OTF_CoverageFormat*) 
              ((char*)f2 +  coffset);
      SD_USHORT bcoffset = ntohs (f2->backtrackClassDef);
      SD_USHORT icoffset = ntohs (f2->inputClassDef);
      SD_USHORT lcoffset = ntohs (f2->lookaheadClassDef);
      SD_USHORT ccount = ntohs (f2->chainSubClassSetCnt);

      for (unsigned int i=0; i<ccount; i++)
      {
        SD_USHORT coffset = ntohs (f2->chainSubClassSet[i]);
        if (coffset == 0) continue;

        OTF_ChainSubClassSet * sset = 
             (OTF_ChainSubClassSet*) ((char*) f2 + coffset);

        SD_USHORT rcount = ntohs (sset->chainSubClassRuleCnt);
        for (unsigned int j=0; j<rcount; j++)
        {
          SD_USHORT roffset = ntohs (sset->chainSubClassRule[j]);

          OTF_ChainContextSubstFormat2_Backtrack *bformat =  
            (OTF_ChainContextSubstFormat2_Backtrack*) ((char*) sset + roffset);
          SD_USHORT bcount = ntohs(bformat->backtrackGlyphCount);
    
          OTF_ChainContextSubstFormat2_Input * iformat = 
            (OTF_ChainContextSubstFormat2_Input*) &bformat->coverage[bcount];
          SD_USHORT icount = ntohs(iformat->inputGlyphCount);

          if (icount == 0)
          {
             continue;
          }
    
          OTF_ChainContextSubstFormat2_Lookahead * lformat = 
            (OTF_ChainContextSubstFormat2_Lookahead*) &iformat->coverage[icount-1];
          SD_USHORT lcount = ntohs(lformat->lookaheadGlyphCount);
    
#if USE_UNTESTED_CODE
          OTF_ChainContextSubstFormat2_Subst * sformat = 
            (OTF_ChainContextSubstFormat2_Subst*) &lformat->coverage[lcount];
          SD_USHORT scount = ntohs(sformat->substGlyphCount);
#endif

          if ((unsigned int)bcount+icount+lcount>(unsigned int) liglen)
          {
            continue;
          }
          //----------------------------------------------------------------
          // Chain context - 2
          //----------------------------------------------------------------
          bool ok = true;
          unsigned int index = 0;

          /* Backtrack */
          for (unsigned int bi=0; bi<bcount; bi++)
          {
            SD_USHORT backtrack = ntohs (bformat->coverage[bcount-bi-1]);
            if (bcoffset == 0) 
            {
              ok = false; 
              break;
            }
            SD_USHORT clazz = glyphClass((char*)f2 + bcoffset, 
                  chars[index++]);
            if (clazz==0xffff || clazz != backtrack)
            {
              ok = false; 
              break;
            }
          }
          if (!ok) continue;
          SD_USHORT coverageIndex = getCoverageIndex (cf, chars[index++]);
          if (coverageIndex == 0xffff)
          {
             continue;
          }
          /* Input */
          for (unsigned int ii=1; ii<icount; ii++)
          {
            SD_USHORT input = ntohs (iformat->coverage[ii-1]);
            if (icoffset == 0) 
            {
              ok = false; 
              break;
            }
            SD_USHORT clazz = glyphClass((char*)f2 + icoffset, 
                chars[index++]);
            if (clazz==0xffff || clazz != input)
            {
              ok = false; 
              break;
            }
          }
          if (!ok) continue;
          /* Lookahead */
          for (unsigned int li=0; li<lcount; li++)
          {
            SD_USHORT lookahead = ntohs (lformat->coverage[li]);
            if (lcoffset == 0) 
            {
              ok = false; 
              break;
            }
            SD_USHORT clazz = glyphClass((char*)f2 + lcoffset,  
                   chars[index++]);
            if (clazz==0xffff || clazz != lookahead)
            {
              ok = false; 
              break;
            }
          }
          if (!ok) continue;
#if PRINT_UNSUPPORTED
      fprintf (stderr, "UNDEBUGGED %*.*s GPOS type:%u format=%u.\n", 
          SSARGS(name), 2, cformat);
#endif
#if USE_UNTESTED_CODE
          /* Substitute from outchar[om..scount] - assume scount < icount */
          unsigned int inlength = icount;
          bool ret = false;
          for (unsigned int m=0; m<(unsigned int)scount; m++)
          {
            SubstLookupRecord* sr = (SubstLookupRecord*)
              &sformat->substLookupRecord[m];
            SD_USHORT sqi = ntohs (sr->sequenceIndex);
            SD_USHORT loi = ntohs (sr->lookupListIndex);
            /* 
             * Perform substitution loi at outchar[om+sqi].
             * If is is just a single glyph substitution
             * replace outchar[om+sqi].
             * If n glyphs are made into 1 then
             * reduce inlen with n-1, and move data.
             */
            SD_USHORT mlrec = ntohs (lookupList->record[loi]);
            OTF_Lookup * mltable = (OTF_Lookup*)((char*)lookupList + mlrec); 
            SD_USHORT mtype = ntohs (mltable->type);
            unsigned int at = (unsigned int) sqi;
            if (at < inlength && at > 0)
            {
               unsigned int lsize = 2;
              int savex = xpos[at-1];
              int savey = ypos[at-1];
              switch (mtype)
              {
              case 2:
                ret = doPairAdjustment  (name, mltable, &chars[at-1], lsize, 
                   &xpos[at-1], &ypos[at-1]) || ret;
                break;
              case 4:
               ret = doMarkToBase  (name, mltable, &chars[at-1], lsize, 
                   &xpos[at-1], &ypos[at-1]) || ret;
                break;
              case 6:
               ret = doMarkToMark  (name, mltable, &chars[at-1], lsize, 
                   &xpos[at-1], &ypos[at-1]) || ret;
              }
              xpos[at-1] = savex;
              ypos[at-1] = savey;
            }
          }
          return ret;
#else  /* USE_UNTESTED_CODE */
          return false;
#endif
        }
        //----------------------------------------------------------------
        // Chain context - 2
        //----------------------------------------------------------------
      }
    }
    else
    {
#if PRINT_UNSUPPORTED
      fprintf (stderr, "UNSUPPORTED %*.*s GPOS type:%u format=%u.\n", 
          SSARGS(name), 8, cformat);
#endif
    }
  }
  return false;
}

/*!
 * Do a MarkToBase substitution.
 */
/*
 * NOTE: Yudit's GPOS pair adjustment is a stub - it only checks coverage and
 * returns false - and its legacy 'kern' table support is disabled behind
 * #if 0 in SFontTTF::gpos.  Both are left exactly as they are: this library
 * reports what Yudit does, it does not extend the engine.
 */
static bool
doPairAdjustment (const SString& name, OTF_Lookup* ltable, 
  const SS_GlyphIndex* gvarray, unsigned int gvsize,
  int* xpos, int* ypos)
{
  SD_USHORT ltcount = ntohs (ltable->count);
  for (unsigned int k=0; k<ltcount; k++)
  {
    SD_ULONG offset = lookupSubtableOffset (ltable, k);
    OTF_PairAdjustmentFormat1 *lformat1 =  
        (OTF_PairAdjustmentFormat1*) ((char*)ltable +  offset);
    /* check what we can */
    SD_USHORT cformat = ntohs (lformat1->format);
    if (cformat==2)
    {
      OTF_PairAdjustmentFormat2 *lformat2 =  
        (OTF_PairAdjustmentFormat2*) ((char*)ltable +  offset);
      /* Try to get the mark coverage */
      SD_USHORT mcoffset = ntohs (lformat2->coverage);
      OTF_CoverageFormat* mCoverageFormat = (OTF_CoverageFormat*)
       ((char*)lformat2 +  mcoffset);
      SD_USHORT coverageIndex 
        = getCoverageIndex (mCoverageFormat, gvarray[0]);
      // No coverage.
      //SD_USHORT vf1 = ntohs (lformat2->valueFormat1);
      //SD_USHORT vf2 = ntohs (lformat2->valueFormat2);
      if (coverageIndex==0xffff)
      {
         continue;
      }
#if PRINT_UNDEBUGGED
      fprintf (stderr, "UNDEBUGGED %*.*s GPOS type:%u format=%u.\n", 
          SSARGS(name), 2, cformat);
#endif
    }
    else
    {
#if PRINT_UNSUPPORTED
      fprintf (stderr, "UNSUPPORTED %*.*s GPOS type:%u format=%u.\n", 
          SSARGS(name), 2, cformat);
#endif
    }
  }
  return false;
}
/*!
 * Do a MarkToBase substitution.
 */
static bool
doMarkToBase (const SString& name, OTF_Lookup* ltable, 
  const SS_GlyphIndex* gvarray, unsigned int gvsize,
  int* xpos, int* ypos)
{
  SD_USHORT ltcount = ntohs (ltable->count);
  for (unsigned int k=0; k<ltcount; k++)
  {
    SD_ULONG offset = lookupSubtableOffset (ltable, k);
    OTF_MarkBasePosFormat1 *lformat1 =  
        (OTF_MarkBasePosFormat1*) ((char*)ltable +  offset);
    /* check what we can */
    SD_USHORT cformat = ntohs (lformat1->format);
    if (cformat != 1) 
    {
      continue;
    }

    /* Try to get the mark coverage */
    SD_USHORT mcoffset = ntohs (lformat1->markCoverage);
    OTF_CoverageFormat* mCoverageFormat = (OTF_CoverageFormat*)
       ((char*)lformat1 +  mcoffset);
    SD_USHORT markCoverageIndex 
        = getCoverageIndex (mCoverageFormat, gvarray[1]);

    SD_USHORT markOffset = ntohs (lformat1->markArray);
    OTF_MarkArray* markArray = (OTF_MarkArray*)((char*)lformat1 
           + markOffset);
    SD_USHORT markCount = ntohs (markArray->markCount);
    if (markCoverageIndex >= markCount)
    {
      continue; /* no coverage */
    }

    /* Try to get the base coverage */
    SD_USHORT bcoffset = ntohs (lformat1->baseCoverage);
    OTF_CoverageFormat* bCoverageFormat = (OTF_CoverageFormat*)
       ((char*)lformat1 +  bcoffset);
    SD_USHORT baseCoverageIndex 
        = getCoverageIndex (bCoverageFormat, gvarray[0]);

    SD_USHORT baseOffset = ntohs (lformat1->baseArray);
    OTF_BaseArray* baseArray = (OTF_BaseArray*)((char*)lformat1 
           + baseOffset);
    SD_USHORT baseCount = ntohs (baseArray->baseCount);
    if (baseCoverageIndex >= baseCount)
    {
      continue;
    }
    /* get the class count */
    SD_USHORT ccount = ntohs (lformat1->classCount);

    /* figure out what class the mark wants */
    OTF_MarkRecord* markRecord = (OTF_MarkRecord*) 
           &markArray->markRecord[markCoverageIndex];
    SD_USHORT markClass = htons (markRecord->markClass);

    //SD_USHORT markClass = htons (markRecord->markClass);
    if (markClass >= ccount)
    {
      continue;
    }
    /* load this class */
    OTF_BaseRecord* baseRecord = (OTF_BaseRecord*) 
          &baseArray->baseRecord[baseCoverageIndex*ccount]; /* clssize */

    /* Anchors are offset from markArray, baseArray */
    SD_USHORT boffset =(SD_USHORT)htons(baseRecord->baseAnchor[markClass]);
    OTF_Anchor* banchor = (OTF_Anchor*) ((char*) baseArray + boffset);

    SD_USHORT moffset  = (SD_USHORT) htons(markRecord->markAnchor);
    OTF_Anchor* manchor = (OTF_Anchor*) ((char*) markArray + moffset);
    
    unsigned short mformat = htons (manchor->format);
    unsigned short bformat = htons (banchor->format);
    // We will ignore device table for 2, 3.
    if (mformat>3 || bformat>3)
    {
#if PRINT_UNDEBUGGED
      fprintf (stderr, 
         "UNSUPPORTED %*.*s GPOS type:%u format=%u base=%u anchor=%u.\n", 
         SSARGS(name), 4, cformat, bformat, mformat);
#endif
      continue;
    }

    short bx = (short) htons (banchor->x);
    short by = (short) htons (banchor->y);

    short mx = (short) htons (manchor->x);
    short my = (short) htons (manchor->y);
    /* Unnecessary to calculate hot-spot in a more complicated way */
    mx = bx - mx;
    my = by - my;

    xpos[0] = bx;
    ypos[0] = by;

    /* return delta in xpos[1] and ypos[1] */
    xpos[1] = mx;
    ypos[1] = my;
    return true;
  }
  return false;
}

/*!
 * Do a MarkToMark substitution.
 */
static bool
doMarkToMark (const SString& name, OTF_Lookup* ltable, 
  const SS_GlyphIndex* gvarray, unsigned int gvsize,
  int* xpos, int* ypos)
{
  SD_USHORT ltcount = ntohs (ltable->count);
  for (unsigned int k=0; k<ltcount; k++)
  {
    SD_ULONG offset = lookupSubtableOffset (ltable, k);
    OTF_MarkMarkPosFormat1 *lformat1 =  
        (OTF_MarkMarkPosFormat1*) ((char*)ltable +  offset);
    /* check what we can */
    SD_USHORT cformat = ntohs (lformat1->format);
    if (cformat != 1) 
    {
#if PRINT_UNSUPPORTED
      fprintf (stderr, "UNSUPPORTED %*.*s GPOS type:%u format=%u.\n", 
          SSARGS(name), 6, cformat);
#endif
      continue;
    }

    /* Try to get the mark coverage */
    SD_USHORT mcoffset = ntohs (lformat1->mark1Coverage);
    OTF_CoverageFormat* mCoverageFormat = (OTF_CoverageFormat*)
       ((char*)lformat1 +  mcoffset);
    SD_USHORT markCoverageIndex 
        = getCoverageIndex (mCoverageFormat, gvarray[1]);

    SD_USHORT markOffset = ntohs (lformat1->mark1Array);
    OTF_MarkArray* markArray = (OTF_MarkArray*)((char*)lformat1 
           + markOffset);
    SD_USHORT markCount = ntohs (markArray->markCount);
    if (markCoverageIndex >= markCount)
    {
      continue;
    }
    /* Try to get the mark2 coverage */
    SD_USHORT bcoffset = ntohs (lformat1->mark2Coverage);
    OTF_CoverageFormat* bCoverageFormat = (OTF_CoverageFormat*)
       ((char*)lformat1 +  bcoffset);
    SD_USHORT baseCoverageIndex 
        = getCoverageIndex (bCoverageFormat, gvarray[0]);

    SD_USHORT baseOffset = ntohs (lformat1->mark2Array);
    OTF_BaseArray* baseArray = (OTF_BaseArray*)((char*)lformat1 
           + baseOffset);
    SD_USHORT baseCount = ntohs (baseArray->baseCount);
    if (baseCoverageIndex >= baseCount)
    {
      continue;
    }

    /* get the class count */
    SD_USHORT ccount = ntohs (lformat1->classCount);

    /* figure out what class the mark wants */
    OTF_MarkRecord* markRecord = (OTF_MarkRecord*) 
           &markArray->markRecord[markCoverageIndex];

    SD_USHORT markClass = htons (markRecord->markClass);
    if (markClass >= ccount)
    {
      continue;
    }
    /* load this class */
    OTF_BaseRecord* baseRecord = (OTF_BaseRecord*) 
          &baseArray->baseRecord[baseCoverageIndex*ccount]; /* clssize */

    /* Anchors are offset from markArray, baseArray */
    SD_USHORT boffset =(SD_USHORT)htons(baseRecord->baseAnchor[markClass]);
    OTF_Anchor* banchor = (OTF_Anchor*) ((char*) baseArray + boffset);

    SD_USHORT moffset  = (SD_USHORT) htons(markRecord->markAnchor);
    OTF_Anchor* manchor = (OTF_Anchor*) ((char*) markArray + moffset);
    
    unsigned short mformat = htons (manchor->format);
    unsigned short bformat = htons (banchor->format);

    // We will ignore device table for 2, 3.
    if (mformat>2 || bformat>3)
    {
#if PRINT_UNDEBUGGED
      fprintf (stderr, 
         "UNSUPPORTED %*.*s GPOS type:%u format=%u base=%u anchor=%u.\n", 
         SSARGS(name), 4, cformat, bformat, mformat);
#endif
      continue;
    }

    short bx = (short) htons (banchor->x);
    short by = (short) htons (banchor->y);

    short mx = (short) htons (manchor->x);
    short my = (short) htons (manchor->y);
/*
    bool isrl = ((flag & 0x0001)!=0);
    fprintf (stderr, "Found b=%d,%d m=%d,%d  %d rl=%d.\n", 
        bx, by, mx, my, basewidth, isrl);
*/
    /* Unnecessary to calculate hot-spot in a more complicated way */
    mx = bx - mx;
    my = by - my;
    xpos[0] = bx;
    ypos[0] = by;
    /* return delta in xpos[1] and ypos[1] */
    xpos[1] = mx;
    ypos[1] = my;
    return true;
  }
  return false;
}

/*!
 * \brief Find the glyph index for the character. 
 * \param in is the unicode-encoded character.
 * \return the glyph index or null.
 */
SS_GlyphIndex
SFontTTF::gindex (SS_UCS4 in)
{
  return findGlyph (in, false);
}

/*!
 * \brief Get the width of the unscaled glyph.
 * \param in is the glyph index.
 * \return the width of the glyph.
 *  If width is negative or 0, then this mark
 *  should be aligned to the end of the previous
 *  character:
 *   <-------base------->
 *   x-------x----------x 
 *           <--length-->
 */
int
SFontTTF::gwidth (SS_GlyphIndex in)
{
  return getWidth(in);
}

//----------------------------------------------------------------------------
//      SFontLookup Interface
//----------------------------------------------------------------------------

/*!
 * \brief Perform a glyph/ligature substitution.
 * \param feature is a 4-character OTF-feature, like "gsub" 
 * \param in is the input glyph index array.
 * \param in_size is the size of input glyph index array.
 * \param start is the starting point of the substitution
 * \param out is the output will be used to put the Glyphs in.
 *      At least the input glyph size should be allocated.
 * \param out_size will contain how many glyphs were placed in out 
 *      array.
 * \param script is the code of the script, like "deva" 
 * \param is_contextual will be true after a successful 
 *     chaining contextual substitution.
 * \return how many glyphs should disappear from the input 
 *     array between start...start + retvle -1.
 */
unsigned int
SFontTTF::gsub (const char* script, const char* feature, 
      const SS_GlyphIndex* in, unsigned int in_size,
      unsigned int* start,  SS_GlyphIndex *out, 
      unsigned int* out_size, bool* is_contextual)
{
  *is_contextual = false;
  unsigned int liglen;
  *start = 0;
  *out_size = 0;
  
  // 1: Glyph Substitution.
  liglen = getOTFLigature (script, feature, in, in_size, out, 1);
  if (liglen == 1)
  {
    *start = 0;
    *out_size=1;
    return 1;
  }
  // 4: Ligature Substitution.
  liglen = getOTFLigature (script, feature, in, in_size, out, 4);
  if (liglen >= 2)
  {
    *start = 0;
    *out_size = 1;
    return liglen;
  }
  // 5: Contextual Substitution
  liglen = getOTFLigature (script, feature, in, in_size, out, 5);
  if (liglen == 0)
  {
    // 6: Chaining Contextual Substitution
    liglen = getOTFLigature (script, feature, in, in_size, out, 6);
  }

  // I tested this thing when the sizes were the same...
  if (liglen > 0)
  { 
    *is_contextual = true;
    // find out start end and position...
    unsigned int s = 0;
    unsigned int eo = liglen;
    unsigned int ei = in_size;

    while (s < liglen && out[s] == in[s]) s++;
    while (eo > 0 && ei > 0 && out[eo-1] == in[ei-1])
    {
       ei--; eo--;
    }

    /* nothing has changed! */
    if (s>=eo || s>=ei) return 0;

    /* cut out the s..eo window move stuff down */
    for (unsigned int i=0; i<eo-s; i++) out[i] = out[s+i];

    *start = s;
    *out_size = eo-s;
    return ei-s;
  }
  return 0;
}

/*!
 * \brief Get the relative position of glyphs.
 * \param script is the code of the script, like "deva" 
 * \param feature is a 4-character OTF-feature, like "blwm" 
 * \param in contains 2 glyphs. The first glyph assumed to be 
 *     at position 0.0.
 * \param x will contain the relative x position of the
 *     second glyph. If no position can be retrieved the
 *     method will not change the value of this. 
 * \param y will contain the relative y position of the
 *     second glyph. If no position can be retrieved the
 *     method will not change the value of this. 
 * \return true if a position was retrieved.
 */
bool
SFontTTF::gpos (const char* script, const char* feature,
      const SS_GlyphIndex* in, int* x,  int* y)
{
  int lx[2];
  int ly[2];
  lx[0] = 0;
  ly[0] = 0;
  lx[1] = 0;
  ly[1] = 0;
  /* 
   * getPositions stores deltas into x[1] and y[1]
   * and hotspot into x[0] and y[0] - which is ignored here.
   */ 
  // 4: Mark To Base
  if (getPositions (4, in, 2, feature, script, lx, ly))
  {
    *x = lx[1];
    *y = ly[1];
    // Hack. some routines ignore our return values.
    if (*x == 0 && *y == 0) *x = 1;
    return true;
  }
  // 6: Mark To Mark
  if (getPositions (6, in, 2, feature, script, lx, ly))
  {
    *x = lx[1];
    *y = ly[1];
    // Hack. some routines ignore our return values.
    if (*x == 0 && *y == 0) *x = 1;
    return true;
  }
  // 8: Chained.
  if (getPositions (8, in, 2, feature, script, lx, ly))
  {
    *x = lx[1];
    *y = ly[1];
    // Hack. some routines ignore our return values.
    if (*x == 0 && *y == 0) *x = 1;
    return true;
  }
  /* NOTE: Yudit does not position glyphs with GPOS type 2 (pair adjustment)
   * or type 3 (cursive attachment) - doPairAdjustment only inspects coverage
   * and always returns false, and the legacy 'kern' table code is disabled
   * behind #if 0.  Nothing is added here, so the trace reports exactly that. */
  return false;
}
/*!
 * \return the glyph class:
 *   \li 0 - Unknown
 *   \li 1 - Base Glyph (single character spacing glyph)
 *   \li 2 - Base Glyph (single character spacing glyph)
 *   \li 3 - Mark Glyph (non-spacing combining glyph)
 *   \li 4 - Component Glyph (part of a single character, spacing glyph)
*/
unsigned int
SFontTTF::getGlyphClass(SS_GlyphIndex in)
{
  GDEF_HEAD* gdefh = (GDEF_HEAD*) tables[SS_TB_GDEF];
  if (gdefh == 0)
  {
    return 0;
  }
  if (ntohl (gdefh->version) != 0x00010000)
  {
    return 0;
  }
  SD_USHORT offset = htons (gdefh->glyphClassDef);
  if (offset == 0)
  {
    return 0; 
  }
  unsigned int ret = glyphClass ((char*) gdefh + offset, in);
  if (ret == 0xffff)
  {
    return 0;
  }
  return ret;
}

/*!
 * \brief try to attach mark to base.
 * \param where takes the following values:
 *   1 - below.
 * \return true on success. 
  * This code is undebugged.
 */
bool
SFontTTF::attach (SS_GlyphIndex base, SS_GlyphIndex mark, 
  int where, int* x, int* y)
{
  // Below.
  if (where == 1)
  {
    int bxmin, bymin, bxmax, bymax;
    if (!getBBOX (base, &bxmin, &bymin, &bxmax, &bymax))
    {
      return false;
    }
    int mxmin, mymin, mxmax, mymax;
    if (!getBBOX (mark, &mxmin, &mymin, &mxmax, &mymax))
    {
      return false;
    }
    // Dont do it if it overlaps 
    if (mymax >= bymin) return false;

    // Move it along x.
    int mb = (bxmin + bxmax) / 2;
    int mm = (mxmin + mxmax) / 2;
    *x = (mb - mm);
    *y = 0;
    return true;
  }
  return false;
  // This untested piece was an unsuccessful attempt on the same
  // thing.
#if 0
  GDEF_HEAD* gdefh = (GDEF_HEAD*) tables[SS_TB_GDEF];
  if (gdefh == 0)
  {
    return 0;
  }
  if (ntohl (gdefh->version) != 0x00010000)
  {
    return 0;
  }
  SD_USHORT offset = htons (gdefh->attachList);
  if (offset == 0)
  {
    return 0; 
  }
  OTF_AttachList* list = (OTF_AttachList*) ((char*) gdefh + offset);
  SD_USHORT gc =  htons (list->glyphCount);

  SD_USHORT coff = htons (list->coverage);
  OTF_CoverageFormat* cf  = (OTF_CoverageFormat*) ((char*) list + coff);

  SD_USHORT bc = getCoverageIndex (cf, base);
  if (bc >= gc) return false;

  SD_USHORT mc = getCoverageIndex (cf, mark);
  if (mc >= gc) return false;

  SD_USHORT bpoff = htons(list->attachPoint[bc]); 
  OTF_AttachPoint* bp = (OTF_AttachPoint*) ((char*) list + bpoff); 
  SD_USHORT bcount = ntohs (bp->pointCount);

  SD_USHORT mpoff = htons(list->attachPoint[mc]); 
  OTF_AttachPoint* mp = (OTF_AttachPoint*) ((char*) list + mpoff); 
  SD_USHORT mcount = ntohs (mp->pointCount);

  SH_Vector bpx;
  SH_Vector bpy;

  if (!getContours (base, &bpx, &bpy) || bpx.size()==0)
  {
    return  false;
  }

  SH_Vector mpx;
  SH_Vector mpy;

  if (!getContours (mark, &mpx, &mpy) || bpx.size()==0)
  {
    return  false;
  }

  // TODO: attach base 
  if (where == 1)
  {
    unsigned int i;
  //  fprintf (stderr, "XXX Base: %04X count=%u\n", base, bcount);
    for (i=0; i<bcount; i++)
    {
      SD_USHORT index = htons (bp->pointIndex[i]);
      if (mpx.size() <= index) return false;
   //   fprintf (stderr, "XXX base[%u] = %d,%d\n", i, 
    //      bpx[index], bpy[index]);
    }
 //   fprintf (stderr, "XXX Mark: %04X count=%u\n", mark, mcount);
    for (i=0; i<mcount; i++)
    {
      SD_USHORT index = htons (mp->pointIndex[i]);
      if (mpx.size() <= index) return false;
  //    fprintf (stderr, "XXX mark[%u] = %d,%d\n", i, 
   //       mpx[index], mpy[index]);
    }
  }
  return false;
#endif
}

/*!
 * \brief Get the contour points.
 */
bool
SFontTTF::getContours (SS_GlyphIndex glyphno, 
  SH_Vector * xc, SH_Vector *yc )
{
  SD_BYTE* gstart = (SD_BYTE *) tables["glyf"];
  if (gstart == 0) return false;

  TTF_GLYF* gtable;
  int len =0;
  if (longOffsets)
  {
     SD_ULONG* lloca = (SD_ULONG *) tables["loca"];
    if (lloca == 0) return false;
     unsigned int offs1 = ntohl (lloca[glyphno]);
     unsigned int offs2 = ntohl (lloca[glyphno+1]);
     gtable = (TTF_GLYF *) ((char*)gstart + offs1);
     len = offs2-offs1;
  }
  else
  {
     SD_USHORT* sloca = (SD_USHORT *) tables["loca"];
     if (sloca == 0) return false;
     gtable = (TTF_GLYF *) (gstart + (ntohs (sloca[glyphno]) << 1));
     len = (ntohs (sloca[glyphno+1]) - ntohs (sloca[glyphno])) << 1;
  }
  if (len <= 0)
  {
    return false;
  }
  TTF_GLYF* kludge = 0;
  if ((((unsigned long) gtable) & 1) != 0)
  {
    kludge = new TTF_GLYF[len]; 
    CHECK_NEW (kludge);
    memcpy (kludge, gtable, len * sizeof (TTF_GLYF));
    gtable = kludge;
  }

  int ncontours = (int) ((short)ntohs (gtable->numberOfContours));
  if (ncontours < 0) 
  {
    if (kludge) delete[] kludge;
    return false;
  }

  SD_USHORT* contour_end_pt = (SD_USHORT *) ((char *)gtable + sizeof(TTF_GLYF));

  int last_point = (int) ntohs (contour_end_pt[ncontours-1]);

  // length of instructions.
  int n_inst = (int) ntohs (contour_end_pt[ncontours]);

  // flags
  SD_BYTE* ptr = ((SD_BYTE *)contour_end_pt) + (ncontours << 1) + n_inst + 2;

  int j = 0; int k = 0;

  SBinVector<SD_BYTE> flags;
  while (k <= last_point)
  {
    flags.append (ptr[j]);
    if (ptr[j] & REPEAT)
    {
       for (int k1=0; k1 < ptr[j+1]; k1++)
       {
            k++;
            flags.append (ptr[j]);
       }
       j++;
    }
    j++; k++;
  }
  
  SH_Vector xrel;
  SH_Vector xcoord;

  for (k=0; k <= last_point; k++)
  {
    /* Process xrel */
    if (flags[k] & XSD_SHORT)
    {
      if (flags[k] & XSAME)
      {
        xrel.append (ptr[j]);
      }
      else
      {
        xrel.append (-ptr[j]);
      }
      j++;
    }
    else if (flags[k] & XSAME)
    {
      xrel.append (0);
    }
    else
    {
      xrel.append (ptr[j] * 256 + ptr[j+1]);
      j += 2;
    }
    /* Process x coordinate */
    if (k==0)
    {
      xcoord.append (xrel[k]);
    }
    else
    {
      xcoord.append (xrel[k] + xcoord[k-1]);
    }
  }

  SH_Vector yrel;
  SH_Vector ycoord;

  /* one more run fore yrel and ycoord */
  for (k=0; k <= last_point; k++)
  {
    if (flags[k] & YSD_SHORT)
    {
      if (flags[k] & YSAME)
      {
        yrel.append (ptr[j]);
      }
      else
      {
         yrel.append (- ptr[j]);
      }
      j++;
    }
    else if (flags[k] & YSAME)
    {
      yrel.append (0);
    }
    else
    {
      yrel.append (ptr[j] * 256 + ptr[j+1]);
      j += 2;
    }
    if (k==0)
    {
       ycoord.append (yrel[k]);
    }
    else
    {
       ycoord.append (yrel[k] + ycoord[k-1]);
    }
  }
  if (kludge) delete[] kludge;
   *xc = xcoord;
   *yc = ycoord;
  return true;
}
