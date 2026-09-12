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
#include "stoolkit/SCluster.h"
#include "stoolkit/SProperties.h"
#include "stoolkit/SUniMap.h"


static unsigned int 
getRISCluster (const SV_UCS4& unicode, unsigned int index, 
  SV_UCS4* ret, int* finished);

static unsigned int 
getRovasCluster (const SV_UCS4& unicode, unsigned int index, 
  SV_UCS4* ret, int* finished, bool isPUA);

static unsigned int 
getJamoCluster (const SV_UCS4& unicode, unsigned int index, 
  SV_UCS4* ret, int* finished);

static SS_UCS4 precomposJamos(SV_UCS4* jamo);

static unsigned int 
getSouthIndicCluster (unsigned int scriptcode, 
  const SV_UCS4& unicode, 
  unsigned int index, SV_UCS4* ret, int* finished);

static unsigned int 
getIndicCluster (unsigned int scriptcode, 
  const SV_UCS4& unicode, 
  unsigned int index, SV_UCS4* ret, int* finished);

static SUniMap* clusters;
static SUniMap* indic;
static SProperties*  ligatureUnics;
static SProperties*  ligatureClust;

static SProperties*  ligatureCache;
static SS_UCS4  counters[SD_SCRIPT_MAX];

static SS_UCS4 nextLigature (unsigned int script, 
   const SS_UCS4* unicode, unsigned int length);

static void initLigatures();

SString yuditClusterError;

/**
 * Try to form a cluster - an abstract glyphs that can 
 * be broken apart once made. It can be rendered by 
 * a font that contains glyphs and ligatureUnics by subdividing
 * the cluster. The cluster is in memory order - 
 * vowels are place on the appropriate side.
 *
 * Clusters will replace the current SGlyph architecture.
 * All new things should be added here.
 *
 * 2002-04-03 - added surrogate clusters. 
 *
 * @param ucs4 is the input vector.
 * @param i is the index in this vector - next character.
 * @param finished is set to 0 if more data is needed
 *  this parameter can be null.
 * @return the new index in ucs4.
 */
unsigned int
getCluster (const SV_UCS4& ucs4, 
   unsigned int index, SV_UCS4* retchar, int *finished)
{
  if (finished) *finished = -1;

  /* pack surrogates into a cluster 
     - no combining marks on surrogates for the time being. */
  if (ucs4[index] >= 0xd800 && ucs4[index] <= 0xdbff)
  {
    if (ucs4.size() < index+2)
    {
       if (finished) *finished = 0;
       retchar->append (ucs4[index]);
       return index + 1;
    }
    if (ucs4[index+1] >= 0xdc00 && ucs4[index+1] <= 0xdfff)
    {
       retchar->append (((ucs4[index] & 0x3ff)<< 10) 
           + (ucs4[index+1]&0x3ff) + 0x10000);
       if (finished) *finished = 1;
       return index+2;
    }
    return index;
  }
  /* start the game */
  initLigatures();

  /* Should be able to start with ZWJ */
  int scriptcode = (
      (ucs4[index] == 0x200D || ucs4[index] == 0x25CC) && index+1 < ucs4.size())
      ?  getUnicodeScript (ucs4[index+1]) : getUnicodeScript (ucs4[index]);

  if (scriptcode < 0) return index;
  unsigned int ret = index;
  yuditClusterError.clear();
  switch (scriptcode)
  {
  case SD_DEVANAGARI: 
  case SD_BENGALI: 
  case SD_GURMUKHI: 
  case SD_GUJARATI: 
  case SD_ORIYA: 
  case SD_KANNADA:
  case SD_MALAYALAM:
  case SD_SINHALA:
  case SD_TELUGU:
    if (!indic->isOK()) break;;
    ret = getIndicCluster (
         (unsigned int)scriptcode, ucs4, index, retchar, finished);
    break;
  case SD_HANGUL_JAMO:
    ret = getJamoCluster (ucs4, index, retchar, finished);
    break;
  case SD_TIBETAN:
  case SD_THAI:
  case SD_LAO:
    ret = getSouthIndicCluster ((unsigned int)scriptcode, 
          ucs4, index, retchar, finished);
    //if (ret>0) fprintf (stderr, "TIBET Tibetan: %d\n", ret-index);
    break;
  case SD_TAMIL:
  case SD_YUDIT:
    if (!clusters->isOK()) break;
    ret = clusters->lift (ucs4, index, true, retchar);
    break;
  case SD_ROVASIRAS:
    ret = getRovasCluster (ucs4, index, retchar, finished, false);
    break;
  case SD_PUA_ROVAS:
    ret = getRovasCluster (ucs4, index, retchar, finished, true);
    break;
  case SD_REGIONAL_INDICATOR_SYMBOL:
    ret = getRISCluster (ucs4, index, retchar, finished);
    break;
  }
  if (finished==0 && yuditClusterError.size())
  {
    // If you want to debug things uncomment this.
    //fprintf (stderr, "SCluster.cpp:%*.*s\n", SSARGS(yuditClusterError));
  }
  return ret;
}

/** 
 * -1 non rovas.
 * 1 rovas basic
 * 2 rovas liga
 * 3 rovas yudit cluster
 * 0 ZWJ
 */
int getRovasType (SS_UCS4 chr) 
{
    if (chr == 0x200d) return 0;
    if (chr >= 0x10c80 && chr <= 0x10cff) 
    {
        return 1;
    }
    if (getLigatureScriptCode(chr) == SD_ROVASIRAS) return 3;
    return -1;
}

/** 
 * -1 non rovas.
 * 1 rovas basic
 * 2 rovas liga
 * 3 rovas yudit cluster
 * 0 ZWJ
 */
int getPUARovasType (SS_UCS4 chr) 
{
    if (chr == 0x200d) return 0;
    // capital
    if (chr >= 0xee00 && chr <= 0xee29) 
    {
        return 1;
    }
    // capital
    if (chr >= 0xee30 && chr <= 0xee8b) 
    {
        return 2;
    }
    // small 
    if (chr >= 0xeeb0 && chr <= 0xeed9) 
    {
        return 1;
    }
    // small 
    if (chr >= 0xeee0 && chr <= 0xef3b) 
    {
        return 2;
    }
    if (getLigatureScriptCode(chr) == SD_PUA_ROVAS) return 3;
    return -1;
}

static unsigned int 
getRovasCluster (const SV_UCS4& unicode, unsigned int index, 
  SV_UCS4* ret, int* finished, bool isPUA) {

  unsigned int usize = unicode.size();
  if (index>=usize) return index;

  /* set it to finished - this routine would not be called to other scripts  */
  if (finished) *finished = 1;
  /* Some platforms have unsigned char */
  int prevchartype = 0;

  SS_UCS4 nextLig = 0;
  unsigned int i;
  int ligatureType = isPUA ? SD_PUA_ROVAS : SD_ROVASIRAS;
  for (i=index;i<usize; i++)
  { 
    SS_UCS4 next = unicode[i];
    int chartype = isPUA ? getPUARovasType (next) : getRovasType (next);
    switch (chartype)
    {
    case 0:
      if (prevchartype < 1)
      {
        if (i > index+1) 
        {
          nextLig = nextLigature (ligatureType,
              &unicode.array()[index], i-index);
          if (nextLig) ret->append (nextLig);
          return i; 
        }
        ret->clear();
        return index;
      }
      ret->append (next);
      break;
    case 1:
    case 2:
      if (prevchartype != 0)
      {
        if (i > index+1) 
        {
          nextLig = nextLigature (ligatureType,
                &unicode.array()[index], i-index);
          if (nextLig) ret->append (nextLig);
          return i; 
        }
        ret->clear();
        return index;
      }
      ret->append (next);
      break;
    case -1:
      if (i > index+1) 
      {
        nextLig = nextLigature (ligatureType,
            &unicode.array()[index], i-index);
        if (nextLig) ret->append (nextLig);
        return i; 
      }
      ret->clear();
      return index;
    }
    prevchartype = chartype;
  }
  /* Not yet finished. Return unfinished cluster */
  if (finished) *finished = 0;
  if (ret->size() > 1)
  {
     nextLig = nextLigature (ligatureType, 
          &unicode.array()[index], i-index);
     if (nextLig) ret->append (nextLig);
     return i;
  }
  ret->clear();
  return index;
}

static unsigned int 
getRISCluster (const SV_UCS4& unicode, unsigned int index, 
  SV_UCS4* ret, int* finished) {

  unsigned int usize = unicode.size();
  if (index>=usize) return index;

  /* set it to finished - this routine would not be called to other scripts  */
  if (finished) *finished = 1;

  SS_UCS4 nextLig = 0;
  unsigned int i;
  int ligatureType = SD_REGIONAL_INDICATOR_SYMBOL;
  for (i=index;i<index+2 && i<usize; i++)
  { 
    SS_UCS4 next = unicode[i];
    if (getUnicodeScript(next) != SD_REGIONAL_INDICATOR_SYMBOL) {
        if (finished) *finished = 0;
        break;
    }
    ret->append (next - 0x1f1e6 + (int) 'A');
  }
  if (ret->size() > 1) {
     nextLig = nextLigature (ligatureType, 
          &unicode.array()[index], i-index);
     if (nextLig) ret->append (nextLig);
     return i;
  }
  if (finished) *finished = 1;
  ret->clear();
  return index;
}

/**
 * Create a JAMO Cluster as of Unicode 3.0 Chapter 3.11.
 * 1. L.X V.X T.X X.L X.V X.T 
 * 2. T.L 
 * 3. V.L 
 * 4. T.V 
 * In short:  Cluster=L*V*T*
 * Asterisk means: one or more.
 * @param finished is set to 1 if exact match happens
 *                           0 is not yet finished
 *                          -1 if illegal sequence start.
 */
static unsigned int 
getJamoCluster (const SV_UCS4& unicode, unsigned int index, 
  SV_UCS4* ret, int* finished)
{

  unsigned int usize = unicode.size();
  if (index>=usize) return index;


  /* set it to finished - this routine would not be called to other scripts  */
  if (finished) *finished = -1;
  /* Some platforms have unsigned char */
  int prevchartype = getJamoClass (unicode[index]);

  SS_UCS4 nextLig = 0;
  unsigned int i;
  for (i=index;i<usize; i++)
  { 
    SS_UCS4 next = unicode[i];
    int chartype = getJamoClass (next);
    switch (chartype)
    {
    case SD_JAMO_L:
      if (prevchartype != SD_JAMO_L)
      {
        nextLig = precomposJamos (ret);
        if (nextLig==0)
        {
          nextLig = nextLigature (SD_HANGUL_JAMO,
            &unicode.array()[index], i-index);
        }
        if (nextLig) ret->append (nextLig);
        return i; 
      }
      ret->append (next);
      break;
    case SD_JAMO_V:
      if (prevchartype != SD_JAMO_L && prevchartype != SD_JAMO_V)
      {
        nextLig = precomposJamos (ret);
        if (nextLig ==0)
        {
          nextLig = nextLigature (SD_HANGUL_JAMO,
              &unicode.array()[index], i-index);
        }
        if (nextLig) ret->append (nextLig);
        return i; 
      }
      ret->append (next);
      break;
    case SD_JAMO_T:
      /* Do we really have TT sequence ? According to Unicode yes. Hmm.. */
      if (prevchartype != SD_JAMO_V && prevchartype != SD_JAMO_T)
      {
        nextLig = precomposJamos (ret);
        if (nextLig==0)
        {
           nextLig = nextLigature (SD_HANGUL_JAMO,
            &unicode.array()[index], i-index);
        }
        if (nextLig) ret->append (nextLig);
        return i; 
      }
      ret->append (next);
      break;
    case SD_JAMO_X:
    default:
      /* Tone marks can follow the cluster */
// They are supported as composing anyway...
#if 0
      if (next == 0x302e || next == 0x302f)
      {
        ret->append (next);
        i++;
      }
#endif
      nextLig = precomposJamos (ret);
      if (nextLig==0)
      {
        nextLig = nextLigature (SD_HANGUL_JAMO,
          &unicode.array()[index], i-index);
      }
      if (nextLig) ret->append (nextLig);
      return i; 
      break;
    }
    prevchartype = chartype;
  }
  /* Not yet finished. Return unfinished cluster */
  if (finished) *finished = 0;
  if (ret->size()>=1)
  {
     nextLig = precomposJamos (ret);
     if (nextLig==0)
     {
       nextLig = nextLigature (SD_HANGUL_JAMO, 
          &unicode.array()[index], i-index);
     }
     if (nextLig) ret->append (nextLig);
     return i;
  }
  ret->clear();
  return index;
}

/**
 * Precompose JAMOs that are present in unicode tables
 * @param jamo is the vector that holds input jamos and 
 *  output precompositions.
 * @return the precomposed JAMOS or 0
 */
static SS_UCS4
precomposJamos(SV_UCS4* jamo)
{
  if (jamo->size()==0) return 0;
  if (jamo->size()==1) return 0;
  SS_UCS4 last = (*jamo)[jamo->size()-1];
  if (last==0x302e || last==0x302f)
  {
    if (jamo->size()<=2) return 0;
    if (jamo->size()>4) return 0;
    jamo->truncate (jamo->size()-1);
  }
  else if (jamo->size()>3)
  {
    return 0;
  }

  SS_UCS4 l = (*jamo)[0];
  SS_UCS4 v = (*jamo)[1];
  SS_UCS4 t = (jamo->size() >= 3) ? (*jamo)[2] : 0x11a7;
  /* tone marks will be rendered first */
  if (last==0x302e || last==0x302f)
  {
    jamo->insert (0, last);
  }
  if (l>=0x1100 && l<=0x1112 
   && v>=0x1161 && v<=0x1175 
   && t>=0x11a7 && t<=0x11c2)
  {
    jamo->clear();
    SS_UCS4 vle = 21*28* (l-0x1100) + 28 * (v-0x1161) + (t-0x11a7) + 0xac00;
    jamo->append (vle);
    /* create a unique key */
    if (last==0x302e)
    {
      vle = vle & 0x3fff;
    }
    else if (last==0x302f)
    {
      vle = vle & 0x7fff;
    }
    vle +=  0x80000000 + (0x10000 * SD_HANGUL_PREC);
    return vle;
  }
  return 0;
}

/**
 * Get cluster for South Indian Thai-like scripts
 * The cluster is rendered and treated together. It has
 * a unicode and a separated memory representation.
 * Memory representation is only used for fallback rendering.
 * A cluster is
 *
 * a) Consonant + Top/Bottom/Right Sign [+ ...]
 * b) Consonant + Nukta
 * c) Consonant + Nukta + Top/Bottom/Right Sign [+ ...]
 * d) Indep-Vowel + Top/Bottom Sign [+ ...]
 *
 * @param finished is set to 1 if exact match happens
 *                           0 is not yet finished
 *                          -1 if illegal sequence start.
 * It also sets yuditClusterError to an appropriate string.
 */
static unsigned int 
getSouthIndicCluster (unsigned int scriptcode, 
  const SV_UCS4& unicode, unsigned int index, SV_UCS4* ret, int* finished)
{
  unsigned int usize = unicode.size();
  unsigned int i;
  if (finished) *finished = 1;
  /* Some platforms have unsigned char */

  char prevchartype = (char)0x7f; /* big enough */
  SS_UCS4 nextLig = 0;
  for (i=index;i<usize; i++)
  { 
    SS_UCS4 next = unicode[i];
    char chartype = (char) indic->encode (next);
    unsigned int sc = getUnicodeScript (next);
    if (sc!=scriptcode && next != 0x25cc && next != 0x200d && next != 0x200c)
    {
      if (ret->size()==0)
      {
        /* can not start with it */
        if (finished) *finished=-1;
      }
      nextLig = nextLigature (scriptcode, &unicode.array()[index], i-index);
      if (nextLig) ret->append (nextLig);
      return i; 
    }
    switch (chartype)
    {
    case SD_INDIC_INDEP_VOWEL:
      ret->append (next);
      if (i+1 < usize)
      { 
        SS_UCS4 n = unicode[i+1];
        char ct = (char) indic->encode (n);
        if (ct != SD_INDIC_TOP_VOWEL && ct != SD_INDIC_BOTTOM_VOWEL)
        {
           if (ret->size()==1) return index;
           nextLig = nextLigature (scriptcode, 
                &unicode.array()[index], i-index+1);
           if (nextLig) ret->append (nextLig);
           return i+1;
        }
      }
      break;
    case SD_INDIC_CONSONANT_BASE:
    case SD_INDIC_CONSONANT_POST_BASE:
    case SD_INDIC_CONSONANT_BELOW_BASE:
      ret->append (next);
      if (i+1 < usize)
      { 
        SS_UCS4 n = unicode[i+1];
        char ct = (char) indic->encode (n);
        if (ct != SD_INDIC_NUKTA 
             && ct != SD_INDIC_RIGHT_VOWEL
             && ct != SD_INDIC_TOP_VOWEL
             && ct != SD_INDIC_BOTTOM_VOWEL)
        {
           if (ret->size()==1) return index;
           nextLig = nextLigature (scriptcode, 
                &unicode.array()[index], i-index+1);
           if (nextLig) ret->append (nextLig);
           return i+1;
        }
      }
      break;
    case SD_INDIC_NUKTA:
      if (ret->size()==0)
      {
        /* can not start with it */
        if (finished) *finished=-1;
        yuditClusterError = "Cluster should not start with a subjoined consonant.";
        return index; 
      }
      if (prevchartype != SD_INDIC_CONSONANT_BASE
         && prevchartype != SD_INDIC_CONSONANT_POST_BASE
         && prevchartype != SD_INDIC_CONSONANT_BELOW_BASE)
      {
        yuditClusterError = "Subjoined consonant should be preceded by a full consonant.";
        nextLig = nextLigature (scriptcode, &unicode.array()[index], i-index);
        if (nextLig) ret->append (nextLig);
        return i;
      }
      ret->append (next);
      if (i+1 < usize)
      { 
        SS_UCS4 n = unicode[i+1];
        char ct = (char) indic->encode (n);
        if (ct != SD_INDIC_RIGHT_VOWEL
             && ct != SD_INDIC_TOP_VOWEL
             && ct != SD_INDIC_BOTTOM_VOWEL)
        {
           if (ret->size()==1) return index;
           nextLig = nextLigature (scriptcode, 
                &unicode.array()[index], i-index+1);
           if (nextLig) ret->append (nextLig);
           return i+1;
        }
      }
      break;
    case SD_INDIC_LEFT_VOWEL:
    case SD_INDIC_RIGHT_VOWEL:
    case SD_INDIC_TOP_VOWEL:
    case SD_INDIC_BOTTOM_VOWEL:
      if (ret->size()==0)
      {
        /* can not start with it */
        if (finished) *finished=-1;
        yuditClusterError = "Cluster should not start with a dependent wovel.";
        return index; 
      }
      if (prevchartype != SD_INDIC_INDEP_VOWEL
	   && prevchartype != SD_INDIC_CONSONANT_BASE
	   && prevchartype != SD_INDIC_CONSONANT_BELOW_BASE
	   && prevchartype != SD_INDIC_CONSONANT_POST_BASE
	   && prevchartype != SD_INDIC_NUKTA
	   && prevchartype != SD_INDIC_RIGHT_VOWEL
	   && prevchartype != SD_INDIC_TOP_VOWEL
	   && prevchartype != SD_INDIC_BOTTOM_VOWEL)
      {
        yuditClusterError = "Dependent sign should be preceded by another character.";
        nextLig = nextLigature (scriptcode, &unicode.array()[index], i-index);
        if (nextLig) ret->append (nextLig);
        return i;
      }
      ret->append (next);
      if (i+1 < usize)
      { 
        SS_UCS4 n = unicode[i+1];
        char ct = (char) indic->encode (n);
        if (ct != SD_INDIC_RIGHT_VOWEL
	     && ct != SD_INDIC_TOP_VOWEL
	     && ct != SD_INDIC_BOTTOM_VOWEL)
        {
           if (ret->size()==1) return index;
           nextLig = nextLigature (scriptcode, 
                &unicode.array()[index], i-index+1);
           if (nextLig) ret->append (nextLig);
           return i+1;
        }
      }
      break;
    case SD_INDIC_SIGN:
      if (ret->size()==0)
      {
        /* can start with it */
        // if (finished) *finished=-1;
        return index; 
      }
      ret->append (next);
      nextLig = nextLigature (scriptcode, &unicode.array()[index], i-index+1);
      if (nextLig) ret->append (nextLig);
      return i+1;
    default:
      if (ret->size()==0)
      {
        if (finished) *finished=1;
        return index; 
      }
      nextLig = nextLigature (scriptcode, &unicode.array()[index], i-index);
      if (nextLig) ret->append (nextLig);
      return i;
    }
    prevchartype = chartype;
  }
// fprintf (stderr, "TIBET index=%d\n", index);
  if (finished) *finished = 0;
  if (ret->size()>1)
  {
     nextLig = nextLigature (scriptcode, &unicode.array()[index], i-index);
     if (nextLig) ret->append (nextLig);
     return i;
  }
  ret->clear();
  return index;
}

/**
 * Get cluster for North Indian Devanagari-like scripts
 * The cluster is rendered and treated together. It has
 * a unicode and a separated memory representation.
 * Memory representation is only used for fallback rendering.
 * A cluster is
 * a) Consonant
 * b) Consonant + Halant
 * c) Consonant + Halant + ZWJ
 * d) Consonant + Nukta + Halant
 * e) Consonant + Nukta + Halant + ZWJ
 * f) Independent Vowel
 * g) Independent Vowel + Vowel
 * h) [b|c|d|e]*
 * i) [b|c|d|e]* a
 * j) [b|c|d|e]* Vowel
 * k) [a-i] ending with Modifier
 * l) [a-i] ending with ZWNJ
 * For bengali
 * Consonant + ZWJ
 * Halant + Consonant 
 * are also possible.
 * @param scriptcode is one of the scripts (Hard-Coded)
 * @return index if nothing was lifted off vector, return
 * the number of unicode characters + index otherwise.
 * append the output cluster to ret, last element is ligature
 * code - if any.
 * @param finished is set to 1 if exact match happens
 *                           0 is not yet finished
 *                          -1 if illegal sequence start.
 * It also sets yuditClusterError to an appropriate string.
 */
static unsigned int 
getIndicCluster (unsigned int scriptcode, 
  const SV_UCS4& unicode, unsigned int index, SV_UCS4* ret, int* finished)
{
  unsigned int usize = unicode.size();
  unsigned int i;
  if (finished) *finished = 1;
  /* Some platforms have unsigned char */

  char prevchartype = (char)0x7f; /* big enough */
  SS_UCS4 nextLig = 0;
  for (i=index;i<usize; i++)
  { 
   SS_UCS4 next = unicode[i];
   char chartype = (char) indic->encode (next);
//fprintf (stderr, "getIndicCluster=%u %d\n", next, chartype);
   unsigned int sc = getUnicodeScript (next);
   if (sc!=scriptcode && chartype != SD_INDIC_ZWNJ && chartype != SD_INDIC_ZWJ
        && next != 0x25cc)
   {
     if (ret->size()==0)
     {
       /* can not start with it */
       if (finished) *finished=-1;
     }
     nextLig = nextLigature (scriptcode, &unicode.array()[index], i-index);
     if (nextLig) ret->append (nextLig);
     return i; 
   }
   switch (chartype)
   {
   case SD_INDIC_INDEP_VOWEL:
     ret->append (next);
     if (i+1 < usize)
     { 
       SS_UCS4 n = unicode[i+1];
       char ct = (char) indic->encode (n);
       if (ct != SD_INDIC_BOTTOM_VOWEL
            && ct != SD_INDIC_TOP_VOWEL 
            && ct != SD_INDIC_LEFT_VOWEL
            && ct != SD_INDIC_LEFT_RIGHT_VOWEL
            && ct != SD_INDIC_RIGHT_VOWEL
            && ct != SD_INDIC_MODIFIER
            && ct != SD_INDIC_HALANT)
       {
          if (ret->size()==1) return index;
          nextLig = nextLigature (scriptcode, &unicode.array()[index], i-index+1);
          if (nextLig) ret->append (nextLig);
          return i+1;
       }
     }
     break;
   case SD_INDIC_LEFT_VOWEL:
     if (ret->size()==0)
     {
       /* can not start with it */
       if (finished) *finished=-1;
       yuditClusterError = "Cluster should not start with dependent vowel.";
       return index; 
     }
     if (prevchartype != SD_INDIC_CONSONANT_BASE 
           && prevchartype != SD_INDIC_CONSONANT_BELOW_BASE
           && prevchartype != SD_INDIC_CONSONANT_POST_BASE
           && prevchartype != SD_INDIC_CONSONANT_DEAD
           && prevchartype != SD_INDIC_HALANT
           && prevchartype != SD_INDIC_NUKTA
           && prevchartype != SD_INDIC_INDEP_VOWEL)
     {
       yuditClusterError = "Dependent vowel should be preceded by consonant, nukta, halant or independent vowel.";
       nextLig = nextLigature (scriptcode, &unicode.array()[index], i-index);
       if (nextLig) ret->append (nextLig);
       return i;
     }
     if (scriptcode == SD_MALAYALAM)
	ret->insert (ret->size()-1, next);
     else ret->insert (0, next);
     if (i+1 < usize)
     { 
       SS_UCS4 n = unicode[i+1];
       char ct = (char) indic->encode (n);
       if (ct != SD_INDIC_MODIFIER)
       {
         nextLig = nextLigature (scriptcode, &unicode.array()[index],i-index+1);
         if (nextLig) ret->append (nextLig);
         return i+1;
       }
     }
     break;
   case SD_INDIC_LEFT_RIGHT_VOWEL:
     if (ret->size()==0)
     {
       /* can not start with it */
       if (finished) *finished=-1;
       yuditClusterError = "Cluster should not start with dependent vowel.";
       return index; 
     }
     if (prevchartype != SD_INDIC_CONSONANT_BASE 
           && prevchartype != SD_INDIC_CONSONANT_BELOW_BASE
           && prevchartype != SD_INDIC_CONSONANT_POST_BASE
           && prevchartype != SD_INDIC_CONSONANT_DEAD
           && prevchartype != SD_INDIC_HALANT
           && prevchartype != SD_INDIC_NUKTA
           && prevchartype != SD_INDIC_INDEP_VOWEL)
     {
       yuditClusterError = "Dependent vowel should be preceded by consonant, nukta, halant or independent vowel.";
       nextLig = nextLigature (scriptcode, &unicode.array()[index], i-index);
       if (nextLig) ret->append (nextLig);
       return i;
     }
     /* this will be the fallback rendering */
     {
       SS_UCS4 l = getLRVowelLeft (next);
       SS_UCS4 r = getLRVowelRight (next);
       if (l && r)
       {
         if (scriptcode == SD_MALAYALAM)
	    ret->insert (ret->size()-1, l);
	 else ret->insert (0, l);
         ret->append (r);
       }
       else
       {
         ret->append (next);
       }
     }
     if (i+1 < usize)
     { 
       SS_UCS4 n = unicode[i+1];
       char ct = (char) indic->encode (n);
       if (ct != SD_INDIC_MODIFIER)
       {
         nextLig = nextLigature (scriptcode, &unicode.array()[index],i-index+1);
         if (nextLig) ret->append (nextLig);
         return i+1;
       }
     }
     break;
   case SD_INDIC_MODIFIER:
     if (ret->size()==0)
     {
       /* can not start with it */
       yuditClusterError = "Cluster should not start with a modifier.";
       if (finished) *finished=-1;
       return index; 
     }
     if (     prevchartype != SD_INDIC_INDEP_VOWEL 
           && prevchartype != SD_INDIC_TOP_VOWEL
           && prevchartype != SD_INDIC_BOTTOM_VOWEL
           && prevchartype != SD_INDIC_LEFT_VOWEL
           && prevchartype != SD_INDIC_LEFT_RIGHT_VOWEL
           && prevchartype != SD_INDIC_RIGHT_VOWEL
           && prevchartype != SD_INDIC_CONSONANT_BASE
           && prevchartype != SD_INDIC_CONSONANT_BELOW_BASE
           && prevchartype != SD_INDIC_CONSONANT_POST_BASE
           && prevchartype != SD_INDIC_CONSONANT_DEAD
	   && prevchartype != SD_INDIC_NUKTA)
     {
       nextLig = nextLigature (scriptcode, 
               &unicode.array()[index], i-index);
       if (nextLig) ret->append (nextLig);
       return i;
     }
     ret->append (next);
     nextLig = nextLigature (scriptcode, &unicode.array()[index], i-index+1);
     if (nextLig) ret->append (nextLig);
     return i +1;

   case SD_INDIC_SIGN:
     if (ret->size()==0)
     {
       /* can start with it */
       // if (finished) *finished=-1;
       return index; 
     }
     ret->append (next);
     nextLig = nextLigature (scriptcode, &unicode.array()[index], i-index+1);
     if (nextLig) ret->append (nextLig);
     return i+1;

   case SD_INDIC_RIGHT_VOWEL:
   case SD_INDIC_TOP_VOWEL:
   case SD_INDIC_BOTTOM_VOWEL:
     if (ret->size()==0)
     {
       /* can not start with it */
       yuditClusterError = "Cluster should not start with dependent vowel.";
       if (finished) *finished=-1;
       return index; 
     }
     if (prevchartype != SD_INDIC_CONSONANT_BASE 
           && prevchartype != SD_INDIC_CONSONANT_BELOW_BASE
           && prevchartype != SD_INDIC_CONSONANT_POST_BASE
           && prevchartype != SD_INDIC_HALANT
           && prevchartype != SD_INDIC_NUKTA
           && prevchartype != SD_INDIC_CONSONANT_DEAD
	   && prevchartype != SD_INDIC_INDEP_VOWEL)
     {
        yuditClusterError = "Dependent vowel should be preceded by consonant, nukta, halant or independent vowel.";
       nextLig = nextLigature (scriptcode, 
               &unicode.array()[index], i-index);
       if (nextLig) ret->append (nextLig);
       return i;
     }
     ret->append (next);
     if (i+1 < usize)
     { 
       SS_UCS4 n = unicode[i+1];
       char ct = (char) indic->encode (n);
       if (ct != SD_INDIC_MODIFIER)
       {
         nextLig = nextLigature (scriptcode, &unicode.array()[index], i-index+1);
         if (nextLig) ret->append (nextLig);
         return i +1;
       }
     }
     break;
   case SD_INDIC_CONSONANT_BASE:
   case SD_INDIC_CONSONANT_BELOW_BASE:
   case SD_INDIC_CONSONANT_POST_BASE:
     if (ret->size() > 0 && prevchartype != SD_INDIC_HALANT 
           && prevchartype != SD_INDIC_ZWJ 
           && prevchartype != SD_INDIC_CONSONANT_DEAD)
     {
       yuditClusterError = "Consonant should be preceded by halant or nukta or ZWJ";
       nextLig = nextLigature (scriptcode, &unicode.array()[index], i-index);
       if (nextLig) ret->append (nextLig);
       return i;
     }
     ret->append (next);
     if (i+1 < usize)
     { 
       SS_UCS4 n = unicode[i+1];
       char ct = (char) indic->encode (n);
       if (ct != SD_INDIC_HALANT 
            && ct != SD_INDIC_NUKTA
            && ct != SD_INDIC_ZWNJ
            && ct != SD_INDIC_ZWJ
            && ct != SD_INDIC_MODIFIER
            && ct != SD_INDIC_BOTTOM_VOWEL
            && ct != SD_INDIC_TOP_VOWEL 
            && ct != SD_INDIC_LEFT_VOWEL
            && ct != SD_INDIC_LEFT_RIGHT_VOWEL
            && ct != SD_INDIC_CONSONANT_DEAD
            && ct != SD_INDIC_RIGHT_VOWEL)
       {
          if (ret->size()==1) return index;
          nextLig = nextLigature (scriptcode,&unicode.array()[index],i-index+1);
          if (nextLig) ret->append (nextLig);
          return i+1;
       }
     }
     break;
   case SD_INDIC_ZWNJ:
     if (ret->size()==0)
     {
       /* can not start with it */
       yuditClusterError = "Cluster can not start with a ZWNJ.";
       if (finished) *finished=-1;
       return index; 
     }
#if 0
     if (prevchartype != SD_INDIC_HALANT) 
     {
       yuditClusterError = "ZWNJ should be preceded by a halant.";
       nextLig = nextLigature (scriptcode, &unicode.array()[index], i-index);
       if (nextLig) ret->append (nextLig);
       return i;
     }
#endif
     nextLig = nextLigature (scriptcode, &unicode.array()[index], i-index+1);
     if (nextLig) ret->append (nextLig);
     return i+1;
   case SD_INDIC_NUKTA:
     if (ret->size()==0)
     {
       /* can not start with it */
       yuditClusterError = "Cluster can not start with a nukta.";
       if (finished) *finished=-1;
       return index; 
     }
     if (prevchartype != SD_INDIC_CONSONANT_BASE 
        && prevchartype != SD_INDIC_CONSONANT_BELOW_BASE
        && prevchartype != SD_INDIC_CONSONANT_DEAD
        && prevchartype != SD_INDIC_CONSONANT_POST_BASE)
     {
       yuditClusterError = "Nukta should be preceded by a consonant.";
       nextLig = nextLigature (scriptcode, &unicode.array()[index], i-index);
       if (nextLig) ret->append (nextLig);
       return i;
     }
     ret->append (next);
     if (i+1 < usize)
     { 
       SS_UCS4 n = unicode[i+1];
       char ct = (char) indic->encode (n);
       if (ct != SD_INDIC_HALANT 
            && ct != SD_INDIC_MODIFIER
            && ct != SD_INDIC_BOTTOM_VOWEL
            && ct != SD_INDIC_TOP_VOWEL 
            && ct != SD_INDIC_LEFT_VOWEL
            && ct != SD_INDIC_LEFT_RIGHT_VOWEL
            && ct != SD_INDIC_RIGHT_VOWEL)
       {
          if (ret->size()==1) return index;
          nextLig = nextLigature (scriptcode,&unicode.array()[index],i-index+1);
          if (nextLig) ret->append (nextLig);
          return i+1;
       }
     }
     break;
   case SD_INDIC_ZWJ:
     // Bengali can start with ZWJ - it needs a little work.
#if 0
     if (ret->size()==0)
     {
       /* can not start with it */
       yuditClusterError = "Cluster can not start with a ZWJ.";
       if (finished) *finished=-1;
       return index; 
     }
     if (prevchartype != SD_INDIC_HALANT)
     {
       yuditClusterError = "ZWJ should be preceded by a halant.";
       nextLig = nextLigature (scriptcode, &unicode.array()[index], i-index);
       if (nextLig) ret->append (nextLig);
       return i;
     }
#endif
     ret->append (next);
     break;
   case SD_INDIC_HALANT:
     // Bengali can start with a halant - Yaphala
     if (next != 0x09cd && ret->size()==0)
     {
       /* can not start with it */
       yuditClusterError = "Cluster can not start with a halant.";
       if (finished) *finished=-1;
       return index; 
     }
     if (next != 0x09cd 
          && prevchartype != SD_INDIC_INDEP_VOWEL 
          && prevchartype != SD_INDIC_CONSONANT_BASE
          && prevchartype != SD_INDIC_CONSONANT_BELOW_BASE
          && prevchartype != SD_INDIC_CONSONANT_POST_BASE
          && prevchartype != SD_INDIC_NUKTA)
     {
       yuditClusterError = "Halant should be preceded by an independent vowel, a consonant or nukta.";
       nextLig = nextLigature (scriptcode, &unicode.array()[index], i-index);
       if (nextLig) ret->append (nextLig);
       return i;
     }
     ret->append (next);
     break;
   case SD_INDIC_CONSONANT_DEAD:
     // Finish the cluster - I dont know any better solution.
     nextLig = nextLigature (scriptcode, &unicode.array()[index], i-index+1);
     if (nextLig) ret->append (nextLig);
     return i+1;
   default:
     if (ret->size()==0)
     {
       if (finished) *finished=1;
       return index; 
     }
     nextLig = nextLigature (scriptcode, &unicode.array()[index], i-index);
     if (nextLig) ret->append (nextLig);
     return i;
    break;
   }
   prevchartype = chartype;
  }
  if (finished) *finished = 0;
  if (ret->size()>1)
  {
     nextLig = nextLigature (scriptcode, &unicode.array()[index], i-index);
     if (nextLig) ret->append (nextLig);
     return i;
  }
  ret->clear();
  return index;
}


/**
 * Generate a next ligature number if it still does not exist
 */
static SS_UCS4 nextLigature (unsigned int script, 
   const SS_UCS4* unicode, unsigned int length)
{
  initLigatures ();
  if (length<2) return 0;

  SString key = SString((char*)unicode, sizeof (SS_UCS4) * length);
  const SString* cac = ligatureCache->get (key);
  SS_UCS4 liga;
  if (cac && cac->size()==sizeof (SS_UCS4))
  {
    liga = *(SS_UCS4*) (cac->array());
    return liga;
  }
  liga = counters[script];

  /* check overflow */
  if ((liga & 0xffff) == 0xffff) return 0;
  liga++;
  counters[script] = liga;
  /* FIXME: check overflow */
  SString vle = SString((char*)&liga, sizeof (SS_UCS4));
  ligatureCache->put (key, vle);
  //fprintf (stderr, "New Ligature[%d]=%X\n", script, liga);
  return liga;
}

int
getUnicodeScript (SS_UCS4 comp)
{
  /* TONE LETTERS */
  switch (comp)
  {
  case 0x304B: return SD_YUDIT;
  case 0x304D: return SD_YUDIT;
  case 0x304F: return SD_YUDIT;
  case 0x3051: return SD_YUDIT;
  case 0x3053: return SD_YUDIT;
  case 0x30AB: return SD_YUDIT;
  case 0x30AD: return SD_YUDIT;
  case 0x30AF: return SD_YUDIT;
  case 0x30B1: return SD_YUDIT;
  case 0x30B3: return SD_YUDIT;
  case 0x30BB: return SD_YUDIT;
  case 0x30C4: return SD_YUDIT;
  case 0x30C8: return SD_YUDIT;
  case 0x31F7: return SD_YUDIT;
  case 0x00E6: return SD_YUDIT;
  case 0x0254: return SD_YUDIT;
  case 0x028C: return SD_YUDIT;
  case 0x0259: return SD_YUDIT;
  case 0x025A: return SD_YUDIT;
  default: break;
  }
  if (comp >= 0x02E5 && comp <= 0x02E9) return SD_YUDIT;
  if (getJamoClass (comp)>0) return SD_HANGUL_JAMO;

  if (comp >= 0x1f1e6 && comp <= 0x1f1ff) {
    return SD_REGIONAL_INDICATOR_SYMBOL;
  }

  if (comp >= 0x1000) 
  {
    if (getRovasType (comp) == 1)
    {
        return SD_ROVASIRAS;
    }
    if (getPUARovasType (comp) == 1)
    {
        return SD_PUA_ROVAS;
    }
    return -1;
  }

  if (comp < 0x0900 ) return -1; 
  if (comp < 0x0980) return SD_DEVANAGARI;
  if (comp < 0x0A00) return SD_BENGALI;
  if (comp < 0x0A80) return SD_GURMUKHI;
  if (comp < 0x0B00) return SD_GUJARATI;
  if (comp < 0x0B80) return SD_ORIYA;
  if (comp < 0x0C00) return SD_TAMIL;
  if (comp < 0x0C80) return SD_TELUGU;
  if (comp < 0x0D00) return SD_KANNADA;
  if (comp < 0x0D80) return SD_MALAYALAM;
  if (comp < 0x0E00) return SD_SINHALA;
  if (comp < 0x0E80) return SD_THAI;
  if (comp < 0x0F00) return SD_LAO;
  if (comp < 0x0FFF) return SD_TIBETAN;
  return -1;
}
/**
 * return true if this is covered
 */
bool
isCoveredScipt (SS_UCS4 comp, int sc)
{
  switch (sc)
  {
  case SD_YUDIT: return false;
  case SD_REGIONAL_INDICATOR_SYMBOL: return (comp>=0x1f1e6 && comp<=0x1f1ff);
  case SD_DEVANAGARI: return (comp>=0x0900 && comp<0x0980);
  case SD_BENGALI: return (comp>=0x0980 && comp<0x0a00);
  case SD_BENGALI_BEGIN: return (comp>=0x0980 && comp<0x0a00);
  case SD_GURMUKHI: return (comp>=0x0a00 && comp<0x0a80);
  case SD_GUJARATI: return (comp>=0x0a80 && comp<0x0b00);
  case SD_ORIYA: return (comp>=0x0b00 && comp<0x0b80);
  case SD_TAMIL: return (comp>=0x0b80 && comp<0x0c00);
  case SD_TELUGU: return (comp>=0x0c00 && comp<0x0c80);
  case SD_KANNADA: return (comp>=0x0c80 && comp<0x0d00);
  case SD_MALAYALAM: return (comp>=0x0d00 && comp<0x0d80);
  case SD_SINHALA: return (comp>=0x0d80 && comp<0x0e00);
  case SD_THAI: return (comp>=0x0e00 && comp<0x0e80);
  case SD_LAO: return (comp>=0x0e80 && comp<0x0f00);
  case SD_TIBETAN: return (comp>=0x0f00 && comp<0x0fff);
  case SD_HANGUL_JAMO: return (getJamoClass(comp) != 0);
  case SD_HANGUL_PREC: return (getJamoClass(comp) != 0);
  }
  return false;
}


/**
 * Add combining ligature. A combining ligature is a ligature
 * with combining marks. The ligature can be a unicode or
 8 Yudit ligature.
 * @param unicode is the unicode representation of the while thing
 * @param ul is the unicode repr. length
 * @param ligAndMarks contains one ligature + all the marks to it.
 * @param cl is the length of ligAndMarks.
 */  
SS_UCS4
addCombiningLigature (const SS_UCS4* unicode, unsigned int ul,
  const SS_UCS4* ligAndMarks, unsigned int cl)
{
  SS_UCS4 nl = nextLigature (SD_COMBINING_LIGATURE, unicode, ul);
  const SString* found = ligatureUnics->get (
      SString((char*) &nl, sizeof (SS_UCS4)));
  if (found == 0)
  {
     putLigatureUnicode (nl, unicode, ul);
     putLigatureCluster (nl, ligAndMarks, cl);
  }
  return nl;
}

/**
 * Put ligature away to remember
 */
void
putLigatureUnicode (SS_UCS4 ligature, const SS_UCS4* buffer, unsigned int bufsize)
{
  if (ligature <=  0x80000000 || ligature >= 0xA0000000) return;
  initLigatures();
  SString key ((char*)& ligature, sizeof (SS_UCS4));
  const SString* ret = ligatureUnics->get (key);
  if (ret) return; /* already there */
  ligatureUnics->put (key, SString((char*)buffer, bufsize * sizeof (SS_UCS4)));
}

/**
 * Put ligature away to remember
 */
void
putLigatureCluster (SS_UCS4 ligature, const SS_UCS4* buffer, unsigned int bufsize)
{
  if (ligature <=  0x80000000 || ligature >= 0xA0000000) return;
  initLigatures ();
  SString key ((char*)& ligature, sizeof (SS_UCS4));
  const SString* ret = ligatureClust->get (key);
  if (ret) return; /* already there */
  ligatureClust->put (key, SString((char*)buffer, bufsize * sizeof (SS_UCS4)));
}

unsigned int
getLigatureUnicode (SS_UCS4 lig, SS_UCS4* buffer)
{
  SS_UCS4 ligature = lig;
  int sc = getLigatureScriptCode(ligature);
  //
  // SD_BENGALI_BEGIN is an artificial shape-code.
  // 
  if (sc == SD_BENGALI_BEGIN)
  {
     unsigned int en = (SD_BENGALI << 16) |  0x80000000;
     ligature =  (ligature & 0xffff) | en;
  }
  if (ligatureUnics == 0) return 0;
  const SString* ret = ligatureUnics->get (
      SString((char*) &ligature, sizeof (SS_UCS4)));
  if (ret==0) return 0;
  if (buffer==0) return ret->size()/sizeof (SS_UCS4);
  memcpy (buffer, ret->array(), ret->size());
  return ret->size()/sizeof (SS_UCS4);
}

unsigned int
getLigatureCluster (SS_UCS4 lig, SS_UCS4* buffer)
{
  SS_UCS4 ligature = lig;
  int sc = getLigatureScriptCode(ligature);
  //
  // SD_BENGALI_BEGIN is an artificial shape-code.
  // 
  if (sc == SD_BENGALI_BEGIN)
  {
     unsigned int en = (SD_BENGALI << 16) |  0x80000000;
     ligature =  (ligature & 0xffff) | en;
  }
  if (ligatureClust == 0) return 0;
  const SString* ret = ligatureClust->get (
      SString((char*) &ligature, sizeof (SS_UCS4)));
  if (ret==0) return 0;
  if (buffer==0) return ret->size()/sizeof (SS_UCS4);
  memcpy (buffer, ret->array(), ret->size());
  return ret->size()/sizeof (SS_UCS4);
}

static void
initLigatures()
{
  if (ligatureUnics == 0)
  {
    clusters = new SUniMap("cluster");
    CHECK_NEW (clusters);
    indic = new SUniMap("indic");
    CHECK_NEW (indic);

    ligatureUnics = new SProperties(); 
    CHECK_NEW (ligatureUnics);
    ligatureClust = new SProperties();
    CHECK_NEW (ligatureClust);
    ligatureCache = new SProperties();
    CHECK_NEW (ligatureCache);
    for (unsigned int i=0; i<SD_SCRIPT_MAX; i++)
    {
      counters[i] = 0x80000000 + (0x10000 * i);
    }
  }
}

int 
getLigatureScriptCode (SS_UCS4 comp)
{
  if (comp < 0x80000000) return -1;
  SS_UCS4 en = comp & 0x7fff0000;
  en = en >> 16;
  return (int) en;
}

/* get script name or null */
const char*
getLigatureScript (SS_UCS4 comp)
{
  if (comp <= 0x80000000 || comp >= 0xA0000000) return 0;
  SS_UCS4 en = comp & 0x7fff0000;
  en = en >> 16;
  /* I modified this to return Script name as in MS Opentype spec.*/
  switch (en)
  {
  case SD_YUDIT: return "yudit";
  case SD_DEVANAGARI: return "deva";
  case SD_BENGALI: return "beng";
  case SD_BENGALI_BEGIN: return "beng";
  case SD_GURMUKHI: return "guru";
  case SD_GUJARATI: return "gujr";
  case SD_ORIYA: return "orya";
  case SD_TAMIL: return "taml";
  case SD_TELUGU: return "telu";
  case SD_KANNADA: return "knda";
  case SD_MALAYALAM: return "mlym";
  case SD_SINHALA: return "sinh";
  case SD_HANGUL_JAMO: return "jamo";
  case SD_HANGUL_PREC: return "hang";
  case SD_THAI: return "thai";
  case SD_LAO: return "lao ";
  case SD_TIBETAN: return "tibt";
  case SD_ROVASIRAS: return "rovs";
  case SD_PUA_ROVAS: return "prvs";
  case SD_REGIONAL_INDICATOR_SYMBOL: return "flag";
  }
  return 0;
}

bool
isLigature (SS_UCS4 _comp)
{
  /* Yudit ligatures below 0x80008000 are considered hacked glyphs only */
  return (_comp >= 0x80008000 && _comp > 0x80000000 && _comp < 0xA0000000);
}

SS_UCS4
getHalant (int index)
{
  switch (index)
  {
  case SD_DEVANAGARI: 
    return 0x094D;
  case SD_BENGALI: 
    return 0x09CD;
  case SD_BENGALI_BEGIN: 
    return 0x09CD;
  case SD_GURMUKHI: 
    return 0x0A4D;
  case SD_GUJARATI: 
    return 0x0ACD;
  case SD_ORIYA: 
    return 0x0B4D;
  case SD_TELUGU:
    return 0x0C4D;
  case SD_KANNADA:
    return 0x0CCD;
  case SD_MALAYALAM:
    return 0x0D4D;
  case SD_SINHALA:
    return 0x0DCD;
  default:
    return 0;
  }
  return 0;
}

int getCharType (SS_UCS4 unchar)
{
  initLigatures();
  char echartype = (char) indic->encode (unchar);
  return (int) echartype;
}

/**
 * get left part of LR vowel
 */
SS_UCS4
getLRVowelLeft (SS_UCS4 u)
{
  switch (u)
  {
  case 0x09CB:
  case 0x09CC:
    return 0x09c7;
  case 0x0b4b:
  case 0x0b4c:
    return 0x0b47;
  case 0x0d4b:
    return 0x0d47;
  case 0x0d4a:
  case 0x0d4c:
    return 0x0d46;
  default:
    break;
  }
  return 0;
}
/**
 * get right part of LR vowel
 */
SS_UCS4
getLRVowelRight (SS_UCS4 u)
{
  switch (u)
  {
  case 0x09CB:
    return 0x09be;
  case 0x09CC:
    return 0x09d7;
  case 0x0b4b:
    return 0x0b3e;
  case 0x0b4c:
    return 0x0b57;
  case 0x0d4a:
  case 0x0d4b:
    return 0x0d3e;
  case 0x0d4c:
    return 0x0d57;
  default:
    break;
  }
  return 0;
}

/**
 * Decompose yudit ligature into unicode characters
 */
void
expandYuditLigatures (SV_UCS4* decd)
{
  if (decd->size()!=1 || (*decd)[0] < 0x80000000) return;
  SS_UCS4 ucs4 = (*decd)[0];
  decd->remove (0);
  /* Yudit ligatures*/
  switch (ucs4)
  {
  case 0x80000010: /* JIS X 0213: 02B65 */
    decd->append (0x02E9);
    decd->append (0x02E5);
    break;
  case 0x80000011: /* JIS X 0213: 02B66 */
    decd->append (0x02E5);
    decd->append (0x02E9);
    break;
// Generated by ./jiscompose.pl at 2002-04-15
// Add this to stoolkit/SCluster.cpp expandYuditLigatures
  case 0x80000040: /* JIS X 0213: 0x2477 */
    decd->append (0x304B);
    decd->append (0x309A);
    break;
  case 0x80000041: /* JIS X 0213: 0x2478 */
    decd->append (0x304D);
    decd->append (0x309A);
    break;
  case 0x80000042: /* JIS X 0213: 0x2479 */
    decd->append (0x304F);
    decd->append (0x309A);
    break;
  case 0x80000043: /* JIS X 0213: 0x247A */
    decd->append (0x3051);
    decd->append (0x309A);
    break;
  case 0x80000044: /* JIS X 0213: 0x247B */
    decd->append (0x3053);
    decd->append (0x309A);
    break;
  case 0x80000045: /* JIS X 0213: 0x2577 */
    decd->append (0x30AB);
    decd->append (0x309A);
    break;
  case 0x80000046: /* JIS X 0213: 0x2578 */
    decd->append (0x30AD);
    decd->append (0x309A);
    break;
  case 0x80000047: /* JIS X 0213: 0x2579 */
    decd->append (0x30AF);
    decd->append (0x309A);
    break;
  case 0x80000048: /* JIS X 0213: 0x257A */
    decd->append (0x30B1);
    decd->append (0x309A);
    break;
  case 0x80000049: /* JIS X 0213: 0x257B */
    decd->append (0x30B3);
    decd->append (0x309A);
    break;
  case 0x8000004A: /* JIS X 0213: 0x257C */
    decd->append (0x30BB);
    decd->append (0x309A);
    break;
  case 0x8000004B: /* JIS X 0213: 0x257D */
    decd->append (0x30C4);
    decd->append (0x309A);
    break;
  case 0x8000004C: /* JIS X 0213: 0x257E */
    decd->append (0x30C8);
    decd->append (0x309A);
    break;
  case 0x8000004D: /* JIS X 0213: 0x2678 */
    decd->append (0x31F7);
    decd->append (0x309A);
    break;
  case 0x8000004E: /* JIS X 0213: 0x2B44 */
    decd->append (0x00E6);
    decd->append (0x0300);
    break;
  case 0x8000004F: /* JIS X 0213: 0x2B48 */
    decd->append (0x0254);
    decd->append (0x0300);
    break;
  case 0x80000050: /* JIS X 0213: 0x2B49 */
    decd->append (0x0254);
    decd->append (0x0301);
    break;
  case 0x80000051: /* JIS X 0213: 0x2B4A */
    decd->append (0x028C);
    decd->append (0x0300);
    break;
  case 0x80000052: /* JIS X 0213: 0x2B4B */
    decd->append (0x028C);
    decd->append (0x0301);
    break;
  case 0x80000053: /* JIS X 0213: 0x2B4C */
    decd->append (0x0259);
    decd->append (0x0300);
    break;
  case 0x80000054: /* JIS X 0213: 0x2B4D */
    decd->append (0x0259);
    decd->append (0x0301);
    break;
  case 0x80000055: /* JIS X 0213: 0x2B4E */
    decd->append (0x025A);
    decd->append (0x0300);
    break;
  case 0x80000056: /* JIS X 0213: 0x2B4F */
    decd->append (0x025A);
    decd->append (0x0301);
    break;
// END OF ./jiscompose.pl
  default:
    break;
  }
  if (decd->size()==0) decd->append(0xfffd);
  return;
}

/**
 * Get the Jamo class
 * @param ucs is the unicode character
 * @return one of 
 * <ul>
 *  <li> SD_JAMO_X </li>
 *  <li> SD_JAMO_L </li>
 *  <li> SD_JAMO_V </li>
 *  <li> SD_JAMO_T </li>
 * </ul>
 */
int
getJamoClass (SS_UCS4 uc)
{
  if (uc >= 0x1100 && uc <= 0x115f) return SD_JAMO_L;
  if (uc >= 0x1160 && uc <= 0x11a2) return SD_JAMO_V;
  if (uc >= 0x11a8 && uc <= 0x11f9) return SD_JAMO_T;
  return SD_JAMO_X;
}

/* get the name of OTF font shaping feature name */
const char*
getShapeCode (unsigned int icode)
{
  static const char* shapes[] = {
     "isol",
     "init",
     "medi",
     "fina",
     "med2",
     "fin2",
     "fin3",
     "init",
  };
  if (icode >= 8) return "unknown";
  return shapes[icode];
}
