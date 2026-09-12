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


/* 0xffffffff */
static int SD_TTF_NAN=-1;
 
#include "swindow/SFontTTF.h"
#include "swindow/SScriptProcessor.h"
#include "swindow/STTables.h"

#include "stoolkit/SStringVector.h"
#include "stoolkit/SExcept.h"
#include "stoolkit/SUniMap.h"
#include "stoolkit/SUtil.h"
#include "stoolkit/SCluster.h"
#include "stoolkit/SCharClass.h"
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


/*
 * These glyphs should not exists khmm.. I said these glyphs should not exist.
 * Unicode magic :)
 */
#define SD_G_INDIC_ZWNJ 0xfffe
#define SD_G_INDIC_ZWJ 0xffff



/**
 * This is a hash width key platform '~' encoding 
 * and value SUnimap.
 */
static const SString SS_TB_NAME("name");
static const SString SS_TB_HEAD("head");
static const SString SS_TB_HHEA("hhea");
static const SString SS_TB_POST("post");
static const SString SS_TB_GLYF("glyf");
static const SString SS_TB_CMAP("cmap");
static const SString SS_TB_KERN("kern");
static const SString SS_TB_MAXP("maxp");
static const SString SS_TB_HTMX("hmtx");
static const SString SS_TB_LOCA("loca");
static const SString SS_TB_OS2("OS/2");
static const SString SS_TB_CFF("CFF ");
static const SString SS_TB_CFF2("CFF2");
static const SString SS_TB_COLR("COLR");
static const SString SS_TB_CPAL("CPAL");

/*
static const long SS_TN_NOTICE=0;
static const long SS_TN_FAMILY=1;
static const long SS_TN_WEIGHT=2;
static const long SS_TN_X3=3;
*/

static const long SS_TN_FULLNAME=4;
//static const long SS_TN_VERSION=5;
static const long SS_TN_FONTNAME=6;
//static const long SS_TN_X7=7;
static const long SS_TN_MAX=8;



static void debugChars (const char* msg, 
  const SS_GlyphIndex* gchars, unsigned int len);
static SS_GlyphIndex findGlyph0 (TTF_CMAP_FMT0* encoding0, SS_UCS4 ucs4);
static SS_GlyphIndex findGlyph4 (TTF_CMAP_FMT4* encoding4, SS_UCS4 ucs4);
static SS_GlyphIndex findGlyph12 (TTF_CMAP_FMT12* encoding12, SS_UCS4 ucs4);
static SS_GlyphIndex findGlyph2 (TTF_CMAP_FMT2* encoding2, SS_UCS4 ucs4);

static double f2dot14 (short x);
static void moveto (SCanvas* canvas, const SS_Matrix2D &m,
  SD_SHORT _x, SD_SHORT _y);
static void lineto (SCanvas* canvas, const SS_Matrix2D &m,
  SD_SHORT _x, SD_SHORT _y);
static void cureveto (SCanvas* canvas, const SS_Matrix2D &m,
  SD_SHORT _x0, SD_SHORT _y0,
  SD_SHORT _x1, SD_SHORT _y1,
  SD_SHORT _x2, SD_SHORT _y2);

/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 * Many parts of this file are originally written by Andrew Weeks.
 */

SS_UCS4 SFontTTF::setBaseCharacter = 0;

/**
 * Initialize a TTF
 * @param name is the font file
 */
SFontTTF::SFontTTF (const SFile& _file,  const SString& _fontencoding) 
  : file (_file)
{
  ok = true;
  fontencoding = _fontencoding;
  isEmoji = false;
  if (fontencoding == "emoji") {
    isEmoji = true;
    fontencoding.clear();
  }
  hardWire = SS_NONE;
  defaultGlyph = 0;
  baseGlyph = 0;
  cffFont = 0;
}

SFontTTF::~SFontTTF ()
{
}

bool
SFontTTF::isOK()
{
  if (!ok) return false;

  /* initialize */
  if (name.size()==0)
  {
    name = file.getName();
//fprintf (stdout, "%*.*s %*.*s\n", SSARGS(name), SSARGS(fontencoding));
    if (fontencoding == "mslvt")
    {
      hardWire = SS_MSLVT;
      fontencoding.clear();
    }
    else if (fontencoding == "nojamo")
    {
      hardWire = SS_NOJAMO;
      fontencoding.clear();
    }
    else if (fontencoding == "jamo")
    {
      hardWire = SS_JAMO;
      fontencoding.clear();
    }
    /* Experimental filters for whole ranges */
    else if (fontencoding == "indic")
    {
      hardWire = SS_INDIC;
      fontencoding.clear();
    }
    else if (fontencoding == "deva")
    {
      hardWire = SS_DEVANAGARI;
      fontencoding.clear();
    }
    else if (fontencoding == "beng")
    {
      hardWire = SS_BENGALI;
      fontencoding.clear();
    }
    else if (fontencoding == "guru")
    {
      hardWire = SS_GURMUKHI;
      fontencoding.clear();
    }
    else if (fontencoding == "gujr")
    {
      hardWire = SS_GUJARATI;
      fontencoding.clear();
    }
    else if (fontencoding == "orya")
    {
      hardWire = SS_ORIYA;
      fontencoding.clear();
    }
    else if (fontencoding == "taml")
    {
      hardWire = SS_TAMIL;
      fontencoding.clear();
    }
    else if (fontencoding == "telu")
    {
      hardWire = SS_TELUGU;
      fontencoding.clear();
    }
    else if (fontencoding == "knda")
    {
      hardWire = SS_KANNADA;
      fontencoding.clear();
    }
    else if (fontencoding == "mlym")
    {
      hardWire = SS_MALAYALAM;
      fontencoding.clear();
    }
    else if (fontencoding == "sinh")
    {
      hardWire = SS_SINHALA;
      fontencoding.clear();
    }
    else if (fontencoding == "thai")
    {
      hardWire = SS_THAI;
      fontencoding.clear();
    }
    else if (fontencoding == "lao")
    {
      hardWire = SS_LAO;
      fontencoding.clear();
    }
    else if (fontencoding == "tibt")
    {
      hardWire = SS_TIBETAN;
      fontencoding.clear();
    }
    else
    {
      SString n = name; n.lower();
      /* only Ogulim may work. forget any prefix.*/
      if (n.match("*ogulim.ttf"))
      {
        hardWire = SS_MSLVT;
        fontencoding.clear();
      }
    }
    if (file.size() < 0)
    {
      ok = false;
    }
    else
    {
      image = file.getFileImage();
      ok = image.size()>0 && init();
    }
  }
  return ok;
}

/**
 * initialize all numbers.
 * return false if something is wrong with this font.
 */
bool
SFontTTF::init ()
{
  cffFont = 0;
  TTF_DIRECTORY* directory = (TTF_DIRECTORY *) image.array();
  if (ntohl (directory->sfntVersion) == 0x4f54544f) {
     cffFont = new SFontCFF();
  }
  else if (ntohl (directory->sfntVersion) != 0x00010000)
  {
    fprintf (stderr, "SFontTTF: BAD TTF file [%*.*s] sfntVersion=%x\n", 
        SSARGS(name), ntohl (directory->sfntVersion));
    return false;
  }
  TTF_DIR_ENTRY* dir_entry = &(directory->list);
  
  char tag[5];
  unsigned int i;
  for (i=0; i < (unsigned short)ntohs(directory->numTables); i++)
  {
    for (unsigned int j=0; j<4; j++)
    {
      tag[j] = dir_entry->tag[j];
    }
    tag[4] = 0;
    tables.put (tag, image.array() + ntohl (dir_entry->offset));
    if (memcmp(tag, "EBDT", 4)==0 || memcmp(tag, "EBLC", 4)==0
        || memcmp(tag, "EBSC", 4)==0)
    {
      //fprintf (stderr, "SFontTTF info: TTF file [%*.*s] contains bitmaps.\n",
       //  SSARGS(name));
    }
    dir_entry++;
  }
  if (!processName())
  {
     return false;
  }
  if (!checkTables())
  {
     return false;
  }
  broken = false;

  TTF_CMAP* cmap_table = (TTF_CMAP*) tables[SS_TB_CMAP];
  int num_tables = ntohs(cmap_table->numberOfEncodingTables);

  TTF_OS2* os2table = (TTF_OS2*) tables[SS_TB_OS2];
  if (os2table)
  {
    //defaultGlyph = ntohs(os2table->usDefaultChar);
    //unsigned int rangel = ntohl (os2table->ulUnicodeRange1);
    //unsigned int rangeh = ntohl (os2table->ulUnicodeRange2);
    //fprintf (stderr, "%*.*s defaultGlyph=%u %u %u\n", 
     //   SSARGS(name), defaultGlyph, rangel, rangeh);
  }

  /* We go through all the tables and choose the bes table */
  int bestType = 0;
  if (fontencoding.size())
  {
    charEncoder = SUniMap(fontencoding);
    if (!charEncoder.isOK())
    {
      fprintf (stderr, "SFontTTF: umap '%*.*s' not found for '%*.*s'.\n",
        SSARGS(fontencoding), SSARGS(name));
    }
  }
  charEncoderTable = (unsigned int) num_tables; 
  for (i=0; i < (unsigned int) num_tables; i++)
  {
    TTF_CMAP_ENTRY* table_entry = &(cmap_table->encodingTable[i]);
    int offset = ntohl(table_entry->offset);
    TTF_CMAP_FMT4* encoding4 = (TTF_CMAP_FMT4 *) ((SD_BYTE *)cmap_table + offset);
    int format = ntohs(encoding4->format);
    int platform = ntohs(table_entry->platformID);
    int encoding_id = ntohs(table_entry->encodingID);
    /**
     * All 
     *  platform == TT_PLAT_ID_MICROSOFT(3)
     *  encoding_id == TT_ENC_ID_ISO_10646 (1)
     * will have TTF_CMAP_FMT4 (format==4
     * character map. Others *might* have. 
     *
     * TODO: support more cmap formats like 32 bit unicode.
     *  currently 32 bit unicode is done through external map.
     */
    if (format != 4) continue;

    /* could not find any good table */
    if (charEncoderTable ==(unsigned int) num_tables)
    {
      charEncoderTable = i;
    }
/*
    fprintf (stderr, "%*.*s platform=%d encoding=%d at %u\n", SSARGS(name),
       platform, encoding_id, i);
*/
    switch (platform)
    {
    case TT_PLAT_ID_MICROSOFT:
      switch (encoding_id)
      { 
      case TT_ENC_ID_MS_SYMBOL:
        break;
      case TT_ENC_ID_MS_UNICODE:
        bestType = 9; /* these mystic numbers are my scores */
        charEncoderTable = i;
        break;
      case TT_ENC_ID_MS_SURROGATES:
        bestType = 10; /* these mystic numbers are my scores */
        charEncoderTable = i;
        break;
      case TT_ENC_ID_MS_SHIFT_JIS:
      case TT_ENC_ID_MS_BIG5:
      case TT_ENC_ID_MS_RPC:
      case TT_ENC_ID_MS_WANSUNG:
      case TT_ENC_ID_MS_JOHAB:
      default:
         break;
     }
     break;
    case TT_PLAT_ID_ISO:
      switch (encoding_id)
      { 
      case TT_ENC_ID_ANY:
        break;
      case TT_ENC_ID_ISO_ASCII:
        if (bestType < 2) 
        {
           bestType = 2;
           charEncoderTable = i;
        }
        break;
      case TT_ENC_ID_ISO_10646:
        if (bestType < 8) 
        {
           bestType = 8;
           charEncoderTable = i;
        }
        break;
      case TT_ENC_ID_ISO_8859_1:
        if (bestType < 4) 
        {
           bestType = 4;
           charEncoderTable = i;
        }
        break;
      default:
        break;
     }
     break;
    case TT_PLAT_ID_APPLE:
      switch (encoding_id)
      { 
      case TT_ENC_ID_APPLE_DEFAULT:
        break;
      case TT_ENC_ID_APPLE_UNICODE_1_1:
      case TT_ENC_ID_APPLE_ISO_10646:
      case TT_ENC_ID_APPLE_UNICODE_2_0:
        if (bestType < 7) 
        {
           bestType = 8;
           charEncoderTable = i;
        }
      default:
         break;
     }
     break;
    case TT_PLAT_ID_MACINTOSH:
      switch (encoding_id)
      { 
      case TT_ENC_ID_MAC_ROMAN:
      /* a lot of other encodings missing */
      default:
       break;
     }
    default:
      break;
    }
  }
  /* look at all tables SGC */
  if (fontencoding.size()!=0) charEncoderTable =(unsigned int) num_tables;
  //fprintf (stderr, "SGC %*.*s besttype = %d table=%d count=%d\n", 
  //     SSARGS (name), bestType, charEncoderTable, num_tables);
  //SString chk = name; chk.lower();
  return true;
}


static int anonym = 1;
/**
 * Process the name table
 */
bool
SFontTTF::processName ()
{
  TTF_NAME*  name_table = (TTF_NAME*) tables[SS_TB_NAME];
  if (name_table==0)
  {
    getName (SS_TN_FONTNAME, name.array(), name.size());
    fprintf (stderr, "SFontTTF: No name fields in %*.*s\n", SSARGS(name));
#if 0
    fprintf (stderr, "SFontTTF: records:[");
    for (unsigned int i=0; i<tables.size(); i++)
    {
      for (unsigned int j=0; j<tables.size(i); j++)
      {
        SString key = tables.key(i, j);
        fprintf (stderr, " %*.*s", SSARGS(key));
        
      }
    }
    fprintf (stderr, " ]\n");
#endif
    return false;
  }

  TTF_NAME_REC* name_record = &(name_table->nameRecords);
  char* string_area = (char *)name_table + ntohs(name_table->offset);
    
  int found=0;
  int i;
  for (i=0; i < ntohs (name_table->numberOfNameRecords); i++) 
  {
    short platform = ntohs(name_record->platformID);
    if (platform == 3)
    {
      found = 1;
      short len = ntohs(name_record->stringLength);
      short strOffset = ntohs(name_record->stringOffset);
      long nameId = ntohs(name_record->nameID);
      if (nameId < SS_TN_MAX)
      {
        getName (nameId, &string_area[strOffset], len);
      }
    }
    name_record++;
  }

  name_record = &(name_table->nameRecords);
  if (!found) for (i=0; i < ntohs(name_table->numberOfNameRecords); i++) 
  {
   short platform = ntohs(name_record->platformID); if (platform ==1)
   {
      found = 1;
      short len = ntohs(name_record->stringLength);
      short strOffset = ntohs(name_record->stringOffset);
      long nameId = ntohs(name_record->nameID);
      if (nameId < SS_TN_MAX)
      {
        getName (nameId, &string_area[strOffset], len);
      }
    }
    name_record++;
  }
  if (!found)
  {
    fprintf (stderr, "SFontTTF: BAD Name fields in %*.*s\n", SSARGS(name));
    return false;
  }

  if (names.get (SS_TN_FONTNAME) == 0 || names[SS_TN_FONTNAME].size() == 0)
  {
        if (names.get(SS_TN_FULLNAME) == 0) 
        {
            char an[64];
            snprintf (an, 63, "anonym%04d", anonym++);
            getName (SS_TN_FONTNAME, an, strlen(an));
#if 0
            fprintf (stderr, "SFontTTF: Assigned %s for %*.*s\n", 
                an, SSARGS(name));
#endif
        }
        else 
        {
            getName (SS_TN_FONTNAME, names[SS_TN_FULLNAME].array(),
                names[SS_TN_FULLNAME].size());
        }
  }
  return true;
}

/**
 * put the string from str into names. 
 * @param id is SS_TN_ something.
 * @param str is the input string
 * @param len is the size of the string
 */
void
SFontTTF::getName (long id, const char* str, int len)
{
  SString s;
  for (int i=0; i<len; i++)
  {
    if (str[i] == 0) continue;
    if (id==SS_TN_FONTNAME)
    {
      if (isalnum(str[i]))
      {
        s.append (str[i]);
      }
      else
      {
        s.append ((char)'_');
      }
    }
    /* This is to make postscript files clean */
    else switch (str[i])
    {
    case '(':
      s.append ((char)'[');
      break;
    case ')':
      s.append ((char)']');
      break;
    default:
      s.append (str[i]);
    }
  }
  names.put (id, s);
}

/**
 * Do a sanity check.all tables
 */
bool
SFontTTF::checkTables ()
{

  if (tables[SS_TB_HEAD] == 0)
  {
    fprintf (stderr, "SFontTTF: BAD head table in %*.*s\n", SSARGS(name));
    return false;
  }
  TTF_HEAD* head_table = (TTF_HEAD*) tables[SS_TB_HEAD];
  longOffsets = ntohs (head_table->indexToLocFormat);

  if (longOffsets != 0 && longOffsets != 1)
  {
    fprintf (stderr, "SFontTTF: BAD TTF file [%*.*s] - indexToLocFormat.\n",
       SSARGS(name));
    return false;
  }

  if (tables[SS_TB_HHEA] == 0)
  {
    fprintf (stderr, "SFontTTF: BAD hhea table in %*.*s\n", SSARGS(name));
    return false;
  }
  if (cffFont) {
    if (tables[SS_TB_CFF]) {
        if (!cffFont->initWithCFF((SD_BYTE*)tables[SS_TB_CFF])) {
            fprintf (stderr, "SFontCFF: initCFF failed %*.*s\n", SSARGS(name));
            return false;
            
        }
    } else if ( tables[SS_TB_CFF2]) {
        if (!cffFont->initWithCFF2((SD_BYTE*)tables[SS_TB_CFF2])) {
            fprintf (stderr, "SFontCFF: initCFF2 failed %*.*s\n", SSARGS(name));
            return false;
        }
    } else {
        fprintf (stderr, "SFontTTF: has no CFF table %*.*s\n", SSARGS(name));
        return false;
    } 
  } else {
    if (tables[SS_TB_GLYF] == 0)
    {
      fprintf (stderr, "SFontTTF: BAD glyf table in %*.*s\n", SSARGS(name));
      return false;
    }
    if (tables[SS_TB_LOCA] == 0)
    {
      fprintf (stderr, "SFontTTF: BAD loca table in %*.*s\n", SSARGS(name));
      return false;
    }
  }
#if 1
  if (tables[SS_TB_COLR] != 0) {
      fprintf (stderr, "SFontTTF: COLR font %*.*s\n", SSARGS(name));
  }
  if (tables[SS_TB_CPAL] != 0) {
      fprintf (stderr, "SFontTTF: CPAL font %*.*s\n", SSARGS(name));
  }
#endif
  if (tables[SS_TB_CMAP] == 0)
  {
    fprintf (stderr, "SFontTTF: BAD cmap table in %*.*s\n", SSARGS(name));
    return false;
  }
  if (tables[SS_TB_HTMX] == 0)
  {
    fprintf (stderr, "SFontTTF: BAD htmx table in %*.*s\n", SSARGS(name));
    return false;
  }
  TTF_POST_HEAD*  post_table = (TTF_POST_HEAD*) tables[SS_TB_POST];
  if (post_table == 0)
  {
    fprintf (stderr, "SFontTTF: missing post table in %*.*s. Using defaults\n",
         SSARGS(name));
    italicAngle = 0.0;
    underlineThickness = 100;
    underlinePosition = 0.0;
    isFixedPitch = 0.0;
  }
  else
  {
    italicAngle = (double) (ntohs(post_table->italicAngle.upper)) +
      (ntohs(post_table->italicAngle.lower) / 65536.0);
    underlineThickness = (double)ntohs(post_table->underlineThickness);
    underlinePosition = (double)ntohs(post_table->underlinePosition);
    isFixedPitch = (ntohl(post_table->isFixedPitch))? true : false;
  }
  TTF_HHEA* hhea_table = (TTF_HHEA*) tables[SS_TB_HHEA];

  lineGap = (double) ((short)htons (hhea_table->lineGap));
  charWidth = (double) ((short)(ntohs(head_table->xMax)+ ntohs(head_table->xMin)));
  charAscent = (double) ((short)htons (hhea_table->ascender));
  /* Be aware charDescent is negative! */
  charDescent = (double) ((short)htons (hhea_table->descender));

  /* kairali-S: dscale=1000 charAscent=298 charDescent=-202 
        dscale_factor dscale/(charAscent-charDescent) */

  /* charDescent is negative - take charAscent aonly to determine size */
  double charheight = charAscent;
  if (charheight < 1) charheight = 1;
  scaleFactor = 1.0 / charheight;
/*
  short unitsPerEM  = ntohs (head_table->unitsPerEm);
  fprintf (stderr, "font=%*.*s charheight=%g unitsPerEM=%d\n", 
      SSARGS(name), charheight, unitsPerEM);
*/
  return true;
}

/**
 * set the base character for better glyph positioning
 * @param base is the base character relative to which 
 * we will position all of out composing marks.
 * When the draw routine is called with non-base 
 * character, then the position will be at the end
 * of the glyph visually, except for U+0500..U+0900 
 * composing marks, where matrix will be set at the 
 * beginning of the base glyph, viaually
 */
void
SFontTTF::setBase(SS_UCS4 base)
{
  setBaseCharacter = base;
  /* we do this font unicode fonts only now */
  
}

/**
 * Get the x,y offset for better positioning of diacritical marks.
  * This routine is supposed to be
 */
void
SFontTTF::getBaseOffsets (const SS_Matrix2D& m, 
  SS_UCS4 _uch, double* offx, double* offy)
{
  *offx = 0.0;
  *offy = 0.0;
  if (!isOK() || setBaseCharacter==0 
      || setBaseCharacter==_uch) return;

  if (setBaseCharacter == baseCharacter && baseGlyph==0)
  {
      return;
  }

  if (setBaseCharacter != baseCharacter)
  {
    baseCharacter = setBaseCharacter;
    if (fontencoding.size()!=0 || baseCharacter >= 0x80000000 
       || !isOK() || hardWire == SS_MSLVT || hardWire == SS_NOJAMO)
    {
      baseGlyph = 0;
      return;
    }
    baseGlyph = findGlyph (baseCharacter);
    if (baseGlyph==0) 
    {
      return;
    }
//fprintf (stderr, "baseCharacter=%04X glyph=%04X\n", baseCharacter, baseGlyph);
    /* get the width of the base char */  
    baseWidth = getGlyphWidth (baseGlyph);
  }
  SString key ((char*)&baseGlyph, sizeof (SS_GlyphIndex));
  key.append (SString ((char*)&_uch, sizeof (SS_UCS4)));

  int cxy = mark2Base.get (key);
  if (cxy == SD_TTF_NAN) return;

  /* we had it not found. SGC */
  int cx = cxy & 0xffff;
  if (cx > 0x7fff) cx -= 0x10000 ;
  int cy = (cxy >> 16) & 0xffff;
  if (cy > 0x7fff) cy -= 0x10000;

  if (cx != 0 || cy != 0)
  {
    *offx =  cx * m.x0;
    *offy =  cy * m.y1;
    return;
  }
  SS_GlyphIndex gi = findGlyph(_uch);
  if (gi==0)
  {
    mark2Base.put (key, SD_TTF_NAN);
    return;
  }
  getOTFMarkToBase (baseGlyph, gi, &cx, &cy);
  if (cx==0 && cy==0)
  {
    mark2Base.put (key, SD_TTF_NAN);
    return;
  }
  /* In sync with SFont.cpp - we need to undo fallback positioning  */
  if (!isLeftAligned(_uch))
  {
    cx -= (baseWidth - getGlyphWidth (gi));
  }
  cxy = (cy << 16) & 0xffff0000;
  cxy = cxy | (cx & 0xffff);
  mark2Base.put (key, cxy);

  *offx =  (double)cx * m.x0;
  *offy =  (double)cy * m.y1;
  return;
}

/**
 * Get the unadjusted width of the glyph
 */
int 
SFontTTF::getGlyphWidth (SS_GlyphIndex glyph)
{
    TTF_HHEA* hhea_table = (TTF_HHEA*) tables[SS_TB_HHEA];
    LONGHORMETRIC* hmtx_entry = (LONGHORMETRIC*) tables[SS_TB_HTMX];
    int n_hmetrics = ntohs(hhea_table->numberOfHMetrics);
    short _bw;
    /* left side bearing is grossly ignored */
    if (glyph >= n_hmetrics)
    {
      _bw = ntohs (hmtx_entry[n_hmetrics-1].advanceWidth);
    }
    else
    {
      _bw = ntohs(hmtx_entry[glyph].advanceWidth);
    }
    int w = (int) ((_bw>0) ? _bw : -_bw);
    return w;
}

/**
 * Draw a single unicode character on canvas using the pen.
 * before calling this, you should call a newpath and
 * after calling this you may want to call fill.
 * @param canvas is the canvas to draw to
 * @param m is the transformation matrix.
 * @param uch is the unicode character
 * @param len is the length of unicode array
 * @return true if drawn
 */
bool
SFontTTF::draw (SCanvas* canvas, const SS_Matrix2D& matrix, SS_UCS4 _uch, 
  bool isLRContext)
{
  if (!isOK()) return false;
  SV_GlyphIndex gi;
  if (!findGlyphs (_uch, &gi)) return false;
  /* This makes things a bit faster */
  if (gi.size()==1)
  {
    SS_GlyphIndex g = gi[0];
    if (g == SD_G_INDIC_ZWJ || g== SD_G_INDIC_ZWNJ) return true;
    drawGlyph (canvas, matrix, g);
    return true;
  }

  SS_Matrix2D mo = matrix;

  /* a cluster can defined positions to fine adjust */
  const SV_INT *positions = mark2BaseList.get (
        SString((char*) &_uch, sizeof (SS_UCS4)));

  /* do we have fine-grained positions ?*/
  if (positions!= 0 && positions->size() >= gi.size())
  {
    mo = matrix;
    const SS_INT* posarray = positions->array();
    for (unsigned int i=0; i<gi.size(); i++)
    {
       // first is 0, or width.
      int xydiff = posarray[i];
      int xdiff = xydiff & 0xffff;
      if (xdiff > 0x7fff) xdiff -= 0x10000 ;
      int ydiff = (xydiff >> 16) & 0xffff;
      if (ydiff > 0x7fff) ydiff -= 0x10000;
#ifdef DEBUG_POSITION 
      fprintf (stderr, "%u->Positions[%u]=%d,%d\n", _uch, gi[i], xdiff, ydiff);
#endif /* DEBUG_LIGATURE */
      /* we have absolute positions */
      mo.t0 = matrix.t0 + matrix.x0 * xdiff;
      mo.t1 = matrix.t1 + matrix.y1 * ydiff;
      /* draw the glyph at this position */
      SS_GlyphIndex g = gi[i];
      if (g != SD_G_INDIC_ZWJ && g != SD_G_INDIC_ZWNJ)
      {
        drawGlyph (canvas, mo, g);
      }
    }
  }
  else
  { 
    for (unsigned int i=0; i<gi.size(); i++)
    {
      bool isRovas = getLigatureScriptCode (_uch) == SD_ROVASIRAS
           || getLigatureScriptCode (_uch) == SD_PUA_ROVAS;
      // For failsafe we preserve old behaviour for non SD_ROVASIRAS
      SS_GlyphIndex g = 
         (!isRovas  
          || (isLRContext && mo.x0 >= 0.0)
          || (!isLRContext && mo.x0 < 0.0)) ? gi[i] : gi[gi.size()-i-1];

      if (g != SD_G_INDIC_ZWJ && g != SD_G_INDIC_ZWNJ)drawGlyph (canvas, mo, g);
      double nwidth = widthGlyph (mo, g);
      if (nwidth < 0) nwidth = -nwidth;
      // For failsafe we preserve old behaviour for non SD_ROVASIRAS
      // Mirroring (mo.x0 < 0) will reverse the glyph order.
      if (!isRovas || mo.x0 >= 0.0)
      {
        mo.translate (nwidth, 0.0);
      }
      else
      {
        mo.translate (-nwidth, 0.0);
      }
    }
  }
  return true;
}

/**
 * Return the advacnce width of the glyphs
 * The value is the value that is multipied with matrix.
 * @param m is the transformation matrix.
 * @param uch is the unicode character
 * @param len is the length of unicode array
 * @param used will show how many characters were used in uch
 * @return the calibrated advace width, if with_ is passed and 
 *  true if it exists.
 */
bool
SFontTTF::width (const SS_Matrix2D& m, SS_UCS4 _uch, double* width_)
{
  if (width_) *width_ = 0.0;
  if (!isOK()) return false;

  /* a cluster can define positions to fine adjust */
  SV_GlyphIndex gi;
  if (!findGlyphs (_uch, &gi)) return false;

  if (_uch > 0x7fffffff)
  {
    const SV_INT* positions = mark2BaseList.get (
          SString((char*) &_uch, sizeof (SS_UCS4)));

    /* do we have fine-grained positions ?*/
    if (positions!= 0 && positions->size() > gi.size())
    {
      const SS_INT* arr = positions->array();
      if (width_)
      {
        int wid = arr[positions->size()-1];
        *width_ =  double (wid) * m.x0;
      }
      return true;
    }
  }
  if (!width_) return true;
  /**
   * Multiple glyphs draw on top of each other. 
   */
  double max = 0;
  /* We draw one after the other.*/
  for (unsigned int i=0; i<gi.size(); i++)
  {
    /* do not draw them */
    double nwidth = widthGlyph (m, gi[i]);
    if (nwidth < 0) nwidth = -nwidth;
    max += nwidth;
  }
  *width_ = max;
  return true;
}

/**
 * \brief Try to make a fuzzy guess if we need to align the diacritics to
 *    the left or to the right.
 * left aligned marks will be rendered this way:
 *    x----basewith----x
 *         x-markwidth-x 
 * right aligned marks will be rendered this way:
 *    x----basewith----x
 *    x-markwidth-x 
 * There are no docs available that this is the right way to do.
 * This is purely guesswork - most fonts will have negative bearing
 * for an overhang.
 */
bool
SFontTTF::isLeftAligned (SS_UCS4 c) const
{
  if (c > 0x7ffffff) return false;
  if (c == 0x0c55) return false;

  SS_GlyphIndex glyphno = ((SFontTTF*)this)->findGlyph (c);
  if (glyphno == 0) return false;
  if (glyphno == SD_G_INDIC_ZWJ) return false;
  if (glyphno == SD_G_INDIC_ZWNJ) return false;

  int lsb = getLeftSideBearing (glyphno);
  return (lsb >= 0);
}

/**
 * Find out the width, knowing the local glyph number
 * @param m is the transformation matrix
 * @param glyphno is the local glyph index in the glyph table.
 */
double
SFontTTF::widthGlyph (const SS_Matrix2D& m, SS_GlyphIndex glyphno)
{
  if (!isOK()) return 0.0;
  if (glyphno == SD_G_INDIC_ZWJ) return 0.0;
  if (glyphno == SD_G_INDIC_ZWNJ) return 0.0;

  SString key ((char*)&glyphno, sizeof (SS_GlyphIndex));
  int cw = char2Width.get (key);
  if (cw == SD_TTF_NAN) return 0.0;
  if (cw != 0) return (double)cw * m.x0;

  TTF_HHEA* hhea_table = (TTF_HHEA*) tables[SS_TB_HHEA];
  LONGHORMETRIC* hmtx_entry = (LONGHORMETRIC*) tables[SS_TB_HTMX];

  int n_hmetrics = ntohs(hhea_table->numberOfHMetrics);
  //SD_FWORD* lsblist = (SD_FWORD *) &hmtx_entry[n_hmetrics];
  
  unsigned short w;

  /* left side bearing is grossly ignored */
  if (glyphno >= n_hmetrics)
  {
    /* get the last one */
    w = ntohs (hmtx_entry[n_hmetrics-1].advanceWidth);
  }
  else
  {
    w = ntohs(hmtx_entry[glyphno].advanceWidth);
  }
  int wi = (int) w;
  /* replace 0.0 with SD_NAN */
  if (wi==0)
  {
    char2Width.put (key, SD_TTF_NAN);
  }
  else 
  {
    char2Width.put (key, (int)w);
  }
  return double (wi) * m.x0;
}

/**
 * \brief Find out raw, unscaled width of glyph.
 * \glyphno is th glyph 
 * \return a with that can be negative.
 */
int
SFontTTF::getWidth (SS_GlyphIndex glyphno)
{
  if (!isOK()) return 0;
  if (glyphno == SD_G_INDIC_ZWJ) return 0;
  if (glyphno == SD_G_INDIC_ZWNJ) return 0;

  SString key ((char*)&glyphno, sizeof (SS_GlyphIndex));

  TTF_HHEA* hhea_table = (TTF_HHEA*) tables[SS_TB_HHEA];
  LONGHORMETRIC* hmtx_entry = (LONGHORMETRIC*) tables[SS_TB_HTMX];
  unsigned short n_hmetrics = ntohs(hhea_table->numberOfHMetrics);
  unsigned short w;
  /* left side bearing is grossly ignored */
  if (glyphno >= n_hmetrics)
  {
    /* get the last one */
    w = ntohs (hmtx_entry[n_hmetrics-1].advanceWidth);
  }
  else
  {
    w = ntohs(hmtx_entry[glyphno].advanceWidth);
  }
  int wi = (int) w;
  int lsb = getLeftSideBearing (glyphno);
  if (lsb < 0) return -wi;
  return wi;
}

/**
 * \brief Find out raw, unscaled left-side bearing.
 * \glyphno is th glyph 
 * \return a with that can be negative.
 */
int
SFontTTF::getLeftSideBearing (SS_GlyphIndex glyphno) const
{
  if (glyphno == SD_G_INDIC_ZWJ) return 0;
  if (glyphno == SD_G_INDIC_ZWNJ) return 0;

  SString key ((char*)&glyphno, sizeof (SS_GlyphIndex));

  TTF_HHEA* hhea_table = (TTF_HHEA*) tables[SS_TB_HHEA];
  LONGHORMETRIC* hmtx_entry = (LONGHORMETRIC*) tables[SS_TB_HTMX];

  if (hmtx_entry == 0 || hhea_table==0) return 0;
  TTF_MAXP* maxp_table = (TTF_MAXP*) tables[SS_TB_MAXP];

  unsigned short numg = (maxp_table) 
     ? htons (maxp_table->numGlyphs) : 0;

  unsigned short n_hmetrics = ntohs(hhea_table->numberOfHMetrics);
  short lsb;
  if (glyphno >= n_hmetrics)
  {
    if (numg == 0)
    {
      lsb = ntohs (hmtx_entry[n_hmetrics-1].lsb);
    }
    else
    {
      short* arr =  (short*) &hmtx_entry[n_hmetrics];
      lsb = htons (arr[glyphno-n_hmetrics]);
    }
  }
  else
  {
    lsb = ntohs(hmtx_entry[glyphno].lsb);
  }
  return (int) lsb;
}

/*!
 * \brief Get the raw (unscaled) bounding box.
 * \return true if such a box exists.
 */
bool
SFontTTF::getBBOX (SS_GlyphIndex glyphno, 
  int* xMin, int* yMin, int* xMax, int* yMax) const
{
  if (glyphno == SD_G_INDIC_ZWJ) return false;
  if (glyphno == SD_G_INDIC_ZWNJ) return false;


  if (cffFont) {
    return cffFont->getBBOXCFF(glyphno, xMin, yMin, xMax, yMax);
  }
  TTF_GLYF* gtable;
  int len =0;

  SD_BYTE* gstart = (SD_BYTE *) tables[SS_TB_GLYF];
  if (gstart == 0) return false;
  if (longOffsets)
  {
      SD_ULONG* lloca = (SD_ULONG *) tables[SS_TB_LOCA];
      if (lloca == 0) return false;
      unsigned int offs1 = ntohl (lloca[glyphno]);
      unsigned int offs2 = ntohl (lloca[glyphno+1]);
      gtable = (TTF_GLYF *) ((char*)gstart + offs1);
      len = offs2-offs1;
  }
  else
  {
      SD_USHORT* sloca = (SD_USHORT *) tables[SS_TB_LOCA];
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
  
  short xmin =  ntohs (gtable->xMin);
  short xmax =  ntohs (gtable->xMax);
  short ymin =  ntohs (gtable->yMin);
  short ymax =  ntohs (gtable->yMax);
  *xMin = xmin;
  *yMin = ymin;
  *xMax = xmax;
  *yMax = ymax;
  if (kludge) delete kludge;
  return true;
}

/**
 * Return the calibrated ascent
 * @param m is the transformation matrix.
 */
double
SFontTTF::ascent (const SS_Matrix2D& m)
{
  if (!isOK()) return 0.0;
  double rvle =  charAscent * m.y1;
  if (rvle < 0)
  {
//    fprintf (stderr, "FIXME negative ascent: SFontTTF.cpp\n");
    return 1;
  }
  return rvle;
}

/**
 * Return the calibrated descent
 * @param m is the transformation matrix.
 */
double
SFontTTF::descent (const SS_Matrix2D& m)
{
  if (!isOK()) return 0.0;
  double rvle =  - charDescent * m.y1;
  if (rvle < 0)
  {
//  fprintf (stderr, "FIXME negative descent: SFontTTF.cpp\n");
    return 1;
  }
  return rvle;
}

/**
 * Return the calibrated average width
 * @param m is the transformation matrix.
 */
double
SFontTTF::width (const SS_Matrix2D& m)
{
  if (!isOK()) return 0.0;
  double rvle =  charWidth * m.x0;
  if (rvle < 0) return -rvle;
  return rvle;
}

/**
 * Return the calibrated gap
 * @param m is the transformation matrix.
 */
double
SFontTTF::gap (const SS_Matrix2D& m) 
{
  if (!isOK()) return 0.0;
  double rvle =  lineGap * m.y1;
  if (rvle < 0) return -rvle;
  return rvle;
}

/**
 * Return the scale factor. You multiply this with point size you want.
 * matrix diagonals for a 10 point font is scale, scale
 */
double
SFontTTF::scale ()
{
  if (!isOK()) return 0.0;
  return scaleFactor;
}


/**
 * This routine tries to find the glyph indeces of a unicode input stream
 * @param in is the input stream
 * @param len is the length if in
 * @param out is the output 
 * @return the number of characters processed in 'in'
 */
bool
SFontTTF::findGlyphs (SS_UCS4 in, SV_GlyphIndex* out)
{
  if (!isOK()) return false;
  SString key ((char*)&in, sizeof (SS_UCS4));
  const SString* cached = char2Glyphs.get (key);
  if (cached)
  {
    if (cached->size()<sizeof (SS_GlyphIndex)) return false;
    unsigned int usize = cached->size();
    for (unsigned int i=0; i<usize; i+= sizeof (SS_GlyphIndex))
    {
       out->append (*((SS_GlyphIndex*)&cached->array()[i]));
    }
    return true;
  }
  

  TTF_CMAP* cmap_table = (TTF_CMAP*) tables[SS_TB_CMAP];
  int num_tables = ntohs(cmap_table->numberOfEncodingTables);
  if (num_tables ==0)
  {
    SString chc; char2Glyphs.put (key, chc);
    return false;
  }

  /* Try to get the ligature index from OTF */
  SS_UCS4 lig = in;
  unsigned int liglen = 0;
  unsigned int scriptcode = getLigatureScriptCode (lig);
  /* no support yet for this monster */

  if ( (scriptcode == SD_ROVASIRAS  || scriptcode == SD_PUA_ROVAS)
       
       && (liglen=getLigatureUnicode(lig, 0)) > 0)
  {
     if (fontencoding.size()!=0)
     {
        SString chc; char2Glyphs.put (key, chc);
        return false;
     }
     SS_UCS4* chars =  new SS_UCS4[liglen];
     CHECK_NEW (chars);
     getLigatureUnicode (lig, chars);
     SS_GlyphIndex* gi = new SS_GlyphIndex [liglen];
     CHECK_NEW (gi);

     unsigned int count = 0;
     unsigned int i;
     SS_GlyphIndex fontZWJ = findGlyph (SD_CD_ZWJ, false); 
     if (fontZWJ == 0) fontZWJ = SD_G_INDIC_ZWJ;
     for (i=0; i<liglen; i++)
     {
        // We only deal with logical order ligatures.
        gi[count] = findGlyph (chars[i], false);
        if (gi[count] == 0 && chars[i] == SD_CD_ZWJ) {
            gi[count] = SD_G_INDIC_ZWJ;
        }
        if (gi[count] == 0) 
        {
            if (chars[i] != 0x200d) 
            {
               SString chc; char2Glyphs.put (key, chc);
               delete[] chars;
               delete[] gi;
               return false;
            }
        } 
        else
        {
          count++;
        }
     }
     if (count == 1)
     {
       for (i=0; i<count; i++) out->append (gi[i]); 
       SString chc ((char*)gi, count * sizeof (SS_GlyphIndex));
       char2Glyphs.put (key, chc);
       delete[] chars;
       delete[] gi;
       return true;
     }
    
     SS_GlyphIndex lig = 0;

     // I dont know yet what feature is best. 

     // We dont know the script code, so we pass 0.
     // We dont know if it should be alig, dlig or rlig. 
     // Use all that is available.
     unsigned int ligs = getOTFLigature (0, "rlig,liga", &gi[0], count, &lig, 4);
     while (ligs > 1 && ligs <= count) 
     {
       gi[0] = lig;
       // ligs fell out.
       for(i=0; i<count-ligs; i++) gi[i+1] = gi[i+ligs];
       count = count-ligs+1;
       ligs = getOTFLigature (0, "rlig,liga", &gi[0], count, &lig, 4);
     }

     for (i=0; i<count; i++) {
        if (gi[i] != fontZWJ) 
        {
            out->append (gi[i]); 
        } 
     }
     SString chc ((char*)out->array(), out->size() * sizeof (SS_GlyphIndex));
     char2Glyphs.put (key, chc);
     delete[] chars;
     delete[] gi;
     return true;
     
  }
  else if (scriptcode == SD_COMBINING_LIGATURE)
  {
    /* never comes here */
    SString chc;
    char2Glyphs.put (key, chc);
    return false;
  }
  else if (scriptcode == SD_AS_SHAPES && fontencoding.size() == 0
     && hasOTFLigatures())
  {
    /* No encoder support for OTF single substitution */
    if (hardWire == SS_MSLVT)
    {
      SString chc;
      char2Glyphs.put (key, chc);
      return false;
    }
    bool success = false;

    unsigned int fcode = (lig & 0xf000) >> 12;
    SS_UCS4 gcode = lig & 0x0fff;

    SS_GlyphIndex gi[2];
    unsigned int len = 1;
    bool shouldBe1 = true;
    switch (gcode)
    {
    case 1: /* A000X001 */
      gi[0] = findGlyph (0x072A); 
      gi[1] = findGlyph (0x0308); 
      len = 2;
      shouldBe1 = false;
      break;
    case 2: /* A000X002 */
      gi[0] = findGlyph (0x06A9); 
      gi[1] = findGlyph (0x0627); 
      len = 2;
      break;
    case 3: /* A000X003 */
      gi[0] = findGlyph (0x06A9); 
      gi[1] = findGlyph (0x0644); 
      len = 2;
      break;
    default:
      gi[0] = findGlyph (gcode); 
      break;
    }
    /* Check if we got all glyphs */
    for (unsigned int i=0; i<len;  i++)
    {
      if ( gi[i] == 0)
      {
        SString chc;
        char2Glyphs.put (key, chc);
        return false;
      }
    }
    /* isolated=1 initial=2 medial=3 final=4 */ 
    // Miikka:
    //  For some strange reason, Syriac alaph-fj is known as
    //  "fina" in OTF, and alaph-r as "med2", so we'll swap
    //  these two
    if (gcode == 0x0710) {
       if (fcode == 4) fcode=5;
       else if (fcode == 5) fcode=4;
    }

    if (len==2)
    {
      SS_GlyphIndex out = 0;
      if (getOTFLigature ("syrc", "ccmp", gi, 2, &out, 4))
      {
        gi[0] = out; len = 1;
        success = true;
      }
      /* is it urdu or just urd<space>? */
      else if (getOTFLigature ("urd ", "ccmp", gi, 2, &out, 4))
      {
        gi[0] = out; len = 1;
        success = true;
      }
      else if (getOTFLigature ("urdu", "ccmp", gi, 2, &out, 4))
      {
        gi[0] = out; len = 1;
        success = true;
      }
    }
    const char* fname = getShapeCode (fcode-1);
    SS_GlyphIndex go = substituteOTFGlyph (fname, gi[0]);

    /* use it if found - fallback otherwise */
    if (go)
    {
      gi[0] = go;
      success = true;
    }
    else if (len!=2)/* fallback where placement is important  */
    {
      SString chc;
      char2Glyphs.put (key, chc);
      return false;
    }

    /* Try to get a ligature substitution */
    if (len==2)
    {
      SS_GlyphIndex out = 0;
      /* FIXME: How about URDU? 
       * should we do this before shaping?
       */
      if (getOTFLigature ("syrc", "rlig", gi, 2, &out, 4))
      {
        gi[0] = out; len = 1;
        success = true;
      }
    }
    if (shouldBe1 && len != 1) success = false;
    if (success)
    {
      out->append (gi[0]);
      if (len==2) out->append (gi[1]);

      SString chc ((char*)out->array(), 
        out->size() * sizeof (SS_GlyphIndex));
      char2Glyphs.put (key, chc);
      return true;
    }
    else
    {
      SString chc;
      char2Glyphs.put (key, chc);
      return false;
    }
  }

  /* INDIC */
  else if (isLigature (lig) && hasOTFLigatures() 
      && scriptcode != SD_AS_SHAPES && scriptcode != SD_AS_LITERAL
       && (liglen=getLigatureUnicode(lig, 0)) > 0)
  {
     if ((hardWire == SS_MSLVT || hardWire == SS_NOJAMO)
          && scriptcode!=SD_HANGUL_PREC && scriptcode!=SD_HANGUL_JAMO)
     {
       SString chc;
       char2Glyphs.put (key, chc);
       return false;
     }

     bool fixedcluster = true;

     SS_UCS4* chars =  new SS_UCS4[liglen];
     CHECK_NEW (chars);
     getLigatureUnicode (lig, chars);

     /*
      * Complex script rendering, with uniscribe-like algorithm.
      * This can be enabled with command line:
      *       -us 
      * option.
      * SS_MSLVT and SS_NOJAMO hardwired fonts will not be processed.
      */
     SScriptProcessor engine (this);

     // Should be able to start with ZWJ 
     SS_UCS4 sample = ((chars[0] == 0x200D || chars[0] == 0x25cc) && liglen > 1) ? chars[1] : chars[0];
     // Precompiled Hangul should not go through this.
     if (scriptcode!=SD_HANGUL_PREC 
         && hardWire!=SS_MSLVT 
         && hardWire!=SS_NOJAMO 
         && engine.isSupported(sample))
     {
       bool isbegin = (scriptcode == SD_BENGALI_BEGIN);
       unsigned int plen = engine.put (chars, liglen, isbegin);
       /*
        * We already have a full cluster, so we can fail
        * only if the engine can not find some glyphs.
        */
       if (plen != liglen)
       {
         SString chc;
         char2Glyphs.put (key, chc);
         delete[] chars;
         return false;
       }
       engine.apply ();
       *out =engine.getGlyphs ();
       if (out->size()==0)
       {
         SString chc;
         char2Glyphs.put (key, chc);
         delete[] chars;
         return false;
       } 

       /* Maintain our glyph-cache. */
       SString chc ((char*)out->array(), out->size() * sizeof (SS_GlyphIndex));
       char2Glyphs.put (key, chc);

       /* Maintain our position-cache. */
       SV_INT positions = engine.getPositions();
       positions.append (engine.getWidth());
       mark2BaseList.put (key, positions);
       delete[] chars;
       return true;
     }

     /*
      * Hangul, Thai and Lao is processed right here in the switch
      */
     switch (scriptcode)
     {
     case SD_THAI:
     case SD_LAO:
       {
         bool ret = false;
         /* don't support non-unicode encoded fonts for now */
         const char * script = getLigatureScript (lig);
         if (fontencoding.size()!=0 || !isOK()  || script==0)
         {
           ret = false;
         }
         else
         {
           ret = findSouthIndicGlyphs (key, scriptcode, 
               script, chars, liglen, out);
         }
         if (ret)
         {
           SString chc ((char*)out->array(), 
              out->size() * sizeof (SS_GlyphIndex));
           char2Glyphs.put (key, chc);
         }
         else
         {
           SString chc;
           char2Glyphs.put (key, chc);
         }
         delete[] chars;
         return ret;
       }
     case SD_HANGUL_PREC:
     case SD_HANGUL_JAMO:
       {
         bool ret = findJamoGlyphs (chars, liglen, out);
         /* cache */
         if (ret)
         {
           SString chc ((char*)out->array(), 
              out->size() * sizeof (SS_GlyphIndex));
           char2Glyphs.put (key, chc);
         }
         else
         {
           SString chc;
           char2Glyphs.put (key, chc);
         }
         delete[] chars;
         return ret;
       }
     case SD_TAMIL:
       fixedcluster = true;
       break;
     default: 
       fixedcluster = false;
       break;
     }

     SUniMap umap = charEncoder;
     if (!umap.isOK())
     {
        delete[] chars;
        SString chc;
        char2Glyphs.put (key, chc);
        return false;
     }

     /* we allocate one more to allow for LEFT_RIGHT vowel expansion */
     SS_GlyphIndex* gchars = new SS_GlyphIndex[liglen+1];
     CHECK_NEW (gchars);
     const char * script = getLigatureScript (lig);
     if (script == 0) script = "default";

     /* get the encoder for this table. */
     bool decoded = true; 
     /* we need this hocus-pocus because getLigature works on
        glyph indeces */
     /* for indic modifiers */
     unsigned int mstart = 0;
     unsigned int mend = 0;
     for (unsigned int i=0; i<liglen; i++)
     {
       SS_UCS2 ucs2 = umap.encode (chars[i]);
       if (ucs2==0)
       {
         if (chars[i] > 0xffff)
         {
            decoded = false;
            break;
         }
         /* BE AWARE HACK! Try straight unicode  */
         ucs2 = chars[i];
       }
       gchars[i] =  findGlyph(ucs2);
       if (gchars[i]==0)
       {
         decoded = false;
         break;
       }
       int endtype = getCharType (chars[i]);
       if (i>0 && endtype == SD_INDIC_MODIFIER && mstart == 0)
       {
          mstart = i; mend = liglen;
       }
     }
     /* adjust liglen to where modifiers start */
     if (mstart != 0)
     {
        liglen = mstart;
     }

     /* this is unicode encoded... */
     SS_GlyphIndex halant = findGlyph (getHalant (scriptcode));
     SS_GlyphIndex reorder = 0;
     SS_GlyphIndex addVirama = 0;
     unsigned int inlen = liglen;
     bool *gbase = NULL;

     // post-consonant Malayalam ra has to be reordered to syllable start
     if (scriptcode == SD_MALAYALAM)
        reorder = findGlyph (0x0d30);

     // special rules for clusters ending in virama
     if (decoded && liglen == 2 && chars[1] == getHalant(scriptcode))
     {
       decoded = false;
       unsigned int olen = getOTFLigatures (gchars, inlen, 
            script, "haln", halant, reorder, gbase);
       if (olen != inlen)
       {
          debugChars ("GCHARS haln=", gchars, olen);
          decoded = true;
          inlen--;
       }
     }
     else if (decoded && chars[liglen-1] == getHalant(scriptcode))
     {
        // todo - RA+H RA+H
        addVirama = gchars[liglen-1];
        inlen--;
     }
     /*
      * Scripts like Tamil do not need complex processing. 
      * The combinations are finite, a fixed cluster suffices.
      */
     if (fixedcluster && decoded)
     {
       SS_GlyphIndex gi;
       unsigned int nind =  getOTFLigature (script, 0, gchars, liglen, &gi);
       if (nind == liglen)
       {
         out->append (gi);
       }
       else
       {
         decoded = false;
       }
     }
     /*
      * Complex script rendering, with our own algorithm.
      */
     else if (decoded)
     {
       /* ----> DEBUG Information */
       debugChars ("GCHARS=", gchars, liglen);
#ifdef DEBUG_LIGATURE
       fprintf (stderr, "Halant=%04X reorder=%04X gbase=%04X\n", 
         halant, reorder, (gbase==0)?0: *gbase);
#endif
       /* ----< DEBUG Information */
       unsigned int olen = getOTFLigatures (gchars, inlen, 
            script, "akhn", halant, reorder, gbase);
       if (olen != inlen)
       {
          debugChars ("GCHARS akhn=", gchars, olen);
          inlen = olen;
       }
       /* can be at beginning only */
       SS_GlyphIndex rphfGlyph = 0;
       SS_GlyphIndex rphfNone = 0;
       if (inlen>2 && gchars[2] != findGlyph(SD_CD_ZWJ))
       {
         debugChars ("BEFORE RPH =", gchars, inlen);
         SS_GlyphIndex g[2]; g[0] = gchars[0]; g[1] = gchars[1];
         olen = getOTFLigatures (g, 2 , script, "rphf",
              halant, reorder, gbase);
         if (olen == 2)
           olen = getOTFLigatures (g, 2, script, "abvs",
                halant, reorder, gbase);
         if (olen == 1 && liglen > 2)
         {
           int ct = getCharType (chars[2]);
           // if chars[2] == SD_CD_ZWJ will be handled automagically here 
           if (ct == SD_INDIC_CONSONANT_BASE 
             || ct == SD_INDIC_CONSONANT_POST_BASE 
             || ct == SD_INDIC_CONSONANT_BELOW_BASE)
           {
             debugChars ("GCHARS rphf=", g, olen);
             rphfGlyph = g[0];
           }
           else
           {
             //fprintf (stderr, "GCHARS rphfNone\n");
             rphfNone = gchars[0];
           }
           /* remove */
           for (unsigned int i=2; i<inlen; i++) gchars[i-2] = gchars[i];
           inlen -= 2;
         }
         debugChars ("AFTER RPH =", gchars, inlen);
       }

       // Vowel placement in Malayalam is somewhat peculiar, as compared
       // to other Indic scripts; also Telugu and Kannada need special treatment
       if ((scriptcode == SD_MALAYALAM &&
           (getCharType (chars[liglen-1]) == SD_INDIC_LEFT_VOWEL ||
            getCharType (chars[liglen-1]) == SD_INDIC_LEFT_RIGHT_VOWEL)) ||
           ((scriptcode == SD_TELUGU || scriptcode == SD_KANNADA) &&
           liglen > 2))
       {
                  gbase = new bool [inlen-1];
                  for (unsigned int i=0; i<inlen-1; i++)
                  {
                     if (gchars[i] == halant)
                        gbase[i] = false;
                     else gbase[i] = true;
                  }
               }

               olen = getOTFLigatures (gchars, inlen,
                    script, "blwf", halant, reorder, gbase);
               if (olen != inlen)
               {
                  debugChars ("GCHARS blwf=", gchars, olen);
                  inlen = olen;
               }
               olen = getOTFLigatures (gchars, inlen,
                    script, "vatu", halant, reorder, gbase);
               if (olen != inlen)
               {
                  debugChars ("GCHARS vatu=", gchars, olen);
                  inlen = olen;
               }
               olen = getOTFLigatures (gchars, inlen,
                    script, "pstf", halant, reorder, gbase);
               if (olen != inlen)
               {
                  debugChars ("GCHARS pstf=", gchars, olen);
                  inlen = olen;
               }
               olen = getOTFLigatures (gchars, inlen, script, "blws",
                    halant, reorder, gbase);
               if (olen != inlen)
               {
                  debugChars ("GCHARS blws=", gchars, olen);
                  inlen = olen;
               }
               olen = getOTFLigatures (gchars, inlen, script, "psts",
                    halant, reorder, gbase);
               if (olen != inlen)
               {
                  debugChars ("GCHARS psts=", gchars, olen);
                  inlen = olen;
               }
               /* if we still have U+0931 at this point, let's try U+0930 "half" */
               if (gchars[0] == findGlyph(0x0931) && inlen > 2)
               {
                  gchars[0] = findGlyph(0x0930);
                  olen = getOTFLigatures (gchars, inlen, script, "half",
                       halant, reorder, gbase);
                  if (olen != inlen)
                  {
                     debugChars ("GCHARS eyelash=", gchars, olen);
                     inlen = olen;
                  }
                  else // otherwise we change it back to U+0931
                  {
                     gchars[0] = findGlyph(0x0931);
                  }
               }
               /* Half-forms */
               olen = getOTFLigatures (gchars, inlen, script, "half",
                    halant, reorder, gbase);
               if (olen != inlen)
               {
                  debugChars ("GCHARS half=", gchars, olen);
                  inlen = olen;
               }

               olen = getOTFLigatures (gchars, inlen, script,
                   "!pstf,blwf,vatu,blws,rphf,psts,haln", halant, reorder, gbase);
               while (olen != inlen)
               {
                  debugChars ("GCHARS any=", gchars, olen);
                  inlen = olen;
                  olen = getOTFLigatures (gchars, inlen, script,
                    "!pstf,blwf,vatu,blws,rphf,psts,haln", halant, reorder, gbase);
               }

               /* in fact, this alone should do all the junk job (above) */
               /*
                * From: http://www.microsoft.com/typography/otspec/indicot/reg.htm
                *
                * In scripts like Malayalam, the halant form of certain consonants
                * is represented by 'chillaksharams'. These can appear at any
                *  non-initial or final consonant location in a syllable. 
                *
                * - unfortunately it is very vague: 'scripts like Malayalam'
                *    gaspar
                */
               if (inlen > 1)
               {
                 /* does it start with consonant + halant + ZWJ ? */
                 bool firstHalanOK = scriptcode!=SD_MALAYALAM  /* bit vague */
                   || (inlen > 2 && gchars[1] == halant && gchars[2] == SD_G_INDIC_ZWJ);

                 if (firstHalanOK) /* a bit vague */
                 {
                    olen = getOTFLigatures (gchars, inlen, script,
                      "haln", halant, reorder, gbase);
                 }
                 else
                 {
                    olen = getOTFLigatures (&gchars[1], inlen-1, script,
                      "haln", halant, reorder, gbase?&gbase[1]:0);
                    olen++;
                 }
                 inlen = olen;
               }

               /* insert back virama and search for feature "haln" */
               if (addVirama)
               {
                 gchars[olen] = addVirama;
                 inlen++;
                 olen = getOTFLigatures (gchars, inlen, script, "haln",
                      halant, reorder, gbase);
               }
               /* This is "haln" not applied in while loop because of a specific 
                  check condition for SD_G_INDIC_ZWNJ in getOTFLigatures */
               else if (inlen > 1 && gchars[inlen-1] == SD_G_INDIC_ZWNJ)
               {
                 if (scriptcode != SD_MALAYALAM) /* a bit vague */
                 {
                   olen = getOTFLigatures (gchars, inlen-1, script, "haln",
                        halant, reorder, gbase);
                   if (olen != inlen-1)
                   {
                      gchars[olen] = gchars[inlen-1];
                      olen++;
                   }
                 }
               }
               /* insert back non repha after getOTFLigatures */
               if (rphfNone)
               {
                 for (unsigned int i=olen-1; i>1; i--)
                 {
                    gchars[i] = gchars[i-2];
                 }
                 gchars[0] = rphfNone;
                 gchars[1] = halant;
                 olen += 2;
               }
                  
               int endtype = getCharType (chars[liglen-1]);
               switch (endtype)
               {
               case SD_INDIC_LEFT_VOWEL:
                 if (olen > 1)
                 {
                    SS_GlyphIndex g = gchars[olen-1];
                    if (gbase)
                    {
                       unsigned int i;
                       for (i=olen-2; i && !gbase[i]; i--);
                       for (unsigned int j=olen-1; j > i; j--)
                          gchars[j]=gchars[j-1];
                       gchars[i] = g;
                    }
                    else
                    {
                       for (unsigned int i=olen-1; i; i--)
                          gchars[i]=gchars[i-1];
                       gchars[0] = g;
                    }
                 }
                 break;
               case SD_INDIC_RIGHT_VOWEL:
               case SD_INDIC_TOP_VOWEL:
               case SD_INDIC_BOTTOM_VOWEL:
                 if (olen > 0)
                 {
                    SS_GlyphIndex g = gchars[olen-1];
                    if (gbase)
                    {
                       unsigned int i;
                       for (i=olen-2; i && !gbase[i]; i--);
                       for (unsigned int j=olen-1; j > i+1; j--)
                          gchars[j]=gchars[j-1];
                       gchars[i+1] = g;
                    }
                 }
                 break;
               case SD_INDIC_LEFT_RIGHT_VOWEL:
                 if (olen > 0)
                 {
                   SS_GlyphIndex g1 = findGlyph (getLRVowelLeft(chars[liglen-1]));
                   SS_GlyphIndex g2 = findGlyph (getLRVowelRight(chars[liglen-1]));
                   if (g1 && g2)
                   {
                     if (gbase)
                     {
                       unsigned int i;
                       for (i=olen-2; i && !gbase[i]; i--);
                       for (unsigned int j=olen-1; j > i; j--)
                          gchars[j]=gchars[j-1];
                       gchars[i] = g1;
                     }
                     else
                     {
                       for (unsigned int i=olen; i; i--)
                          gchars[i]=gchars[i-1];
                       gchars[0] = g1;
                     }
                     gchars[olen] = g2;
                     olen++;
                 liglen++; // We increase this, so that the program could notice
                   // that the original character sequence has changed
                   }
                 }
               }
               if (rphfGlyph)
               {
                  gchars[olen] = rphfGlyph;
                  olen++;
               }
               /* add modifiers back */
               for (unsigned int i=mstart; i<mend; i++)
               {
                  gchars[olen] = gchars[i];
                  olen++;
                  liglen++;
               }
               inlen = olen;
               olen = getOTFLigatures (gchars, inlen, script, "blws",
                    halant, reorder, gbase);
               if (olen != inlen)
               {
                  debugChars ("GCHARS blws=", gchars, olen);
                  inlen = olen;
               }
               olen = getOTFLigatures (gchars, inlen, script, "abvs",
                    halant, reorder, gbase);
               if (olen != inlen)
               {
                  debugChars ("GCHARS abvs=", gchars, olen);
                  inlen = olen;
               }
               olen = getOTFLigatures (gchars, inlen, script, "psts",
                    halant, reorder, gbase);
               if (olen != inlen)
               {
                  debugChars ("GCHARS psts=", gchars, olen);
                  inlen = olen;
               }

               /* Finally, do a chaining context substitution */
               bool chained = doContextSubstitutions (gchars, inlen, &olen, script, 0);
               if (chained)
               {
                  debugChars ("GCHARS ChainContext=", gchars, olen);
                  inlen = olen;
               }
               /* Just consider this decoded, even if no substitution is made. */
               if (olen > 0)
               {
                  for (unsigned int i=0; i<olen; i++)
                  {
                    out->append (gchars[i]);
                  }
                  decoded = true;
               }
               else
               {
                  decoded = false;
               }
             }

             /*
              * At this point both fixed and variable cluster 
              * glyph substitutions have been finished for
              * all scripts.
              */
             if (decoded)
             {
               SString chc ((char*)out->array(), out->size() * sizeof (SS_GlyphIndex));
               char2Glyphs.put (key, chc);
               /* some scripts, like TIBETAN require more fine grained positioning */
               if (!storeMarkPositions (key, out->array(), out->size()))
               {

        #ifdef DEBUG_LIGATURE
                 fprintf (stderr, "Can not find mark to base for %X\n", in);
        #endif
               }
               else
               {
        #ifdef DEBUG_LIGATURE
                 fprintf (stderr, "Found mark to base for %X\n", in);
        #endif
               }

        #ifdef DEBUG_LIGATURE
               fprintf (stderr, "SFontTTF.cpp: Found OTF ligature:%s[%04X] %u -> %u: ", 
                  script, (lig & 0xffff), liglen, out->size());
               debugChars ("GCHARS glyphs=",out->array(), out->size());
               for (unsigned int i=0; i<liglen; i++)
               {
                 fprintf (stderr, " %X", chars[i]);
               }
               fprintf (stderr, "\n");
        #endif
               delete[] chars;
               delete[] gchars;
       if (gbase) delete[] gbase;
       return true;
     }
     /* try to fall-back to font encoder, or hardwire if any */
     out->clear();
     delete[] chars;
     delete[] gchars;
     if (gbase) delete[] gbase;
  } /* End of Indic/Hangul/OTF */

  /* Let precomposed Hangul through. */
  
  bool okToProcess = true;

  /* Set okToProcess according to artificial encodings */
  switch (hardWire)
  {
  case SS_MSLVT:
    /* precomposed or jamo */
    okToProcess = ((in>=0xac00 && in<0xd7a4) || getJamoClass (in) != SD_JAMO_X);
    break;
  case SS_NOJAMO:
    /* non jamo */
    okToProcess = (getJamoClass (in) == SD_JAMO_X);
    break;
  case SS_NONE:
  default:
    okToProcess = true;
    break;
  }

  if (!okToProcess)
  {
     SString chc;
     char2Glyphs.put (key, chc);
     return false;
  }

  /**
   * When using external maps we are using the same map for all
   * tables.
   */
  if (fontencoding.size()!=0 &&  charEncoder.isOK() && !charEncoder.isUMap())
  {
     /* max 3 */
     SV_UCS4 ucs4; ucs4.append (in); SV_UCS4 decd;
     SUniMap umap = charEncoder;
     unsigned int lifted = umap.lift (ucs4, 0, false, &decd);
     if (lifted == 0)
     {
        /* try straight - font has to have ascii mapping */
        SS_GlyphIndex gi = (in>=0x80) ? 0 :  findGlyph (in);
        if (gi)
        {
          out->append (gi);
          SString chc ((char*) &gi,  sizeof (SS_GlyphIndex));
          char2Glyphs.put (key, chc);
          return true;
        }
        SString chc; char2Glyphs.put (key, chc);
        return false;
     }
     for (unsigned int i=0; i<decd.size(); i++)
     {
        SS_GlyphIndex gi = findGlyph (decd[i]);
        if (gi == 0)
        {
          out->clear ();
          SString chc; char2Glyphs.put (key, chc);
          return false;
        }
        out->append (gi);
     }
     SString chc ((char*) out->array(), out->size() * sizeof (SS_GlyphIndex));
     char2Glyphs.put (key, chc);
     return true;
  }

  /* as I see there is no way to define multiple tables
   * for now so we just hardcode first one in reality we should
   * go through  0..num_tables
   */
  SUniMap umap = charEncoder;
  if (!umap.isOK())
  {
    SString chc; char2Glyphs.put (key, chc);
    return false;
  }
  /* get the encoder for this table. */
  // FIXME:
  // if in is non-BMP we will just use the value - hack - I know 
  SS_UCS4 ucs4 = (in>0xffff) ? in : (SS_UCS4) umap.encode (in);
  if (ucs4==0)
  {
    SString chc; char2Glyphs.put (key, chc);
    return false;
  }
  SS_GlyphIndex o = findGlyph (ucs4);
  if (o==0)
  {
    /* Try the decomposed one instead */
    if (hardWire==SS_MSLVT && 
       /* check for Precomposed Korean or JAMO */
      ((in>=0xac00 && in<0xd7a4) || getJamoClass (in) != SD_JAMO_X))
    {
      SS_UCS4 chars[3]; /* lvt */
      unsigned int liglen = 1;
      /* decompose if precomposed */
      if (in>=0xac00 && in<0xd7a4)
      {
        SS_UCS4 hangul = ucs4 - 0xac00;
        chars[0] = hangul / (21*28) + 0x1100;
        chars[1] = (hangul % (21*28))/28 + 0x1161;
        chars[2] = (hangul % 28) + 0x11a7;
        liglen = (chars[2] == 0x11a7) ? 2 : 3;
      }
      else
      {
        liglen = 1;
        chars[0] = in;
      }
      bool ret = findJamoGlyphs (chars, liglen, out);
      /* cache */
      if (ret)
      {
        if (liglen==1 && getJamoClass (in) != SD_JAMO_L)
        {
          /* standalone jamos fill emptyness */
          SS_GlyphIndex placeHolder = findGlyph (0x4e00);
          if (placeHolder) out->insert (0, placeHolder);
        }
        SString chc ((char*)out->array(), 
           out->size() * sizeof (SS_GlyphIndex));
        char2Glyphs.put (key, chc);
        return ret;
      }
      /* not found */
    }
    /* cache the nothing. */
    SString chc; char2Glyphs.put (key, chc);
    return false;
  }
  out->append (o);
  SString chc ((char*)&o, sizeof (SS_GlyphIndex));
  char2Glyphs.put (key, chc);
  return true;
}

/**
 * Chaining Context Substitution may not change the length of the
 * input.
 * @param ino is the input/output array.
 * @param inlen is the length of the ino array.
 * @param olen is the new length of the ino array.
 * @param script is the OTF script code - or null.
 * @param feature is the OTF feature code - or null.
 * @return true if at least one substitution has been made.
 */
bool
SFontTTF::doContextSubstitutions (SS_GlyphIndex* ino, unsigned int inlen, 
  unsigned int * olen, const char* script, const char* feature)
{
  unsigned int i;
  unsigned int len = inlen;
  *olen = len;
  for (i=0; i<len; i++)
  {
    if (ino[i] == 0) return false;
  }
  unsigned int curin=0;

  // We have our limitattions: it can not increase the glyphs.
  SS_GlyphIndex* lig = new SS_GlyphIndex[len];
  bool isok = false;
  while (curin < len)
  {
    if (len-curin < 2) break;

    // Substitute from curin till the end of the input array. 
    unsigned int nind = 
      getOTFLigature (script, feature, &ino[curin], len-curin, lig, 6);
    if (nind == 0)
    {
      curin++;
      continue;
    }
    /* copy output */
    isok = true;
    for (i=0; i<nind; i++)
    {
      ino[i+curin] = lig[i];
    }
    len = nind + curin;
    curin++;
  }
  delete [] lig;
  *olen = len;
  return isok;
}

/**
 * Get OTF ligatures.
 * @param ino is the input-output buffer
 * @param len is the input length
 * @param gbase contains true at base ligature.
 * @return output length
 */
unsigned int
SFontTTF::getOTFLigatures (SS_GlyphIndex* ino, unsigned int len,
  const char* script, const char* feature, SS_GlyphIndex halant, 
  SS_GlyphIndex reord, bool* base) 
{
  unsigned int i;
  for (i=0; i<len; i++)
  {
    if (ino[i] == 0) return len;
  }
  if (len == 1) return 1;
  /* collect all ligatures in one loop, starting from big ones. */
  unsigned int curin=0;
  unsigned int curout=0;
  /* moved this outside of the loop */
  
  bool needreorder = feature!=0 
    && (strcmp (feature, "vatu") ==0 
        || strcmp (feature, "blwf")==0 || strcmp (feature, "pstf")==0
        || strcmp (feature, "blws")==0 || strcmp (feature, "psts")==0);

  while (curin < len)
  {
    SS_GlyphIndex lig;
    unsigned int nind = 0;
    bool fullglyph = true;
    bool reorder = false;
    /* with these features we always need to reorder stuff */
    if (needreorder) { 
      /* we have at least 3 characters to reorder */
      if (len-curin >= 3 && ino[curin+1] == halant)
      {
        SS_GlyphIndex tmp[3];
        tmp[0] = ino[curin];
        tmp[1] = ino[curin+2];
        tmp[2] = ino[curin+1];
        nind = getOTFLigature (script, feature, tmp, 3, &lig);
      }
      /* we have at least 2 characters to reorder */
      else if (len-curin >= 2 && ino[curin] == halant)
      {
        SS_GlyphIndex tmp[2];
        tmp[0] = ino[curin+1];
        tmp[1] = ino[curin];
        nind = getOTFLigature (script, feature, tmp, 2, &lig);
        fullglyph = false;
        /* set reorder if resulting/original glyphs is a reorder glyph */
        if (reord && tmp[0] == reord) reorder = true;
        /* reorder shows that this should go to zero position */
      }
      else
      {
        /* don't reorder - just apply feature. nothing will happen. */
        nind = getOTFLigature (script, feature, &ino[curin], len-curin, &lig);
      }
    }
    else /* ! needreorder */
    {
      /* just apply feature.  */
      nind = getOTFLigature (script, feature, &ino[curin], len-curin, &lig);
    }
    /* nind is the index of the ligature found */
    bool isok = (nind != 0);

    /* i point to next character index. ino is not rewritten yet and lig may 
       contain a ligature that was found. */
    i=nind+curin;
    if (isok)
    {
      /* don't worry i can not be zero nind!=0 checks it*/
      /* SD_G_INDIC_ZWNJ prevents half form when halant comes. */
      /* halant + ZWNJ */
      if (i+1==len && ino[i-1] == halant && ino[i] == SD_G_INDIC_ZWNJ)
      {
        isok = false;
      }
    }
    /* substitution can go ahead */
    if (isok)
    {
      ino[curout] = lig;
      curout++;
      curin = i;
      /* we need to update base output parameter */
      if (base) {
         unsigned int j, k;
         for (j=curin, k=curout; j<len; j++, k++) base[k]=base[j];
         if (!fullglyph)
         {
            base[curout-1] = false;
         }
      }
      /* we need to update base output parameter */
      if (reorder)
      {
         /* shift stuff up, and insert to zero pos, instead of curout */
         for (unsigned int j=curout; j; j--) ino[j]=ino[j-1];
         ino[0] = lig;
      }
    }
    else  /* not ok, continue loop to find ligatures from next position */
    {
      ino[curout] = ino[curin];
      curout++;
      curin++;
    }
  }
  return curout;
}

/**
 * Find one single glyph. 
 * @return the index or 0.
 * TODO: SFontTTF currently handles TTF_CMAP_FMT4 only. 
 *  This can encode 16 bit unicode only. Make it handle
 *   format 8  Mixed 16 bit and 32 bit coverage using Surrogates(U+D800-U+DFFF)
 *   format 10 Trimmed Array using Surrogates (U+D800-U+DFFF)
 *   format 12 Segmented Coverage using Surrogates (U+D800-U+DFFF)
 * I am not aware of any other format that does not used 
 * this MS surrogate hack.
 * (Guess why are these ugly surrogates in Unicode ? :)
 * /param ownjoiners is true if we use our 
 *        own SD_G_INDIC_ZWJ and SD_G_INDIC_ZWNJ
 */
SS_GlyphIndex
SFontTTF::findGlyph (SS_UCS4 in, bool ownjoiners)
{
  /* Support only BMP for now */
  if (in == SD_CD_ZWJ && ownjoiners) return SD_G_INDIC_ZWJ;
  if (in == SD_CD_ZWNJ && ownjoiners) return SD_G_INDIC_ZWNJ;
  /* Experimental filters for whole ranges */
  switch (hardWire)
  { 
  case SS_INDIC:
    if (in<0x0900 || in>0x0FFF) return 0;
    break;
  case SS_DEVANAGARI:
    if (in<0x0900 || in>0x097F) return 0;
    break;
  case SS_BENGALI:
    if (in<0x0980 || in>0x09FF) return 0;
    break;
  case SS_GURMUKHI:
    if (in<0x0A00 || in>0x0A7F) return 0;
    break;
  case SS_GUJARATI:
    if (in<0x0A80 || in>0x0AFF) return 0;
    break;
  case SS_ORIYA:
    if (in<0x0B00 || in>0x0B7F) return 0;
    break;
  case SS_TAMIL:
    if (in<0x0B80 || in>0x0BFF) return 0;
    break;
  case SS_TELUGU:
    if (in<0x0C00 || in>0x0C7F) return 0;
    break;
  case SS_KANNADA:
    if (in<0x0C80 || in>0x0CFF) return 0;
    break;
  case SS_MALAYALAM:
    if (in<0x0D00 || in>0x0D7F) return 0;
    break;
  case SS_SINHALA:
    if (in<0x0D80 || in>0x0DFF) return 0;
    break;
  case SS_THAI:
    if (in<0x0E00 || in>0x0E7F) return 0;
    break;
  case SS_LAO:
    if (in<0x0E80 || in>0x0EFF) return 0;
    break;
  case SS_TIBETAN:
    if (in<0x0F00 || in>0x0FFF) return 0;
    break;
  case SS_JAMO:
    if (in<0x1100 || in>0x11FF) return 0;
    break;
  default: break;
  }
  // Safeguard against making everything emoji
  // EmojiOne_Color even makes numbers emoji.
  if (isEmoji && in<0x2122) return 0;

  TTF_CMAP* cmap_table = (TTF_CMAP*) tables[SS_TB_CMAP];
  int num_tables = ntohs(cmap_table->numberOfEncodingTables);
  /* Go for it directly */
  bool uniconly = false;
  if (charEncoderTable != (unsigned int) num_tables)
  {
    TTF_CMAP_ENTRY* table_entry = 
             &(cmap_table->encodingTable[charEncoderTable]);

    int offset = ntohl(table_entry->offset);
    int format = ntohs(*((SD_USHORT*)((SD_BYTE*)cmap_table+offset)));
    SS_GlyphIndex gi=0;
    switch (format)
    {
    case 0:
      gi =  findGlyph0 ((TTF_CMAP_FMT0 *)((SD_BYTE *)cmap_table + offset), in);
      break;
    case 2:
      /* this has to have an encoder */
      if (fontencoding.size())
      {
        gi =  findGlyph2 ((TTF_CMAP_FMT2 *)((SD_BYTE *)cmap_table + offset), in);
      }
      break;
    case 4:
      gi =  findGlyph4 ((TTF_CMAP_FMT4 *)((SD_BYTE *)cmap_table + offset), in);
      break;
    case 8: /* TODO */
      break;
    case 10:
      break;
    case 12:
      gi =  findGlyph12 ((TTF_CMAP_FMT12 *)((SD_BYTE *)cmap_table + offset), in);
      break;
    default:
      break;
    }
    if (gi) return gi;
    /* look for unicode encoding only */
    uniconly = true;
  }
  //fprintf (stderr, "Second round\n");

  /* go through all tables */
  int platform = 0;
  int encoding_id = 0;
  for (int i=0; i < num_tables; i++)
  {
    TTF_CMAP_ENTRY* table_entry = &(cmap_table->encodingTable[i]);
    int offset = ntohl(table_entry->offset);
    int format = ntohs(*((SD_USHORT*)((SD_BYTE*)cmap_table+offset)));
    bool isok = true;
    if (uniconly)
    {
      platform = ntohs(table_entry->platformID);
      encoding_id = ntohs(table_entry->encodingID);
      isok = false;
      switch (platform)
      {
      case TT_PLAT_ID_MICROSOFT:
        switch (encoding_id)
        { 
        case TT_ENC_ID_MS_UNICODE:
        case TT_ENC_ID_MS_SURROGATES:
          isok = true; break;
        case TT_ENC_ID_MS_SYMBOL:
        case TT_ENC_ID_MS_SHIFT_JIS:
        case TT_ENC_ID_MS_BIG5:
        case TT_ENC_ID_MS_RPC:
        case TT_ENC_ID_MS_WANSUNG:
        case TT_ENC_ID_MS_JOHAB:
        default:
         break;
        }
        break;
      case TT_PLAT_ID_ISO:
        switch (encoding_id)
        { 
        case TT_ENC_ID_ISO_ASCII:
        case TT_ENC_ID_ISO_10646:
        case TT_ENC_ID_ISO_8859_1:
         isok = true; break;
        case TT_ENC_ID_ANY:
        default:
         break;
        }
        break;
      case TT_PLAT_ID_APPLE:
        switch (encoding_id)
        { 
        case TT_ENC_ID_APPLE_UNICODE_1_1:
        case TT_ENC_ID_APPLE_ISO_10646:
        case TT_ENC_ID_APPLE_UNICODE_2_0:
          isok = true; break;
        case TT_ENC_ID_APPLE_DEFAULT:
        default:
          break;
        }
        break;
      case TT_PLAT_ID_MACINTOSH:
        switch (encoding_id)
        { 
        case TT_ENC_ID_MAC_ROMAN:
          isok = true; break;
          /* a lot of other encodings missing */
        default:
          break;
        }
        break;
      default:
        break;
      }
    }
    if (!isok) continue;
    SS_GlyphIndex gi=0;
    switch (format)
    {
    case 0:/* TODO - SGC 8 bit*/
      gi =  findGlyph0 ((TTF_CMAP_FMT0 *)((SD_BYTE *)cmap_table + offset), in);
      break; 
    case 2:
      /* this has to have an encoder */
      if (fontencoding.size())
      {
        gi =  findGlyph2 ((TTF_CMAP_FMT2 *)((SD_BYTE *)cmap_table + offset), in);
      }
      break;
    case 4:
      gi =  findGlyph4 ((TTF_CMAP_FMT4 *)((SD_BYTE *)cmap_table + offset), in);
      break;
    case 8: /* TODO */
      break;
    case 10:
      break;
    case 12:
      gi =  findGlyph12 ((TTF_CMAP_FMT12 *)((SD_BYTE *)cmap_table + offset), in);
      break;
    default:
      break;
    }
/*
    if (uniconly && in > 0x1000 && format == 12 && gi)
    {
     fprintf (stderr, "platform = %d id=%d\n", platform, encoding_id);
    }
*/
    if (gi) return gi;
  }
  return 0;
}

/**
 * Try to find Glyph in an encoding format 4 table 
 * @param encoding4 is the encoding4 tables 
 * @param ucs2 is the character to find.
 * @return the glyph index.
 */
static SS_GlyphIndex
findGlyph0 (TTF_CMAP_FMT0* encoding0, SS_UCS4 ucs4)
{
  if (ucs4==0||ucs4>255) return 0;
  /* 1 byte does not need byteorder */
  return (SS_GlyphIndex) encoding0->glyphIdArray[ucs4];
}

/**
 * Try to find Glyph in an encoding format 2 table 
 * @param encoding4 is the encoding4 tables 
 * @param ucs2 is the character to find.
 * @return the glyph index.
 */
static SS_GlyphIndex
findGlyph2 (TTF_CMAP_FMT2* encoding2, SS_UCS4 ucs4)
{
  if (ucs4 > 0xffff) return 0;
  unsigned int first = (ucs4>>8) &0xff;
  unsigned int second = ucs4 & 0xff;
  SD_USHORT n = 0;
  if (first == 0)
  {
    SD_USHORT k = ntohs (encoding2->subHeaderKeys[second]) / 8;
    if (k!=0) return 0;
    TTF_CMAP_FMT2_SUBHEADER * sh1 = &encoding2->subHeaders[0];
    SD_USHORT firstCode = ntohs (sh1->firstCode);
    SD_USHORT entryCount = ntohs (sh1->entryCount);
    SD_USHORT ro = ntohs (sh1->idRangeOffset);
    if (firstCode!=0 || entryCount != 256 || ro == 0) return 0;
    unsigned int ind = (ro/2) + (second - firstCode);
    n = (SD_USHORT) ntohs(*(&sh1->idRangeOffset + ind));
    if (n==0) return n;
    SD_SHORT delta = (SD_SHORT)ntohs (sh1->idDelta);
    /* negative possible */
    n += delta;
    n = n % 0xffff;
    return n;
  }
  SD_USHORT k = ntohs (encoding2->subHeaderKeys[first]) / 8;
  /* 1 byte - we can not deal with this */
  if (k==0) return 0;
  TTF_CMAP_FMT2_SUBHEADER * sh = &encoding2->subHeaders[k];
  SD_USHORT firstCode = ntohs (sh->firstCode);
  SD_USHORT entryCount = ntohs (sh->entryCount);

  if (second < (unsigned int) firstCode  || second >= 
         ((unsigned int)firstCode + (unsigned int)entryCount)) return 0;

  SD_USHORT ro = ntohs (sh->idRangeOffset);
  /* If the idRangeOffset value for the segment is not 0, 
   * the mapping of the character codes relies on
   * the glyphIndexArray.
   */
  if (ro==0) return 0;
  /* 
   * The value of the idRangeOffset is the number of bytes past 
   * the actual location of the idRangeOffset word where the 
   * glyphIndexArray element corresponding to firstCode
   * appears 
   */
  unsigned int ind = (ro/2) + (second - firstCode);
  n = (SD_USHORT) ntohs(*(&sh->idRangeOffset + ind));
  if (n==0) return 0;
  /* If the idRangeOffset is 0, the idDelta value is added 
   * directly to the character code to get the corresponding 
   * glyph index
   */
  SD_SHORT delta = (SD_SHORT)ntohs (sh->idDelta);
  /* negative possible */
  n += delta;
  n = n % 0xffff;
  return (SS_GlyphIndex) n;
}

/**
 * Try to find Glyph in an encoding format 4 table 
 * @param encoding4 is the encoding4 tables 
 * @param ucs2 is the character to find.
 * @return the glyph index.
 */
static SS_GlyphIndex
findGlyph4 (TTF_CMAP_FMT4* encoding4, SS_UCS4 ucs4)
{
  if (ucs4 > 0xffff) return 0;
  /* Finally we found it. Maybe */
  int seg_c2 = ntohs(encoding4->segCountX2);
  SD_SHORT cmap_n_segs = seg_c2 >> 1;
  SD_BYTE* ptr = (SD_BYTE *)encoding4 + 14;
  SD_USHORT* cmap_seg_end = (SD_USHORT *) ptr;
  /* here comes a pad, then: */
  SD_USHORT* cmap_seg_start = (SD_USHORT *) (ptr + seg_c2 + 2);
  SD_SHORT* cmap_idDelta = (SD_SHORT *) (ptr + (seg_c2 * 2 )+ 2);
  SD_SHORT* cmap_idRangeOffset = (SD_SHORT *) (ptr + (seg_c2 * 3) + 2);
  //SD_USHORT* glyphIndexArray = (SD_USHORT *) (ptr + (seg_c2 * 4) + 2);

  /* No choice. Go through the segments */
  for (int j=0; j < cmap_n_segs; j++)
  {
    SD_USHORT start = ntohs(cmap_seg_start[j]);
    SD_USHORT end   = ntohs(cmap_seg_end[j]);
    SD_USHORT ro    = ntohs(cmap_idRangeOffset[j]);
    if (start == 0xffff) return 0;

    if (ucs4> end || ucs4 < start) continue;
    /* If the idRangeOffset value for the segment is not 0, 
     * the mapping of the character codes relies on
     * the glyphIndexArray.
     */
    SD_USHORT n = 0;
    SD_SHORT delta = ntohs(cmap_idDelta[j]);
    /* 
     * Should be ro only - but it dumps on code2000.ttf 
     * with U+5C81 if I don't check for delta too 
     */
    if (ro!=0 && delta==0)
    {
      //n =  ntohs (glyphIndexArray[ro/2 + (ucs4 - start) + ro]);
      unsigned int ind = (ro/2) + (ucs4 - start);
      n = (SD_USHORT) ntohs(*(&cmap_idRangeOffset [j] + ind));

    }
    /* If the idRangeOffset is 0, the idDelta value is added 
     * directly to the character code to get the corresponding 
     * glyph index
     */
    else if (delta!=0) /* should not really check for != 0 - I am paranoid */
    {
      /* negative possible */
      n = ucs4 + delta;
      n = n % 0xffff;
    }
    return (SS_GlyphIndex) n;
  }
  return 0;
}
/**
 * Try to find Glyph in an encoding format 4 table 
 * @param encoding4 is the encoding4 tables 
 * @param ucs2 is the character to find.
 * @return the glyph index.
 */
static SS_GlyphIndex
findGlyph12 (TTF_CMAP_FMT12* encoding12, SS_UCS4 ucs4)
{
  unsigned int count = ntohl (encoding12->nGroups);
  for (unsigned int i=0; i<count; i++)
  {
    unsigned int start = ntohl (encoding12->entry[i].startCharCode);
    unsigned int end = ntohl (encoding12->entry[i].endCharCode);
    if (ucs4 >= start && ucs4 <= end)
    {
       unsigned int gl = ntohl (encoding12->entry[i].startGlyphCode);
       gl += (ucs4 - start);
       if (gl > 0xffff)
       {
         return 0;
       }
       return (SS_GlyphIndex) gl;
    }
  }
  return 0;
}

/**
 * Draw a single glyph.
 * @pama canvas is the canvas to draw to
 * @param pen is the pen to draw with
 * @param m is the transformation matrix.
 * @param glyphindex is the local glyph index
 * @return nothing 
 */
void
SFontTTF::drawGlyph (SCanvas* canvas, const SS_Matrix2D& matrix, 
  SS_GlyphIndex glyphno)
{
  if (!isOK()) return ;
  if (glyphno == SD_G_INDIC_ZWJ) return;
  if (glyphno == SD_G_INDIC_ZWNJ) return;
  TTF_GLYF* gtable = 0;
  int len =0;
  if (cffFont) {
     cffFont->drawGlyphCFF(canvas, matrix, glyphno);
     return;
  }
  SD_BYTE* gstart = (SD_BYTE *) tables[SS_TB_GLYF];
  if (longOffsets)
  {
     SD_ULONG* lloca = (SD_ULONG *) tables[SS_TB_LOCA];
     unsigned int offs1 = ntohl (lloca[glyphno]);
     unsigned int offs2 = ntohl (lloca[glyphno+1]);
     gtable = (TTF_GLYF *) ((char*)gstart + offs1);
     len = offs2-offs1;
  }
  else
  {
     SD_USHORT* sloca = (SD_USHORT *) tables[SS_TB_LOCA];
     gtable = (TTF_GLYF *) (gstart + (ntohs (sloca[glyphno]) << 1));
     len = (ntohs (sloca[glyphno+1]) - ntohs (sloca[glyphno])) << 1;
  }
  if (len <= 0)
  {
    if (!broken && len < 0)
    {
      fprintf (stderr, "SFontTTF-2: non-existent glyph %u in %*.*s %d\n",
        (unsigned int) glyphno,
        SSARGS (name), (int) len) ;
      broken = true;
    }
    return;
  }

  TTF_GLYF* kludge = 0;
  if ((((unsigned long) gtable) & 1) != 0)
  {
    if (!broken)
    {
// TODO: dont need this fprintf  (stderr, "SFontTTF: fixing unaligned %*.*s.\n", SSARGS(name));
      broken = true;
    }
    kludge = new TTF_GLYF[len]; 
    CHECK_NEW (kludge);
    memcpy (kludge, gtable, len * sizeof (TTF_GLYF));
    gtable = kludge;
  }

  int ncontours = (int) ((short)ntohs (gtable->numberOfContours));
  if (ncontours <= 0)
  {
    SD_BYTE *ptr = ((SD_BYTE *) gtable + sizeof(TTF_GLYF));
    SD_SHORT *sptr = (SD_SHORT *) ptr;
    SD_USHORT flagbyte;
    do
    {
      SS_Matrix2D m;
      flagbyte = ntohs(*sptr); sptr ++;
      SS_GlyphIndex glyphindex = ntohs(*sptr); sptr ++;

      if (flagbyte & ARG_1_AND_2_ARE_WORDS)
      {
         /* we need to make it short as it can be negative */
         m.t0 = (double) ((SD_SHORT)ntohs(*sptr)); sptr++;
         m.t1 = (double) ((SD_SHORT)ntohs(*sptr)); sptr++;
      }
      else
      {
         char* bptr = (char *)sptr;
         m.t0 = (signed char)bptr[0];
         m.t1 = (signed char)bptr[1];
         sptr ++;
      }
      if (flagbyte & WE_HAVE_A_SCALE)
      {
         m.x0 = m.y1 = f2dot14(*sptr);
         sptr ++;
      }
      else if (flagbyte & WE_HAVE_AN_X_AND_Y_SCALE)
      {
        m.x0 = f2dot14(*sptr); sptr ++;
        m.y1 = f2dot14(*sptr); sptr ++;
      }
      else if (flagbyte & WE_HAVE_A_TWO_BY_TWO)
      {
        m.x0 = f2dot14(*sptr); sptr ++;
        m.y0 = f2dot14(*sptr); sptr ++;
        m.x1 = f2dot14(*sptr); sptr ++;
        m.y1 = f2dot14(*sptr); sptr ++;
      }
      SS_Matrix2D mm = matrix * m;
      /* recursively call itself */
      drawGlyph (canvas, mm, glyphindex);
    } while (flagbyte & MORE_COMPONENTS);
    if (kludge) delete kludge;
    return;
  }

/**
glif
----

int16 numberOfContours 
If the number of contours is positive or zero, it is a single glyph;
If the number of contours less than zero, the glyph is compound 

FWord xMin
FWord yMin
Minimum x,y for coordinate data

FWord xMax
FWord yMax
Maximum x,y for coordinate data

simple glyphs
-------------
uint16 	endPtsOfContours[n]
Array of last points of each contour; n is the number of contours; array entries are point indices

uint16 	instructionLength
uint8   instructions[instructionLength]

uint8  xCoordinates[]
uint8 or int16 yCoordinates[]
*/


  SD_USHORT* contour_end_pt = (SD_USHORT *) ((char *)gtable + sizeof(TTF_GLYF));

  int last_point = (int) ntohs (contour_end_pt[ncontours-1]);

  int n_inst = (int) ntohs (contour_end_pt[ncontours]);

  SD_BYTE* flag_ptr = ((SD_BYTE *)contour_end_pt) + (ncontours << 1) + n_inst + 2;

  int j = 0; int k = 0;

  SBinVector<SD_BYTE> flags;
  while (k <= last_point)
  {
    flags.append (flag_ptr[j]);
    if (flag_ptr[j] & REPEAT)
    {
       for (int k1=0; k1 < flag_ptr[j+1]; k1++)
       {
            k++;
            flags.append (flag_ptr[j]);
       }
       j++;
    }
    j++; k++;
  }
  
  SBinVector<SD_SHORT> xrel;
  SBinVector<SD_SHORT> xcoord;

  for (k=0; k <= last_point; k++)
  {
    /* Process xrel */
    if (flags[k] & XSD_SHORT)
    {
      if (flags[k] & XSAME)
      {
        xrel.append (flag_ptr[j]);
      }
      else
      {
        xrel.append (-flag_ptr[j]);
      }
      j++;
    }
    else if (flags[k] & XSAME)
    {
      xrel.append (0);
    }
    else
    {
      xrel.append (flag_ptr[j] * 256 + flag_ptr[j+1]);
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

  SBinVector<SD_SHORT> yrel;
  SBinVector<SD_SHORT> ycoord;

  /* one more run fore yrel and ycoord */
  for (k=0; k <= last_point; k++)
  {
    if (flags[k] & YSD_SHORT)
    {
      if (flags[k] & YSAME)
      {
        yrel.append (flag_ptr[j]);
      }
      else
      {
         yrel.append (- flag_ptr[j]);
      }
      j++;
    }
    else if (flags[k] & YSAME)
    {
      yrel.append (0);
    }
    else
    {
      yrel.append (flag_ptr[j] * 256 + flag_ptr[j+1]);
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

  /* complete rewrite after Yudit 3.0.1 */
  for (int contour = 0; contour<ncontours; contour++)
  {
    int start_point = (contour==0) ? 0 : ntohs(contour_end_pt[contour-1])+1;
    int end_point = ntohs(contour_end_pt[contour]);

    // control point count end point is inclusive
    int cpc = end_point-start_point+1;
    // bad font
    if (cpc < 0) {
        if (!broken) {
            fprintf  (stderr, "Bad TTF contour count %*.*s.\n", SSARGS(name));
        }
        broken = true;
        continue;
    }
    int base_point= 0;
    // Find a starting point that is on the path. 
    while (base_point<cpc) {
        if (flags[start_point+base_point] & ONOROFF) break;
        base_point++;
    } 
    if (base_point >= cpc) {
        // it is perfectly legal not to have a basepoint
        // do circle.
        if (cpc < 3) {
            if (!broken) {
                fprintf  (stderr, "Not enough points for circle. %*.*s.\n", SSARGS(name));
            }
            broken = true;
            continue;
        }
        moveto (canvas, matrix, (xcoord[start_point]+xcoord[start_point+1])/2, 
            (ycoord[start_point] + ycoord[start_point+1])/2);

        for (int arc=0; arc < cpc; arc++) { 
            int ka =  start_point + ((arc+0) % cpc);
            int kb =  start_point + ((arc+1) % cpc);
            int kc =  start_point + ((arc+2) % cpc);
            cureveto (canvas, matrix, 
               (xcoord[ka]+5*xcoord[kb])/6, (ycoord[ka]+5*ycoord[kb])/6,
               (5*xcoord[kb]+xcoord[kc])/6, (5*ycoord[kb]+ycoord[kc])/6,
               (xcoord[kb]+xcoord[kc])/2, (ycoord[kb]+ycoord[kc])/2);
        }
        canvas->closepath ();
        continue;
    }
    // now we go though and draw.
    for (int point=0; point<cpc; point++) {
        int ps1 = (base_point + point) % cpc;
        int cs1 = start_point + ps1;
        if (point == 0) {
            moveto (canvas, matrix, xcoord[cs1], ycoord[cs1]);
            continue;
        }
        if (flags[cs1] & ONOROFF) {
            lineto (canvas, matrix, xcoord[cs1], ycoord[cs1]);
            continue;
        }
        // The point is not on path, lets find out how long.
        int psg = ps1;
        int nguide = 0;
        while (true) {
            if (flags[start_point + (psg % cpc)] & ONOROFF) {
                break;
            }
            if ((psg+1)%cpc == ps1) {
                break;
            }
            psg++;
            nguide++;
            if (nguide >= cpc) {
                fprintf  (stderr, "Internal error %*.*s.\n", 
                    SSARGS(name));
                psg = ps1;
                nguide = 0;
                break;
            }
        }
        // consume guide.
        point += nguide; 

        // back 1 point.
        int ps = (ps1 + cpc -1) % cpc;
        int cs = start_point + ps;

        int ps2 = (ps1 + 1) % cpc;
        int cs2 = start_point + ps2;

        int ps3 = (ps1 + 2) % cpc;
        int cs3 = start_point + ps3;

        int ce = start_point + (psg % cpc);
        switch (nguide)
        {
        case 0: // never
            lineto (canvas, matrix, xcoord[ce], ycoord[ce]);
            break;
        case 1:
            cureveto (canvas, matrix,
                (xcoord[cs]+2*xcoord[cs1])/3, (ycoord[cs]+2*ycoord[cs1])/3,
                (2*xcoord[cs1]+xcoord[ce])/3, (2*ycoord[cs1]+ycoord[ce])/3,
                xcoord[ce], ycoord[ce]);
            break;

        case 2: 
            cureveto (canvas, matrix, 
                (-xcoord[cs]+4*xcoord[cs1])/3, (-ycoord[cs]+4*ycoord[cs1])/3,
                (4*xcoord[cs2]-xcoord[ce])/3, (4*ycoord[cs2]-ycoord[ce])/3,
                xcoord[ce], ycoord[ce]);
            break;

        case 3: 
            cureveto (canvas, matrix, 
                (xcoord[cs]+2*xcoord[cs1])/3, (ycoord[cs]+2*ycoord[cs1])/3,
                (5*xcoord[cs1]+xcoord[cs2])/6, (5*ycoord[cs1]+ycoord[cs2])/6,
                (xcoord[cs1]+xcoord[cs2])/2, (ycoord[cs1]+ycoord[cs2])/2);
            cureveto (canvas, matrix,
                (xcoord[cs1]+5*xcoord[cs2])/6, (ycoord[cs1]+5*ycoord[cs2])/6,
                (5*xcoord[cs2]+xcoord[cs3])/6, (5*ycoord[cs2]+ycoord[cs3])/6,
                (xcoord[cs3]+xcoord[cs2])/2, (ycoord[cs3]+ycoord[cs2])/2);
            cureveto (canvas, matrix,
                (xcoord[cs2]+5*xcoord[cs3])/6, (ycoord[cs2]+5*ycoord[cs3])/6,
                (2*xcoord[cs3]+xcoord[ce])/3, (2*ycoord[cs3]+ycoord[ce])/3,
                xcoord[ce], ycoord[ce]);
            break;
        default:

            cureveto (canvas, matrix, 
                (xcoord[cs]+2*xcoord[cs1])/3, (ycoord[cs]+2*ycoord[cs1])/3,
                (5*xcoord[cs1]+xcoord[cs2])/6, (5*ycoord[cs1]+ycoord[cs2])/6,
                (xcoord[cs1]+xcoord[cs2])/2, (ycoord[cs1]+ycoord[cs2])/2);

            int k1 = ps + nguide;
            int ka;
            for (k = ps+2; k <= k1-1; k++)
            {
                ka = start_point + ((k-1+cpc) % cpc); 
                int kb = start_point + (k % cpc); 
                int kc = start_point + ((k+1) % cpc); 

                cureveto (canvas, matrix, 
                    (xcoord[ka]+5*xcoord[kb])/6, (ycoord[ka]+5*ycoord[kb])/6,
                    (5*xcoord[kb]+xcoord[kc])/6, (5*ycoord[kb]+ycoord[kc])/6,
                    (xcoord[kb]+xcoord[kc])/2, (ycoord[kb]+ycoord[kc])/2);
            }
            ka = start_point + ((k1-1+cpc) % cpc); 
            k1 = start_point + (k1 % cpc);
            cureveto (canvas, matrix, 
                (xcoord[ka]+5*xcoord[k1])/6, (ycoord[ka]+5*ycoord[k1])/6,
                (2*xcoord[k1]+xcoord[ce])/3, (2*ycoord[k1]+ycoord[ce])/3,
                xcoord[ce], ycoord[ce]);
            break;
        }   /* end switch (nguide) */
    } 
    canvas->closepath ();
  }
  if (kludge!=0) delete kludge;
  return;
}

/**
 * The moveto
 */
static void
moveto (SCanvas* canvas, const SS_Matrix2D &m,
  SD_SHORT _x, SD_SHORT _y)
{
  double x = m.x0 * double (_x) + m.y0 * double (_y) + m.t0;
  double y = m.x1 * double (_x) + m.y1 * double (_y) + m.t1;
  canvas->moveto (x, y);
}
/**
 * The lineto
 */
static void
lineto (SCanvas* canvas, const SS_Matrix2D &m,
  SD_SHORT _x, SD_SHORT _y)
{
  double x = m.x0 * double (_x) + m.y0 * double (_y) + m.t0;
  double y = m.x1 * double (_x) + m.y1 * double (_y) + m.t1;
  canvas->lineto (x, y);
}

/**
 * The curveto
 */
static void
cureveto (SCanvas* canvas, const SS_Matrix2D &m,
  SD_SHORT _x0, SD_SHORT _y0,
  SD_SHORT _x1, SD_SHORT _y1,
  SD_SHORT _x2, SD_SHORT _y2)
{
  double x0 = m.x0 * double (_x0) + m.y0 * double (_y0) + m.t0;
  double y0 = m.x1 * double (_x0) + m.y1 * double (_y0) + m.t1;

  double x1 = m.x0 * double (_x1) + m.y0 * double (_y1) + m.t0;
  double y1 = m.x1 * double (_x1) + m.y1 * double (_y1) + m.t1;

  double x2 = m.x0 * double (_x2) + m.y0 * double (_y2) + m.t0;
  double y2 = m.x1 * double (_x2) + m.y1 * double (_y2) + m.t1;
  canvas->curveto (x0, y0, x1, y1, x2, y2);
}

/**
 * Create double numbers between -2 .. 1.99994 
 * from a packed short.
 */
static double
f2dot14 (short x)
{
  short y = ntohs(x);
  return (y >> 14) + ((y & 0x3fff) / 16384.0);
}

static void
debugChars (const char* msg, const SS_GlyphIndex* gchars, unsigned int len)
{
#ifdef DEBUG_LIGATURE
  fprintf (stderr, "SFontTTF.cpp: %s", msg);
  for (unsigned int i=0; i<len; i++)
  {
    fprintf (stderr, " %04X", gchars[i]);
  }
  fprintf (stderr, "\n");
#endif
}
