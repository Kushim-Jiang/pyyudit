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

#include "stoolkit/SGlyph.h"
#include "stoolkit/SParagraph.h"

#include "stoolkit/SBinHashtable.h"
#include "stoolkit/SUtil.h"
#include "stoolkit/STextData.h"
#include "stoolkit/SCluster.h"

typedef enum {
  SD_SH_ISOLATED=0,
  SD_SH_INITIAL=1,
  SD_SH_MEDIAL=2,
  SD_SH_FINAL=3,

  /* Syriac U+0710 extra shapes */
  SD_SH_SYRIAC_ALAPH_FINAL_FJ=4,
  SD_SH_SYRIAC_ALAPH_ISOLATED_FN=5,
  SD_SH_SYRIAC_ALAPH_ISOLATED_FX=6,
  SD_BENGALI_INITIAL=7,
  SD_SH_NOSHAPE=SD_NOSHAPE
} SShape;

SBinHashtable<SGlyphShared*> glyphCache;

static char getSimpleShape (const SS_UCS4* now, 
   const SS_UCS4* before, const SS_UCS4* after);

/*------------------------------------------------------------------------------
 *      The class SFallbackShapes should not be in this file. 
 *------------------------------------------------------------------------------
 * This is used to cache shapes internally.
 * TODO: move this to another file.
 */
class SFallbackShapes 
{
public:
  SFallbackShapes(); 
  ~SFallbackShapes();
  const SS_UCS4* get (unsigned int shape, const SS_UCS4* ochars, 
    unsigned int sizes);
  void put (unsigned int shape, const SS_UCS4* ochars, const SS_UCS4* schars, unsigned int sizes);
private:
  SBinHashtable<SS_UCS4*> shape0; 
  SBinHashtable<SS_UCS4*> shape1; 
  SBinHashtable<SS_UCS4*> shape2; 
  SBinHashtable<SS_UCS4*> shape3; 
};

SFallbackShapes::SFallbackShapes()
{
}
SFallbackShapes::~SFallbackShapes()
{
  /* FIXME: cleanup allocated SS_UCS4* arrays */
}

const SS_UCS4*
SFallbackShapes::get (unsigned int shape, 
   const SS_UCS4* ochars, unsigned int sizes)
{
  const SS_UCS4* elem = 0;
  SString skey ((char*)ochars, sizes * sizeof (SS_UCS4));
  switch (shape)
  {
  case 0: elem = shape0.get (skey); break;
  case 1: elem = shape1.get (skey); break;
  case 2: elem = shape2.get (skey); break;
  case 3: elem = shape3.get (skey); break;
  }
  /* referecing live data */
  return elem;
}

void
SFallbackShapes::put (unsigned int shape, 
   const SS_UCS4* ochars, const SS_UCS4* schars, unsigned int sizes)
{
  SS_UCS4* vp = new SS_UCS4[sizes];
  CHECK_NEW (vp);
  SString skey ((char*)ochars, sizes * sizeof (SS_UCS4));

  /* copy over the shaped chars. */
  for (unsigned int i=0; i<sizes; i++)
  {   
    vp[i] = schars[i]; 
  }
  switch (shape)
  {
  case 0: shape0.put (skey, vp); break;
  case 1: shape1.put (skey, vp); break;
  case 2: shape2.put (skey, vp); break;
  case 3: shape3.put (skey, vp); break;
  }
}

static SFallbackShapes* fallbackShapes = 0;

/*------------------------------------------------------------------------------
 *  SGlyph: Copy constructor, assign operator, SObject clone, destructor
 *------------------------------------------------------------------------------
 */
/**
 * Create a glyph from another glyph
 * @param glyph is the other glyph.
 */
SGlyph::SGlyph (const SGlyph& glyph)
{
  embedding = glyph.embedding;
  state = glyph.state;
  underlined = glyph.underlined;
  selected = glyph.selected;
  usePrecomp = glyph.usePrecomp;
  decompCharClass = glyph.decompCharClass;
  currentShape = glyph.currentShape;
  shared = glyph.shared;
}

/**
 * Nothing to desctruct now.
 */
SGlyph::~SGlyph ()
{
}

/**
 * Assign a glyph.
 * @param glyph is the other glyph.
 */
SGlyph
SGlyph::operator=(const SGlyph& glyph)
{
  embedding = glyph.embedding;
  state = glyph.state;
  underlined = glyph.underlined;
  selected = glyph.selected;
  usePrecomp = glyph.usePrecomp;
  decompCharClass = glyph.decompCharClass;

  shared = glyph.shared;
  return *this;
}

/**
 * All objects are to define this.
 */
SObject*
SGlyph::clone() const
{
  return new SGlyph (*this);
}
/*------------------------------------------------------------------------------
 *                     SGlyph
 *------------------------------------------------------------------------------
 */
/**
 * I would like to protect this to this package.
 * Not meant to be used in other packages 
 */
SGlyph::SGlyph (SGlyphShared* _shared)
{
  selected = false;
  underlined = false;
  usePrecomp = true;
  decompCharClass = SD_CC_MAX;
  embedding = 0;
  shared = _shared;
  currentShape = shared->shaped ? 0 : SD_NOSHAPE; /* first or not shaped */
}

/**
 * This is the definition of one glyph. It may be composed of
 * several characters.
 * @param decomp is the character array in order. It ALWAYS contains chars.
 * @param comp is the composition character. It might be 0. In this
 *  case decomp needs to be used.
 * @param shaped is used only if shared is not cached. It tells if
 *  the glyph is shaped.
 * @param cluster - points to the cluster boundary
 * @param compindex - points to the extra composing boundary
 * YOU SHOULD ALSO SET usePrecomp aftwerwards.
 *
 * ONLY PRECOMPOSED OR SINGLE CHARS WILL BE CACHED!
 * comp is nonzero for precomposed chars.
 */
SGlyph::SGlyph (const SV_UCS4 &decomp, SS_UCS4 comp, 
   bool shaped, unsigned int cluster, unsigned int compindex, 
   bool _usePrecomposed)
{
  selected = false;
  underlined = false;
  usePrecomp = true;
  embedding = false;

  SS_UCS4 precomp = comp;

  SString skey = (_usePrecomposed && compindex<1) 
       ? SString((char*)&precomp, sizeof (SS_UCS4))
       : SString ();
  if (decomp.size() > 1) {
    skey = SString ((char*) decomp.array(), sizeof (SS_UCS4) * decomp.size());
  }
  decompCharClass = SD_CC_MAX;
  if (decomp.size() > 0 && precomp < 0x80000000) {
     decompCharClass = getCharClass(decomp[0]);
  }

  usePrecomp = _usePrecomposed;
  SGlyphShared* shr = 0;
  if ((shr=glyphCache.get (skey)))
  {
    shared = shr; 
//fprintf (stderr, "FOUND CACHED\n");
  }
  else
  {
    shared = new  SGlyphShared();
    shared->shaped = shaped;
    shared->cluster = cluster;
    shared->composing = compindex;

    /* 
     *  We save only 
     *  1. chars with precomposition 
     *  2. single chars
     */
    shared->precomposed = precomp;

    if (decomp.size() > 0  || compindex > 0)
    {
      shared->ucs4v = decomp;
    }
    if (skey.size())
    {
       glyphCache.put (skey, shared);
    }

    SS_UCS4 fchar = getFirstChar();
    shared->type = (char) getCharClass(fchar);
    shared->bidi = (char) getBiDiClass(fchar);

    shared->mirror = precomp ? getMirroredCharacter(precomp) : 0;
    shared->tab = (fchar == SD_CD_TAB);
    shared->lineend = (fchar==SD_CD_LF  || fchar==SD_CD_CR 
           || fchar==SD_CD_PS || fchar==SD_CD_LS || fchar==SD_CD_FF);

    /* clusters and stuff need shared->type and shared->bidi */
    if (cluster>0)
    {
       /* decompose */
       SS_UCS4 firstchar = decomp[cluster];
       shared->type = (char) getCharClass(firstchar);  
       shared->bidi = (char) getBiDiClass(firstchar);
    }
  }
  currentShape = shared->shaped ? 0 : SD_NOSHAPE; /* first or not shaped */
}

/**
 * Modify this glyph by adding extra composing characters
 * to it.
 * @param c is a new composing character to be added.
 * @return true if this was a composing character.
 */
bool
SGlyph::addComposing(SS_UCS4 c)
{
  if (c==0) return false;
  if (isEOP()) return false;
  SV_UCS4 chars = getChars();
  chars.append (c);
  unsigned int to = 0;
  SParagraph pg(chars, &to); 
  if (pg.size()!=1) return false;
  if (to != chars.size()) return false;
  
  shared = (SGlyphShared*) pg[0].shared; 
  usePrecomp = pg[0].usePrecomp;
//  decompCharClass = SD_CC_MAX;
  return true;
}
/**
 * @return the removed composing character if any
 * return 0 if there are no more composing characters.
 */
SS_UCS4
SGlyph::removeComposing()
{
  SV_UCS4 chars = getChars();
  if (chars.size()<2) return 0;
  SS_UCS4 c = chars[chars.size()-1];
  chars.truncate (chars.size()-1);

  unsigned int to = 0;
  SParagraph pg(chars, &to); 

  if (pg.size()!=1) return 0;
  if (to != chars.size()) return 0;
  shared = (SGlyphShared*) pg[0].shared; 
  usePrecomp = pg[0].usePrecomp;
//  decompCharClass = SD_CC_MAX;
  return c;
}


/*------------------------------------------------------------------------------
 *                    SGlyph Work on characters 
 *------------------------------------------------------------------------------
 */
/**
 * Get the composition of the glyph. If this is a shaped
 * Glyph this is the ORIGINAL composition - if any. 
 */ 
const SS_UCS4
SGlyph::getChar() const
{
  return shared->precomposed;
}

/*
 * Get the mirrored version of the glyph.
 */

const SS_UCS4
SGlyph::getMirroredChar() const
{
  return shared->mirror;
}

/**
 * Get the current shape of a shaped glyph.
 * If glyph is not shaping return getChar.
 */
const SS_UCS4
SGlyph::getShapedChar() const
{
  if (shared->shaped && currentShape != SD_NOSHAPE)
  {
    /* hard-coded 3 extra shapes for SYRIAC_ALAPH U+0710 */
    if ((unsigned int)currentShape == (unsigned int)SD_SH_SYRIAC_ALAPH_FINAL_FJ)
    {
      return (SS_UCS4) 0xA0005710;
    }
    if ((unsigned int)currentShape == (unsigned int)SD_SH_SYRIAC_ALAPH_ISOLATED_FN)
    {
      return (SS_UCS4) 0xA0006710;
    }
    if ((unsigned int)currentShape == (unsigned int)SD_SH_SYRIAC_ALAPH_ISOLATED_FX)
    {
      return (SS_UCS4) 0xA0007710;
    }
    SS_UCS4 curr = shared->ucs4v.array()[(unsigned int)currentShape];
    if (curr) return curr;
    /* some fallback. if we are lucky... */
  }
  if ((unsigned int)currentShape == (unsigned int)SD_BENGALI_INITIAL)
  {
    unsigned int en = (SD_BENGALI_BEGIN << 16) |  0x80000000;
    return ( (shared->precomposed & 0xffff) | en);
  }
  return shared->precomposed;
}

/**
 * Get the first character of this composition or decomposition
 */
const SS_UCS4
SGlyph::getFirstChar () const
{
  SS_UCS4 g = getChar();
  if (g==0 && (compSize() > 0 || decompSize() > 0)) 
  {
     g = getDecompArray()[0];
  }
  return g;
}

/**
 * In certain cases - when the font does not contain the 
 * shaped character and we need to fall back to the original
 * characters. Original characters are unshaped - this 
 * routine is making them shaped. 
 * This is hard coded for now.
 * @return null if there are fallback shapes
 */
const SS_UCS4*
SGlyph::getShapeFallback() const
{
  if (!shared->shaped  || fallbackShapes == 0
     || (unsigned int)currentShape > 3) return 0;

  unsigned int dcsize = decompSize(); 
  if (dcsize==0) return 0;

  return fallbackShapes->get ((unsigned int)currentShape, 
        getDecompArray(), dcsize);
}

/*------------------------------------------------------------------------------
 *                    SGlyph Work on decompositions 
 *------------------------------------------------------------------------------
 */

/**
 * Get the precomposed characters _or_ the composition characters
 * add extra composing characters at the end.
 * If precomposed characters we input, return them, if decomposed
 * were input return them. It returns the unicode representation 
 * of the given cluster. All extra composing character are also
 * added.
 */
SV_UCS4
SGlyph::getChars() const
{
  SV_UCS4 ret;
  /* If precomposed char should be used or there are no decomps */
  if ((usePrecomp && getChar() != 0) || decompSize()==0)
  {
    ret.append (getChar());
    /* add extra composing stuff, if any */
    if (shared->composing > 0)
    {
      unsigned int sz = shared->ucs4v.size();
      for (unsigned int i=(unsigned int)shared->composing; i<sz; i++)
      {
        ret.append (shared->ucs4v[i]);
      }
    }
    return SV_UCS4(ret);
  }
  /* TODO: shaping on clusters */
  if (shared->cluster!=0)
  {
    /* just add the composing also which is at the end of the array */
    unsigned int sz = shared->ucs4v.size();
    for (unsigned int i=(unsigned int)shared->cluster; i<sz; i++)
    {
      ret.append (shared->ucs4v[i]);
    }
    return SV_UCS4(ret);
  }
  /* just add the composing also which is at the end of the array */
  unsigned int sz = shared->ucs4v.size();
  unsigned int from = (shared->shaped)?4:0;
  for (unsigned int i=from; i<sz; i++)
  {
    ret.append (shared->ucs4v.array()[i]);
  }
  return SV_UCS4(ret);
}

/**
 * @return the size of the decomposition buffer
 */
unsigned int
SGlyph::decompSize() const
{
  if (shared->shaped)
  {
    /* For shaped we have an offset */
    if ((unsigned int)shared->composing ==0)return shared->ucs4v.size()-4;
    return (unsigned int)shared->composing - 4;
  }
  if (shared->cluster!=0)
  {
    return(unsigned int) shared->cluster;
  }

  if ((unsigned int)shared->composing==0) return shared->ucs4v.size();
  return (unsigned int)shared->composing;
}

/**
 * @return the size of the extra composition buffer
 */
unsigned int
SGlyph::compSize() const
{
  if (shared->composing == 0) return 0;
  return shared->ucs4v.size() - (unsigned int) shared->composing;
}


/**
 * Return the decomposition array of the glyph.
 * If the glyph is a shaped glyph it still returns the
 * correct decomposition - if any.
 */
const SS_UCS4*
SGlyph::getDecompArray() const
{
  if (shared->shaped)
  {
    /* For shaped we have an offset */
    return &shared->ucs4v.array()[4];
  }
  return shared->ucs4v.array();
}

/**
 * Return the omposition array of the glyph.
 * If the glyph is a shaped glyph it still returns the
 * correct composition - if any.
 * The composition has to be applied to the whole rendered cluster.
 */
const SS_UCS4*
SGlyph::getCompArray() const
{
  return &shared->ucs4v.array()[(unsigned int) shared->composing];
}


/**
 * Return the shape array of the glyph.
 * If the glyph is not shaped returned 0
 * the array has a size of foru and it contains the
 * isolated, initial, medial, final forms
 * If a form iz 0 it is not defined.
 */
const SS_UCS4*
SGlyph::getShapeArray() const
{
  if (shared->shaped)
  {
    /* For shaped we have an offset */
    return shared->ucs4v.array();
  }
  return 0;
}

/**
 * return the decomposed character or shaped array, at a certain place.
 */
SS_UCS4
SGlyph::operator[] (unsigned int index) const
{
  if (shared->shaped)
  {
    /* For shaped we have an offset */
    return shared->ucs4v.array()[4 + index];
  }
  return shared->ucs4v.array()[index];
}


/*
 * check if the character has to be mirrored in RTL
 */

bool
SGlyph::isMirrorable() const
{
  return (shared->mirror != 0);
}

/**
 * check if this character is special - lineend, tab LRM RLM
 * These require special rendering.
 */
bool
SGlyph::isSpecial() const
{
  SS_UCS4 fc = getFirstChar();
  return (shared->lineend || shared->tab || fc < 0x20 
     || fc == SD_CD_LRM || fc == SD_CD_RLM || fc==SD_CD_ZWJ || fc==SD_CD_ZWNJ);
  
}

/*
 * Get character type.
 */
SD_CharClass
SGlyph::getType()  const
{
  if (decompCharClass == SD_CC_MAX || usePrecomp) {
    return (SD_CharClass) ((unsigned char)shared->type);
  }
  return decompCharClass;
}

/*
 * Get character type.
 */
SD_BiDiClass
SGlyph::getBiDiType()  const
{
  return (SD_BiDiClass) ((unsigned char)shared->bidi);
}

/**
 * Check if this is a white space.
 */
bool
SGlyph::isWhiteSpace() const
{
  if (isSpecial ()) return true;
  switch (getFirstChar())
  {
  case 0x20:
  case 0x1680:
  case 0x2000:
  case 0x2001:
  case 0x2002:
  case 0x2003:
  case 0x2004:
  case 0x2005:
  case 0x2006:
  case 0x2007:
  case 0x2008:
  case 0x2009:
  case 0x200a:
  // case 0x2028: LINE SEPARATOR 
  case 0x202f:
  case 0x205f:
  case 0x3000:
    return true;
  }
  return false;
}


/**
 * Check if this is a valid number
 * Supports many of the number in unicode, including CJK, arabic, hebrew and greek...
* Addition by Maarten van Gompel <proycon@anaproy.homeip.net>
 */
bool
SGlyph::isNumber() const
{
  SD_CharClass type = getType();
  switch (type)
  {
  case SD_CC_Nd:
  case SD_CC_Nl:
  case SD_CC_No:
    return true;
  default:
    break;
  }
  return false;
}

/**
 * This method is used to highlight letters with a color.
 */
bool
SGlyph::isLetter() const
{
  SD_CharClass type = getType();
  switch (type)
  {
  case SD_CC_Lu:
  case SD_CC_Ll:
  case SD_CC_Lt:
  case SD_CC_Lm:
  case SD_CC_Lo:
    return true;
  default:
    break;
  }
  return false;
}


/**
 * Check if this is a valid delimiter
 * This method is used to facilitate wordwarp functionality.
 *
 * Supports many of the delimiters in unicode, including CJK delimiters, arabic, hebrew and greek...
* Addition by Maarten van Gompel <proycon@anaproy.homeip.net>
 */
bool
SGlyph::isDelimiter() const
{
  if (isSpecial ()) return true;
  SD_CharClass type = getType();
  switch (type)
  {
  case SD_CC_Pc:
  case SD_CC_Pd:
  case SD_CC_Ps:
  case SD_CC_Pe:
  case SD_CC_Pi:
  case SD_CC_Pf:
  case SD_CC_Po:
  case SD_CC_Zs:
  case SD_CC_Zl:
  case SD_CC_Zp:
    return true;
  default:
    break;
  }
  return false;
}

// previously we used isDelimiter to decide weather the word can be broken
// at this point. Now I added CJK and KANA because they can wrap any time.
bool
SGlyph::canWrap() const
{
  if (isDelimiter ()) return true;
  SS_UCS4 chr = shared->precomposed;
  // CJK Symbols and Punctuation
  if (chr >= 0x3000 && chr <=0x303f) return true;
  // Hiragana
  if (chr >= 0x3040 && chr <=0x309f) return true;
  // Katakana
  if (chr >= 0x30A0 && chr <=0x30ff) return true;
  // Bopomofo
  // Hangul Compatibility Jamo
  // CJK Strokes 
  // Katakana Phonetic Extensions
  // Enclosed CJK Letters and Months
  // CJK Compatibility
  // CJK Unified Ideographs Extension A
  // Yijing Hexagram Symbols
  // CJK Unified Ideographs
  if (chr >= 0x3100 && chr <=0x9fff) return true;
  // Hangul Syllables  	
  if (chr >= 0xac00 && chr <=0xd7af) return true;
  // Japanese fullwidth alphabet
  if (chr >= 0xff21 && chr <=0xff5a) return true;
  // Halfwidth and Fullwidth Forms
  if (chr >= 0xff00 && chr <=0xffef) return true;
  // CJK Compatibility Ideographs
  if (chr >= 0xf900 && chr <=0xfaff) return true;
  // CJK Unified Ideographs Extension B
  if (chr >= 0x20000 && chr <=0x2A6DF) return true;
  // CJK Compatibility Ideographs Supplement  	
  if (chr >= 0x2F800 && chr <=0x2FA1F) return true;
  return false; 
}


/**
 * Check if this is a transparent character.
 */
bool
SGlyph::isTransparent() const
{
  // From Miikka-Markus Alhonen:
  // "T = Mn + Cf - ZWNJ - ZWJ" (ArabicShaping-4.txt of UCD 3.1.1)
  // This means that every character belonging to character classes 
  // Mn (04) or Cf (0E) except for ZWNJ U+200C and ZWJ U+200D - and 
  // nothing else - is transparent.
  // So, even all the characters in Combining Diacritical Marks 
  // U+0300 - U+036F, Hebrew vowels U+0591 - U+05BD, Syriac vowels 
  // U+0730 - U+074A etc. are transparent characters, not just the 
  // Arabic tashkeel
  //return (gcategory.encode (u4) == 0x4 && ...); // Mark, Non-Spacing
  if (shared->type == (char)SD_CC_Mn || (char)shared->type == SD_CC_Me 
     || shared->type == (char)SD_CC_Cf)
  {
     SS_UCS4 u4 = getFirstChar();
     return  (u4 != SD_CD_ZWNJ && u4 != SD_CD_ZWJ);
  }
  return false;
}

/**
 * return true if this is the end of a paragraph
 */
bool
SGlyph::isEOP() const
{
  return (shared->lineend 
    && shared->precomposed != SD_CD_LS
    && shared->precomposed != SD_CD_FF);
}

/**
 * return true if this is the end of a paragraph.
 */
bool
SGlyph::isEOL() const
{
  return (shared->precomposed == SD_CD_LS || shared->precomposed == SD_CD_FF);
}

bool
SGlyph::isFF() const
{
  return (shared->precomposed == SD_CD_FF);
}

bool
SGlyph::isTab() const
{
  return shared->tab;
}

bool
SGlyph::isCluster() const
{
  return shared->cluster != 0;
}

/**
 * We use private area to define our own ligatures.
 */
bool
SGlyph::isYuditLigature() const
{
  if (decompSize()==0) return 0;
  /* We might above ligature for shaping */
  SS_UCS4 ch = getShapedChar();
  return isLigature (ch);
}

/**
 * We use private area to define our own precomposed characters.
 */
bool
SGlyph::isYuditComposition() const
{
  if (decompSize()==0) return 0;
  /* We might above ligature for shaping */
  SS_UCS4 ch = getShapedChar();
  if (ch>= 0xA0000000)
  {
    return true;
  }
  return false;
}

/**
 * Construct a key from characters inside the glyph
 * @return the key. the key does not contain attributes, 
 * it only has glyph info.
 */
SString
SGlyph::charKey() const
{
 
  if (decompSize()+compSize() == 0)
  {
    SS_UCS4 chr = getChar();
    return SString ((char*) &chr, sizeof (SS_UCS4));
  }
  return SString ((char*) shared->ucs4v.array(), 
         shared->ucs4v.size() * sizeof (SS_UCS4));
}

/*------------------------------------------------------------------------------
 *                    comparison
 *------------------------------------------------------------------------------
 */
bool
SGlyph::operator == (const SGlyph& g2) const
{
  if (shared->ucs4v.size() != g2.shared->ucs4v.size())
  {
    return false;
  }
  if (shared->ucs4v.size()==0)
  {
    return (getChar() == g2.getChar());
  }
  for (unsigned int i=0; i<shared->ucs4v.size(); i++)
  {
     if (shared->ucs4v[i] != g2.shared->ucs4v[i]) return false;
  }
  return true;
}

bool
SGlyph::operator != (const SGlyph& g2) const
{
  if (shared->ucs4v.size() != g2.shared->ucs4v.size())
  {
    return true;
  }
  if (shared->ucs4v.size()==0)
  {
    return (getChar() != g2.getChar());
  }
  for (unsigned int i=0; i<shared->ucs4v.size(); i++)
  {
     if (shared->ucs4v[i] != g2.shared->ucs4v[i]) return true;
  }
  return false;
}

/**
 * Cache fallback shapes into 
 * fbIsolated, fbInitial, fbMedial, fbFinal global hashTables.
 * @param shapes tells us what shapes will be present.
 *   this is an array of size 4, (isolated, initial, medial, final)
 * @param chars tells us the characters we need shapes for.
 * @param size is the size of chars array.
 */
void
addFallbackShapes (SUniMap* shaper, const SS_UCS4* shapes,
  const SS_UCS4* chars, unsigned int csize)
{
  if (csize ==0) return; /* robustness */
  /* first build shape arrays */
  SV_UCS4 isol;
  SV_UCS4 init;
  SV_UCS4 medi;
  SV_UCS4 fina;

  if (fallbackShapes == 0)
  {
    fallbackShapes = new SFallbackShapes();
    CHECK_NEW (fallbackShapes);
  }
  /* isolated will always have it */
  if (fallbackShapes->get (0, chars, csize))
  {
     return;
  }

  unsigned int i;
  for (i=0; i<csize; i++)
  {
    SS_UCS2 u2=getCharClass(chars[i]);
    /* combining ones are substituted with SD_CD_ZWJ */ 
    if (u2 == SD_CC_Mn || u2 == SD_CC_Me)
    {
      isol.append (SD_CD_ZWJ);
      init.append (SD_CD_ZWJ);
      medi.append (SD_CD_ZWJ);
      fina.append (SD_CD_ZWJ);
      continue;
    }
    SV_UCS4 v; v.append (chars[i]);
    SV_UCS4 ret;
    unsigned int n = shaper->lift (v, 0, false, &ret);
    /* the composition comes at the end  - if any */
    if (n==1 && ret.size()==4)
    {
      isol.append (ret[0]);
      init.append (ret[1]);
      medi.append (ret[2]);
      fina.append (ret[3]);
    }
    /* make it transparent: deal with SD_CD_ZWJ for now. */
    else if (chars[i]== SD_CD_ZWJ || chars[i]== SD_CD_ZWNJ)
    {
      isol.append (chars[i]);
      init.append (chars[i]);
      medi.append (chars[i]);
      fina.append (chars[i]);
    }
    else /* no shapes - treat as isol */
    {
      isol.append (chars[i]);
      init.append (0);
      medi.append (0);
      fina.append (0);
    }
  }
  /* now collect shape arrays */
  SV_UCS4 isolA;
  SV_UCS4 initA;
  SV_UCS4 mediA;
  SV_UCS4 finaA;

  for (i=0; i<csize; i++)
  {

    SS_UCS4 now[4];
    now[0] = isol[i]; now[1] = init[i]; now[2] = medi[i]; now[3] = fina[i];

    SS_UCS4 prev[4];
    SS_UCS4 next[4];

    /* build 4 shapes */
    for (unsigned int j=0; j<4; j++)
    {
      /* we always do isolated so that we can check it it was processed */
      if (j!=0 && shapes[j] ==0) continue;

      int i0 = ((int)i) - 1;

      /* skip ZWJ and ZWNJ */
      while (i0>=0 && (chars[(unsigned int)i0] == SD_CD_ZWJ 
           || chars[(unsigned int)i0] == SD_CD_ZWNJ)) i0--;

      /* Emulate previous */
      if (i0>=0)
      {
        prev[0] = isol[(unsigned int)i0]; prev[1] = init[(unsigned int)i0];
        prev[2] = medi[(unsigned int)i0]; prev[3] = fina[(unsigned int)i0];
      }
      else if (j==(unsigned int)SD_SH_MEDIAL 
             || j==(unsigned int)SD_SH_FINAL)
      {
       /* Previous had initial or medial */
        prev[0] = 0; prev[1] = 1; prev[2] = 1; prev[3] = 0;
      }
      else /* ISOLATED INITIAL */
      {
        /* Previous has isolated or final */
        prev[0] = 1; prev[1] = 0; prev[2] = 0; prev[3] = 1; 
      }

      /* there is next */
      unsigned int i2 = i+1;
      /* skip ZWJ and ZWNJ */
      while (i2<csize && (chars[i2] == SD_CD_ZWJ || chars[i2] == SD_CD_ZWNJ)) i2++;
      if (i2<csize)
      {
        next[0] = isol[i2]; next[1] = init[i2];
        next[2] = medi[i2]; next[3] = fina[i2];
      }
      else if (j==(unsigned int)SD_SH_ISOLATED 
             ||j==(unsigned int)SD_SH_FINAL)
      {
        /* Next has isolated or initial */
        next[0] = 1; next[1] = 1; next[2] = 0; next[3] = 0; 
      }
      else /* MEDIAL INITIAL */
      {
        /* Next has medial or final */
        next[0] = 0; next[1] = 0; next[2] = 1; next[3] = 1; 
      }
      /* get the shape for this character */
      char sh = getSimpleShape (now, prev, next);
      
      SS_UCS4 schar = ((unsigned int)sh < 4) 
          ? now[(unsigned int)sh] : 0;
      if (schar == 0)
      {
        //fprintf (stderr, "No shape %d for %X\n", (unsigned int)sh, chars[i]);
        schar = chars[i];
      }
      switch ((int)j)
      {
      case (unsigned int)SD_SH_ISOLATED:
        isolA.append (schar);
        break;
      case (unsigned int)SD_SH_INITIAL:
        initA.append (schar);
        break;
      case (unsigned int)SD_SH_MEDIAL:
        mediA.append (schar);
        break;
      case (unsigned int)SD_SH_FINAL:
        finaA.append (schar);
        break;
      }
    }
  }
  /* set cache */
  if (isolA.size()==csize) fallbackShapes->put (0, chars, isolA.array(), csize);
  if (initA.size()==csize) fallbackShapes->put (1, chars, initA.array(), csize);
  if (mediA.size()==csize) fallbackShapes->put (2, chars, mediA.array(), csize);
  if (finaA.size()==csize) fallbackShapes->put (3, chars, finaA.array(), csize);
}

/**
 * calculate the current shape 
 * return true if shape changed
 */
bool
SGlyph::setShape(const SGlyph* gbefore, const SGlyph* gafter)
{
  if (!shared->shaped &&
    getLigatureScriptCode (shared->precomposed)!=SD_BENGALI)
  {
    return false;
  }
  char shape = getShape (gbefore, gafter);
  if (shape == currentShape)  return false;
  currentShape = shape;
  return true;
}

/**
 * Get the shape at the current position
 * Please note that it works in visual order!
 * @return 
 * <ul> 
 *  <li> SD_NOSHAPE  no shape </li>
 *  <li> 0   isolated </li>
 *  <li> 1   initial (space after-rl)</li>
 *  <li> 2   medial </li>
 *  <li> 3   final (space before-rl)</li>
 * </ul>
 * @param gbefore is the glyph before this line, transparent chars skipped
 * @param gafter is the glyph before this line, transparent chars skipped
 */
char
SGlyph::getShape(const SGlyph* gbefore, const SGlyph* gafter)
{
  static SS_UCS4 initials[4] = {0x0, 0x0, 0x0, 0x0};
  static SS_UCS4 dualjoining[4] = {1, 1, 1, 1};

  /* is it a shapeable one ? */
  const SS_UCS4* now = getShapeArray();
  if (now == 0)
  {
    if (getLigatureScriptCode (shared->precomposed)==SD_BENGALI)
    {
      if (gbefore==0 ||
          (getLigatureScriptCode (gbefore->shared->precomposed)!=SD_BENGALI 
           && getUnicodeScript  (gbefore->getFirstChar()) != SD_BENGALI))
      {
        return (char) SD_BENGALI_INITIAL;
      }
    }
    return (char) SD_SH_NOSHAPE;
  }
  const SS_UCS4* before = initials;
  const SS_UCS4* after = initials;
  if (gbefore) 
  {
    before=gbefore->getShapeArray();
    if (before==0) before=initials;
    /* tatweel and ZWJ are dual joining */
    if (gbefore->getFirstChar() == SD_CD_ARABIC_TATWEEL ||
	gbefore->getFirstChar() == SD_CD_ZWJ) before = dualjoining;
  }
  if (gafter) 
  {
    after=gafter->getShapeArray();
    if (after==0) after=initials;
    /* tatweel and ZWJ are dual joining */
    if (gafter->getFirstChar() == SD_CD_ARABIC_TATWEEL ||
	gafter->getFirstChar() == SD_CD_ZWJ) after = dualjoining;
  }

  /* Make it all rl for simplicity */
  SS_UCS4 beforeChar = (gbefore==0) ? 0 : gbefore->getFirstChar();
  SS_UCS4 afterChar = (gafter==0) ? 0 : gafter->getFirstChar();

  bool syriacEOWAlaph = false;
  syriacEOWAlaph  = (afterChar < 0x070f ||  afterChar  > 0x074f) &&
      (afterChar < 0x0621 || afterChar > 0x065f) && afterChar != SD_CD_ZWJ;
  syriacEOWAlaph = syriacEOWAlaph && (getFirstChar() == 0x0710);

  /* This is not End of word. Isolated. */
  if (syriacEOWAlaph &&  (beforeChar <  0x070f || beforeChar > 0x074f) &&
       beforeChar != SD_CD_ARABIC_TATWEEL && beforeChar != SD_CD_ZWJ)
  {
    return (char) SD_SH_ISOLATED;
  }
  /* End-Of-Word rules for Syriac Alaph */
  if (syriacEOWAlaph)
  {
    /* FX - Isolated end of the word when preceded 
      by Syriac dalath or rish: U+0715 U+0716 U+072A */
    if (beforeChar == SD_CD_SYRIAC_LETTER_DALATH 
        || beforeChar == SD_CD_SYRIAC_LETTER_DOTLESS_DALATH 
        || beforeChar == SD_CD_SYRIAC_LETTER_RISH)
    {
      return (char) SD_SH_SYRIAC_ALAPH_ISOLATED_FX;
    }
    /* FJ - Final end of word  */
    if (before[(unsigned int)SD_SH_INITIAL])
    {
      return (char) SD_SH_SYRIAC_ALAPH_FINAL_FJ;
    }
    /* FN - Isolated end of word  
      except when preceded by Syriac dalath or rish */ 
    return (char) SD_SH_SYRIAC_ALAPH_ISOLATED_FN;
  }
  /* call shaper */
  return getSimpleShape (now, before, after);
}

/**
 * This simple shaper is used after 
 * exceptions are applied, and when a fallback shape
 * is generated 
 * @param now is the current shape array
 * @param before is the shape array of the previous character
 * @param after is the shape array of the next character
 * before and after is in logical order.
 * The shape array contains 4 elements for
 *  isolated, initial, medial and final forms.
 */
static char
getSimpleShape (const SS_UCS4* now, 
   const SS_UCS4* before, const SS_UCS4* after)
{

  if ((before[(unsigned int)SD_SH_INITIAL])
     && now[(unsigned int)SD_SH_MEDIAL]
     && (after[(unsigned int)SD_SH_FINAL]))
  {
    return (char) SD_SH_MEDIAL;
  }
  if (after[(unsigned int)SD_SH_FINAL]
    && now[(unsigned int)SD_SH_INITIAL])
  {
    return (char) SD_SH_INITIAL;
  }
  if (before[(unsigned int)SD_SH_INITIAL]
     && now[(unsigned int)SD_SH_FINAL])
  {
    return (char) SD_SH_FINAL;
  }
  if (now[(unsigned int)SD_SH_ISOLATED])
  {
    return (char) SD_SH_ISOLATED;
  }
  /* fallback */
  return (char) SD_SH_NOSHAPE;
}


SGlyphShared*
getGlyphShared (SS_UCS4 c)
{
  return glyphCache.get (SString((char*)&c, sizeof (SS_UCS4)));
}

