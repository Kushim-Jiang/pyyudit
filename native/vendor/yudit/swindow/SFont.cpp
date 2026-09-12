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
 
#include "swindow/SFont.h"

#include "swindow/SFontFB.h"
#include "stoolkit/SMatrix.h"
#include "stoolkit/SHashtable.h"
#include "stoolkit/STextData.h"
#include "stoolkit/SExcept.h"
#include "stoolkit/SCluster.h"

const SString&
SFont::getDefaultFontList ()
{
    static SString ret;
    if (ret.size() > 0) {
        return ret;
    }
    ret = SString (
        /* 16: yudit.hex has also Hosszu Gabors PUA old hungarian. */
        "yudit.hex,arabforms.hex,syriacforms.hex,unifont.hex,"
        /* 18: */
        "markus9x18.bdf,markus18x18ja.bdf,"
        // recommended Noto fonts
        "NotoSansGujarati-Regular.ttf:gujr,"
        "NotoSansDevanagari-Regular.ttf:deva,"
        "NotoSansBengali-Regular.ttf:beng,"
        "NotoSansGurmukhi-Regular.ttf:guru,"
        "NotoSansOriya-Regular.ttf:orya,"
        "NotoSansTamil-Regular.ttf:taml,"
        "NotoSansTelugu-Regular.ttf:telu,"
        "NotoSansMalayalam-Regular.ttf:mlym,"
        "NotoSansSinhala-Regular.ttf:sinh,"
        "NotoSansThai-Regular.ttf:thai,"
        "NotoSansLao-Regular.ttf:lao,"
        "NotoSansKannada-Regular.ttf:knda,"
/* This screws up line height. removed. add it as tibt.tttf if you need it.
        "NotoSansTibetan-Regular.ttf:tibt," 
*/
        // I kept this excellent font 
        "raghu.ttf:deva,"

        "Arial.ttf,"
        "NotoSans-Regular.ttf,"

        // Japanese
        "VLGothicReqular.ttf,"
        // Hangul 
        "gulim.ttf,"
        "ogulim.ttf:mslvt,"
//        "SansSerif.ttf,"
        "NotoSansEthiopic-Regular.ttf,"
        "NotoSansArabic-Regular.ttf,"
        "NotoSansSyriac-Regular.ttf,"
        "NotoSansTagalog-Regular.ttf,"
        "NotoSansRunic-Regular.ttf,"
        "NotoSansOldItalic-Regular.ttf,"
        "NotoSansSymbols-Regular.ttf,"
        "NotoSansSymbols2-Regular.ttf,"
        "NotoSansCanadianAboriginal-Regular.ttf,"
        "NotoSansCherokee-Regular.ttf,"
        "NotoSansGlagolitic-Regular.ttf,"
        "NotoSansHanunoo-Regular.ttf,"
        "NotoSansShavian-Regular.ttf,"
        // emoji, Ubuntu installs it by font-name
        "EmojiOne_Color.otf:emoji,"
        // emoji, Mac installs it by file-name
        "EmojiOneColor.otf:emoji,"

        "TwitterColorEmoji-SVGinOT.ttf:emoji,"
        /* ubuntu system usually has these */
        "Ubuntu-R.ttf,"
        "fonts-japanese-gothic.ttf,"
        // yum install ipa-gothic-fonts ipa-pgothic-fonts
        "ipag.ttf,"
        /* Macintosh */
        "SFNS.ttf,"
        "SFArabic.ttf,"
        // Last resorts
        "Arialuni.ttf,"
        "Arial Unicode.ttf,"
        // Font vendor: Agfa Monotype Corporation
        "arial-unicode-ms.ttf,"
        // From Yudit 
        "OldHungarian_Full.ttf:unicode:RL,"
        //"FreeSans.ttf,"
        "unifont.ttf,"
        // From Yudit 
        "yudit.ttf,"
        /* X11 */
        "-*-*-medium-r-normal--16-*-*-*-c-*-iso8859-1,"
        "-*-*-*-*-*--16-*-*-*-c-*-iso8859-1,"
    );
    return ret;
}

static SFontFB fallbackFont;
typedef SHashtable<SFontImplVector> SFontHashtable;
static SFontHashtable* fontHashtable=0;

static SFontFB::SIGN mapFB (const SGlyph& glyph);

/* write the composing character below this */

/**
 * Create a font. The font can have many faces. 
 * @param _name is the logical name of this font.
 */
SFont::SFont (const SString _name) : name(_name), xlfd (SD_XLFD_ANY)
{
 if (fontHashtable == 0 ) fontHashtable = new SFontHashtable();
 if (fontHashtable->get (name) != 0) fontVector = (*fontHashtable)[name];
 setSize (16.0);
}

/**
 * Create a font. The font can have many faces. 
 * @param _name is the logical name of this font.
 */
SFont::SFont (const SString _name, double _size) : name(_name), xlfd (SD_XLFD_ANY)
{
 if (fontHashtable == 0 ) fontHashtable = new SFontHashtable();
 if (fontHashtable->get (name) != 0) fontVector = (*fontHashtable)[name];
 setSize (_size);
}


/**
 * get a point 16 font called _default
 */
SFont::SFont (void) : name("default"), xlfd (SD_XLFD_ANY)
{
  static bool _defaultSet=false;
  if (_defaultSet==false)
  {
    const SString& fl = getDefaultFontList();
    SStringVector m(fl);
    SFontImplVector list;
    for (unsigned int i=0; i<m.size(); i++)
    {
      /* encoding is optional */
      SStringVector v(m[i], ":", false);
      SString enc = v[0];

      if (v.size()>1 && v[1].size()!=0) enc = v[1];

      SFontImpl impl (v[0], enc);
      if (v.size()>2 && v[2].size()>0)
      {
         SStringVector pvect(v[2], ";");
         SProperties props;
         for (unsigned int j=0; j<pvect.size(); j++)
         {
            SStringVector vv(pvect[j], "=", true);
            if (vv.size() > 1)
            {
               props.put (vv[0], vv[1]);
            }
            else
            {
               props.put (vv[0], "true");
            }
         }
         impl.setAttributes(props);
      }
      list.append (impl);
    }
    put ("default", list);
  }
  if (fontHashtable == 0 ) fontHashtable = new SFontHashtable();
  fontVector = (*fontHashtable)["default"];
  setSize (16.0);
  
}

/**
 * Copy this font.
 */
SFont::SFont (const SFont& font)
{
  name = font.name;
  xlfd = font.xlfd;
  fontAscent = font.fontAscent;
  fontDescent = font.fontDescent;
  fontWidth = font.fontWidth;
  fontGap = font.fontGap;
  fontVector = font.fontVector;
  fontScale = font.fontScale;
  fallbackScale = font.fallbackScale;
}

/**
 * assign this font.
 */
SFont
SFont::operator = (const SFont& font)
{
  if (&font == this) return *this;
  name = font.name;
  fontAscent = font.fontAscent;
  fontDescent = font.fontDescent;
  fontWidth = font.fontWidth;
  fontGap = font.fontGap;
  xlfd = font.xlfd;
  fontScale = font.fontScale;
  fallbackScale = font.fallbackScale;
  fontVector = font.fontVector;
  return *this;
}

SFont::~SFont()
{
}


/**
 * Go through the list and set the size.
 */
void
SFont::setSize(double points)
{
  fontScale = points;
  fallbackScale = points;
  SS_Matrix2D m;
  double sc = fallbackFont.scale();
  m.scale (fontScale * sc, fontScale * sc);
  fontGap = 0.0;
  /* at least */
  fontAscent = -1.0;
  fontDescent = 0.0;

  for (unsigned int i=0; i<fontVector.size(); i++)
  {
    SFontImpl im = fontVector[i];
    im.scale (fontScale, fontScale);
    //fontVector.remove (i);
    fontVector.replace (i, im);
    double g = im.gap ();
    double a = im.ascent ();
    double d = im.descent ();
    if (g > fontGap) fontGap = g;
    if (a > fontAscent) fontAscent = a;
    if (d > fontDescent) fontDescent = d;
  }
  if (fontAscent <= 1.0 || fontAscent < points/2)
  {
    fontGap =  fallbackFont.gap(m);
    fontAscent =  fallbackFont.ascent(m);
    fontDescent =  fallbackFont.descent(m);
    if (fontAscent < 1.0) fontAscent = 1.0;
  }
  fallbackScale = fontAscent+fontDescent;
}

/**
 * Get the size of the font.
 * @return the size in points
 */
double
SFont::getSize () const
{
  return fontScale;
}

/**
 * A static method to build a font list.
 * This is only use at initialization time.
 * @param name is the name of the font.
 * @param gface is the face to add to the name.
 */
void
SFont::put (const SString name, const SFontImplVector& face)
{
  if (fontHashtable == 0 ) fontHashtable = new SFontHashtable();
  fontHashtable->put (name, face);
}

/**
 * clear all the stuff in the list
 */
void
SFont::clear()
{
}

/**
 * map a fallback font sign.
 */
static SFontFB::SIGN
mapFB (const SGlyph& glyph)
{
  if (glyph.decompSize() > 1)
  {
    if (glyph[0] == SD_CD_CR || glyph[1] == SD_CD_LF) return SFontFB::CRLF;
    if (glyph[1] == SD_CD_CR || glyph[0] == SD_CD_LF) return SFontFB::LFCR;
  }
  else
  {
    if (glyph.getChar() == SD_CD_CR) return SFontFB::CR;
    if (glyph.getChar() == SD_CD_LF) return SFontFB::LF;

    if (glyph.getChar() == SD_CD_LS) return SFontFB::LS;
    if (glyph.getChar() == SD_CD_FF) return SFontFB::FF;

    if (glyph.getChar() == SD_CD_PS) return SFontFB::PS;
    if (glyph.getChar() == SD_CD_TAB) return SFontFB::TAB;
    if (glyph.getChar() == SD_CD_LRM) return SFontFB::LRM;
    if (glyph.getChar() == SD_CD_RLM) return SFontFB::RLM;
    if (glyph.getChar() == SD_CD_ZWJ) return SFontFB::FB_ZWJ;
    if (glyph.getChar() == SD_CD_ZWNJ) return SFontFB::FB_ZWNJ;
  }
  return SFontFB::CTRL;
}

/**
 * Try to draw one single glyph.
 * @param canvas is the canvas to draw to 
 * @param m is the conversion matrix
 * @param uch is the array containing ucs4 
 * @prama len is the length of the array
 */
void
SFont::draw (SCanvas* canvas, const SPen& pen, const SS_Matrix2D& m, 
  const SGlyph& glyph)
{
  double currw = 0.0;
  bool isSelected = glyph.selected;
  if (glyph.isSpecial())
  {
    SS_Matrix2D sd;
    double sc = fallbackFont.scale();
    sd.scale (fallbackScale * sc, fallbackScale * sc);
    sd.translate (0, -fontDescent);
    SS_Matrix2D sm =  m * sd;
    SFontFB::SIGN sig = mapFB (glyph);
    /* The markers are not mirrored. */
    if (!glyph.isLR() && sig != SFontFB::LRM && sig != SFontFB::RLM)
    {
      currw =  fallbackFont.signWidth (sm, sig);
      /* mirroring */
      sm.x0 = -sm.x0;
      sm.t0 = sm.t0 + currw;
    }
    fallbackFont.signDraw(canvas, pen, sm, sig, glyph.getFirstChar());
    return;
  }


  /* try to draw it with all fonts. */
  SS_UCS4 comp =  glyph.getShapedChar();
  unsigned int scriptcode = getLigatureScriptCode (comp);
  SV_UCS4 ligclust;
  if (scriptcode == SD_COMBINING_LIGATURE)
  {
    unsigned int lgsize  = getLigatureCluster (comp, 0);
    if (lgsize > 1)
    {
      SS_UCS4* lc = new SS_UCS4[lgsize];
      CHECK_NEW (lc);
      getLigatureCluster (comp, lc);
      comp = lc[0];
      for (unsigned int i=0; i<lgsize; i++)
      {
        ligclust.append (lc[i]);
      }
      delete[] lc;
    }
  }
  else if (!glyph.isLR() && glyph.isMirrorable())
  {
    comp = glyph.getMirroredChar();
  }

  /* gsize is the full size */
  unsigned int gsize = glyph.decompSize();
  /* This array is the full array */
  const SS_UCS4* decomp = glyph.getDecompArray(); 

  if (comp == 0 && glyph.decompSize() == 1) comp = decomp[0];

  /* first try the precomposed */
  bool baseOK = false;
  double baseWidth = 0.0;
  setBase (comp);
  if (comp != 0 && comp!= 0x200c && comp != 0x200d)
  {
    unsigned int i;

    /* use mirrored glyphs for Old Hungarian, Old Italic */
    /* getCharClass ROVASIRAS CAPS / ROVASIRAS SMALL */
    // SFontImpl::needSoftMirror, 
    // getPUARovasType
    if (  (comp >= 0xee00 && comp < 0xef3f)
        || (comp >= 0x10300 && comp <= 0x1032F)
        || (comp >= 0x10c00 && comp <= 0x10fff)
        || scriptcode == SD_ROVASIRAS || scriptcode == SD_PUA_ROVAS)
    {
      /* Try to use lr and rl attributes. This is the non-mirrored dwaring. */
      for (i=0; i<fontVector.size(); i++)
      {
        SFontImpl im = fontVector[i];
        // Skip cases where font is strongly the opposite direction.
        if (im.needSoftMirror (comp, glyph.isLR())) continue;
        
        /* this is for better positioning of diacritical marks. */
        if (im.draw (canvas, pen, m, comp, glyph.isLR(), isSelected, baseOK)) 
        {
          im.width (comp, &baseWidth);
          baseOK=true;
          break;
        }
      }
      SS_Matrix2D mm =  m;
      /* Try to mirror it - we drew neutrals and same directions */
      if (!baseOK) for (i=0; i<fontVector.size(); i++)
      {
        SFontImpl im = fontVector[i];
        bool used = im.width (comp, &currw);
        if (used)
        {
          /* Mirrorring can be done only if we render on our own. */
          mm.x0 = -m.x0;  /* try to indicate mirroring */
          if (im.isTTF())
          {
            mm.t0 = m.t0 + currw;
          }
          im.draw (canvas, pen, mm, comp, glyph.isLR(), isSelected, baseOK); 
          baseWidth = currw;
          baseOK = true;
          break;
        }
        currw = 0.0;
      }
      // Try ligatures composed with ZWJ
      // commented out as this is done in im for OldHUngarian and OldItalic
#if 0
      if (false && !baseOK && comp > 0x7fffffff) {
         unsigned int liglen = getLigatureUnicode(comp, 0);
         if (liglen > 0) {
            SS_UCS4* chars =  new SS_UCS4[liglen];
            CHECK_NEW (chars);
            getLigatureUnicode (comp, chars);
            unsigned int j;
            for (i=0; i<fontVector.size(); i++) {
                SFontImpl im = fontVector[i];
                for (j=0; j<liglen; j++) {
                    if (chars[j] == SD_CD_ZWJ) {
                        if (j%2 != 1) break;
                        continue;
                    }
                    if (j%2 != 0) break;
                    if (!im.width (chars[j], &currw)) {
                        break;
                    }
                }
                if (j < liglen) continue;
                baseWidth = 0;
                for (j=0; j<liglen; j++) {
                    SS_UCS4 ch = glyph.isLR() ? chars[j] 
                        : chars[liglen-j-1];
                    if (ch == SD_CD_ZWJ) continue;
                    unsigned int progress = 0;
                    im.width (ch, &currw);
                    baseWidth += currw;
                    SS_Matrix2D mm =  m;
                    if (im.needSoftMirror (ch, glyph.isLR())) {
                        mm.x0 = -m.x0;  // try to indicate mirroring
                        if (im.isTTF())
                        {
                            mm.t0 = m.t0 + currw;
                        }
                    }
                    mm.t0 = mm.t0 + progress;
             //       im.draw (canvas, pen, mm, ch, glyph.isLR()); 
                    progress += currw;
                }
                baseOK = true;
            }
            delete [] chars;
         }
      }
#endif
    }
    else
    {
//fprintf (stderr, "XXX ScriptCode = %x shapedcahr=%x\n", scriptcode, comp);
       /* Try all fonts on it */
      for (i=0; i<fontVector.size(); i++)
      {
        SFontImpl im = fontVector[i];
        if (im.draw (canvas, pen, m, comp, glyph.isLR(), isSelected, baseOK)) 
        {
          im.width (comp, &baseWidth);
          baseOK = true;
          break;
        }
      }
    }
  }

  /*
   * If it is shaped and current shape is isolated fallback 
   */
  if (!baseOK && glyph.getShapeArray()!=0 && glyph.currentShape == 0)// ISOLATED
  {
    SS_UCS4 orig =  glyph.getChar();
    if (orig!=comp && orig != 0)
    {
      setBase (orig);
      for (unsigned int i=0; i<fontVector.size(); i++)
      {
        SFontImpl im = fontVector[i];
        if (im.draw (canvas, pen, m, orig, glyph.isLR(), isSelected, baseOK)) 
        {
          im.width (orig, &baseWidth);
          baseOK = true;
          break;
        }
      }
    }
  }

  /* Try precompositions  */
  const SS_UCS4* fbs = 0;
  if (!baseOK && gsize > 0)
  {
    double* positions = new double[gsize];
    unsigned int* indeces = new unsigned int[gsize];
    CHECK_NEW (positions);
    CHECK_NEW (indeces);
    bool found = false;

    bool overstrike =  (glyph.getShapeArray()==0 
        && !glyph.isYuditLigature() && !glyph.isCluster());


    /*
     * Hack for special Yudit ligatures 
     * Normally clusters are not OVERSTRIKE.
     * MARK Composing Cluster: see bin/cluster/cluster.template 
    */
    if (gsize > 1 && 
        (decomp[1] == 0x309A || decomp[1] == 0x300 || decomp[1] == 0x301))
    {
      overstrike = true;
    }
    else
    {
    }
    int scode = getLigatureScriptCode (comp); 
    if (scode == SD_HANGUL_JAMO 
        || scode == SD_LAO || scode == SD_THAI  || scode == SD_TIBETAN)
    {
      overstrike = true;
    }

    /* shape fallback does not include composing marks */
    fbs = glyph.getShapeFallback();
    if (fbs)
    {
      overstrike = false; /* well, this is overstrike sometimes */
      decomp = fbs; /* even if we can not draw it, this will be displayed */
    }
    /* we can not do overstrike positioning on a shape fallback */
    if (overstrike)
    {
      setBase (decomp[0]);
    }
    else
    {
      setBase (0);
    }

    /* build positions */
    unsigned int index = 0;
    unsigned int i=0;
    unsigned int fsize = fontVector.size();
    double fullsize = 0;
    while (i<fsize) 
    {
      SFontImpl im = fontVector[i];
      currw = 0.0;
      bool used = (index > 0 && decomp[index]==0x200d) 
          ? true: im.width (decomp[index], &currw);
      /* ZWJ and ZWNJ - use fallback if not present.*/
      if (!used && i+1 == fsize && !overstrike)
      {
         SS_Matrix2D sm;
         double sc = fallbackFont.scale();
         sm.scale (fallbackScale * sc, fallbackScale * sc);
         used = true;
         i = fsize;
         currw = fallbackFont.width (sm, decomp[index]);
      }
      /* True Type fonts will need to position 
         non spacing marks *after* moving cursor */
      if (index ==0)
      {
         if (!used || currw==0)
         {
           i++; continue;
         }
         positions[index] = currw; 
         fullsize = currw;
      }
      else if (overstrike) /* may have zero width */
      {
         if (!used)
         {
           i++; continue;
         }
         /* by default don't move caret */
         positions[index] = 0;

         /* 
          * Should be in sync with: SFontTTF::getBaseOffsets
          */
         if (!im.isLeftAligned(decomp[index]))
         {
            positions[index] = positions[0]-currw;
         }
      }
      else
      {
         /* clusters can have 0 width stuff. */
         if (!used)
         {
           i++; continue;
         }
         fullsize += currw;
         positions[index] = fullsize;
      }
      indeces[index] = i;
      i=0;
      index++;
      /* found if all found */
      if (index == gsize)
      {
        baseWidth = fullsize;
        found = true;
        break;
      }
    }
    if (found)
    {
      for (i=0; i<gsize; i++)
      {
        SS_Matrix2D mo = m;
        double translatex = 0.0;
        if (glyph.isLR())
        {
          if (overstrike)
          {
             translatex = (i==0) ? 0.0 : positions[i];
          }
          else
          {
             translatex = (i==0) ? 0.0 : positions[i-1];
          }
        }
        else
        {
          if (overstrike)
          {
             translatex = (i==0) ? 0.0 : positions[i];
          }
          else
          {
             translatex = fullsize - positions[i];
          }
        }
        if (indeces[i] == fsize)
        {
          SS_Matrix2D sd;
          double sc = fallbackFont.scale();
          sd.scale (fallbackScale * sc, fallbackScale * sc);
          sd.translate (0, -fontDescent);
          SS_Matrix2D sm =  m * sd;
          sm.translate (translatex, (double)0.0);
          fallbackFont.draw(canvas, pen, sm, decomp[i]);
        }
        else
        {
          mo.translate (translatex, (double)0.0);
          SFontImpl im = fontVector[indeces[i]];
          im.draw (canvas, pen, mo, decomp[i], glyph.isLR(), isSelected, baseOK);
            // SGC
           baseOK = true; 
        }
      }
    }
    delete[] positions;
    delete[] indeces;
    if (found) 
    {
      baseOK = true;
      /* combining marks are not part of fallback */
      if (fbs==0) ligclust.clear();
    }
  }

  /*
   * If it is shaped and current shape is any fallback 
   * Isolated fallback has been processed already.
   */
  if (!baseOK && glyph.getShapeArray()!=0 && glyph.currentShape != 0) //ISOLATED
  {
    SS_UCS4 orig =  glyph.getChar();
    if (orig!=comp && orig != 0)
    {
      setBase (orig);
      for (unsigned int i=0; i<fontVector.size(); i++)
      {
        SFontImpl im = fontVector[i];
        if (im.draw (canvas, pen, m, orig, glyph.isLR(), isSelected, baseOK)) 
        {
          im.width (orig, &baseWidth);
          baseOK = true;
          break;
        }
      }
    }
  }

  /* Add extra composing characters as overstrike - if possible  */
  if (baseOK) /* we already have set the base with setbase */
  {
    gsize = glyph.compSize();
    decomp = glyph.getCompArray();
    /* add composing marks for composing clusters */
    unsigned int postcomp = 0;
    if (ligclust.size() > 1)
    {
      ligclust.remove (0);
      postcomp = ligclust.size();
      for (unsigned int i=0; i<gsize; i++)
      {
        ligclust.append (decomp[i]);
      } 
      gsize = ligclust.size();
      decomp = ligclust.array();
    }
    for (unsigned int i=0; i<gsize; i++)
    {
      /* Try all fonts on it */
      SS_Matrix2D mc = m;
      for (unsigned int j=0; j<fontVector.size(); j++)
      {
        SFontImpl im = fontVector[j];
        bool used = im.width (decomp[i], &currw);
        if (!used) continue;
        /* FIXME: find a better way */
        /* get these rl scripts right somehow, fonts suppose you go
          visual order...  */

        /* Measure LR: composing from the end. */
        if (!im.isLeftAligned (decomp[i]))
        {
          mc.t0 = m.t0 + baseWidth - currw;
        }
        /* try to play with composing marks in the middle of ligature */
        /* This trick works only if ligature has two base characters */
        else if (i<postcomp && (im.isTTF() || fbs != 0))
        {
          /* RL: move it more to the end */
          if ((decomp[i] >= 0x500 && decomp[i] < 0x900))
          {
            mc.t0 = m.t0 + baseWidth / 2;
          }
        }
        /* Draw it finally... */
        if (im.draw (canvas, pen, mc, decomp[i], glyph.isLR(), isSelected, baseOK)) 
        {
           break;
        }
      }
    }
  
    /* return anyway. 
       Extra composing failures will not be indicated for now. There is one 
       for sure when you apply the composing to a cluster  */
    return;
  }

  /* Draw some last resort font. */
  SS_Matrix2D sd;
  double sc = fallbackFont.scale();
  sd.scale (fallbackScale * sc, fallbackScale * sc);
  sd.translate (0, -fontDescent);
  SS_Matrix2D sm =  m * sd;
  SS_Matrix2D mo = sm;
  SV_UCS4 allchar;
  if (comp != 0)
  {
    allchar.append (comp);
    gsize = glyph.compSize();
    decomp = glyph.getCompArray();
  }
  else
  {
    /* draw everything. CompArray is right after DecompArray */
    gsize += glyph.compSize();
  }
  for (unsigned int k=0; k<gsize; k++)
  {
    allchar.append (decomp[k]);
  }
  gsize = allchar.size();
  decomp = allchar.array();

  int myindex = 0;
  int inc = 1;
  int limit = gsize;
  if (!glyph.isLR())
  {
    myindex = gsize-1;
    inc = -1;
    limit = -1;
  }
  while (myindex!=limit)
  {
    SS_UCS4 chr = decomp[myindex];
    if (getLigatureScriptCode (chr) == SD_AS_LITERAL)
    {
      chr = chr & 0xff;
    }
    fallbackFont.draw(canvas, pen, mo, chr);
    currw = fallbackFont.width (mo, chr);
    mo.translate (currw, (double)0.0);
    myindex = myindex + inc;
  }
  return;
}


/**
 * return the width of the characters
 * @param m is the conversion matrix
 * @param uch is the array containing ucs4 
 * @prama len is the length of the array
 */
 
double
SFont::width (const SGlyph& glyph)
{
  double maxw = 0.0;
  double currw = 0.0;
  if (glyph.isSpecial ())
  {
    SS_Matrix2D m;
    double sc = fallbackFont.scale();
    m.scale (fallbackScale * sc, fallbackScale * sc);
    SFontFB::SIGN sig = mapFB (glyph);
    currw =  fallbackFont.signWidth (m, sig);
    return currw;
  }

  /* try to draw it with all fonts. */
  SS_UCS4 comp =  glyph.getShapedChar();
  unsigned int scriptcode = getLigatureScriptCode (comp);
  if (scriptcode == SD_COMBINING_LIGATURE)
  {
    unsigned int lgsize = getLigatureCluster (comp, 0);
    if (lgsize > 1)
    {
      SS_UCS4* lc = new SS_UCS4[lgsize];
      CHECK_NEW (lc);
      getLigatureCluster (comp, lc);
      comp = lc[0];
      delete[] lc;
    }
  }
  else if (!glyph.isLR() && glyph.isMirrorable())
  {
    comp = glyph.getMirroredChar();
  }

  /* first try the precomposed */
  if (comp != 0 && comp!= 0x200c && comp != 0x200d)
  {
    for (unsigned int i=0; i<fontVector.size(); i++)
    {
      SFontImpl im = fontVector[i];
      if (im.width (comp, &currw))
      {
        if (currw < 0.0) return 1.0;
        return currw;
      }
    }
  }
  /*
   * If it is shaped and current shape is isolated fallback 
   */
  if (glyph.getShapeArray()!=0 && glyph.currentShape == 0) // ISOLATED
  {
    SS_UCS4 orig =  glyph.getChar();
    if (orig!=comp && orig != 0)
    {
      for (unsigned int i=0; i<fontVector.size(); i++)
      {
        SFontImpl im = fontVector[i];
        bool used = im.width (orig, &currw);
        if (used && currw >0.0)
        {
          return currw;
        }
      }
    }
  }

  /* You reach this point if comp did not work */
  unsigned int gsize = glyph.decompSize();
  const SS_UCS4* decomp = glyph.getDecompArray(); 

  if (gsize > 0)
  {
    bool overstrike =  (glyph.getShapeArray() ==0 
        && !glyph.isYuditLigature() && !glyph.isCluster());

    /* hack for special Yudit ligatures */
    /* MARK Composing Cluster: see bin/cluster/cluster.template */
    if (gsize > 1 && 
        (decomp[1] == 0x309A || decomp[1] == 0x300 || decomp[1] == 0x301))
    {
      overstrike = true;
    }
    int scode = getLigatureScriptCode (comp); 
    if (scode == SD_HANGUL_JAMO 
        || scode == SD_LAO || scode == SD_THAI  || scode == SD_TIBETAN)
    {
      overstrike = true;
    }

    const SS_UCS4* fbs = glyph.getShapeFallback();
    if (fbs)
    {
      overstrike = false;
      decomp = fbs; /* even if we can not draw it, this will be displayed */
    }

    unsigned int index = 0;
    unsigned int i=0;
    unsigned int fsize = fontVector.size();
    while (i<fsize)
    {
      SFontImpl im = fontVector[i];
      currw = 0.0; 
      bool used = (index > 0 && decomp[index]==0x200d)
        ? true : im.width (decomp[index], &currw);
      /* can draw fallback in the middle */
      if (!used && i+1 == fsize && !overstrike)
      {
         SS_Matrix2D sm;
         double sc = fallbackFont.scale();
         sm.scale (fallbackScale * sc, fallbackScale * sc);
         used = true;
         i = fsize;
         currw = fallbackFont.width (sm, decomp[index]);
      }
      if (index==0 && (currw==0.0 || !used))
      {
          i++; continue;
      }
       /* clusters also can have 0 width stuff */
      if (!used)
      {
        i++; continue;
      }
      if (overstrike)
      {
        if (index==0) maxw = currw;
      }
      else
      {
        maxw += currw;
      }
      index++;
      i = 0;
      if (index >= gsize)
      {
        if (maxw > 0.0) return maxw;
        break;
      }
    }
  }
  /*
   * If it is shaped and current shape is any fallback 
   * Isolated fallback has been processed already.
   */
  if (glyph.getShapeArray()!=0 && glyph.currentShape != 0) // ISOLATED
  {
    SS_UCS4 orig =  glyph.getChar();
    if (orig!=comp && orig != 0)
    {
      for (unsigned int i=0; i<fontVector.size(); i++)
      {
        SFontImpl im = fontVector[i];
        bool used = im.width (orig, &currw);
        if (used && currw >0.0)
        {
          return currw;
        }
      }
    }
  }

  /* last resort font */
  SS_Matrix2D sm;
  double sc = fallbackFont.scale();
  sm.scale (fallbackScale * sc, fallbackScale * sc);
  maxw  = 0;
  if (comp != 0)
  {
    SS_UCS4 chr = comp;
    if (getLigatureScriptCode (chr) == SD_AS_LITERAL)
    {
      chr = chr & 0xff;
    }
    maxw = fallbackFont.width (sm, chr);
    gsize = glyph.compSize();
    decomp = glyph.getCompArray();
  }
  else
  {
    /* draw everything */
    gsize += glyph.compSize();
  }
  for (unsigned int i=0; i< gsize; i++)
  {
    SS_UCS4 chr = decomp[i];
    if (getLigatureScriptCode (chr) == SD_AS_LITERAL)
    {
      chr = chr & 0xff;
    }
    maxw = maxw + fallbackFont.width (sm, chr);
  }
  return maxw;
}

/**
 * return the overall width
 */
double
SFont::width () const
{
  return fontWidth;
}

/**
 * return the overall ascent
 */
double
SFont::ascent () const
{
  return fontAscent;
}


/**
 * return the overall descent
 */
double
SFont::descent () const
{
  return fontDescent;
}

/**
 * return the overall gap
 */
double
SFont::gap () const
{
  return fontGap;
}
