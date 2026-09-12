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
 
#ifndef SFontTTF_h
#define SFontTTF_h

#include "swindow/SCanvas.h"
#include "swindow/SPen.h"
#include "swindow/SFontLookup.h"
#include "swindow/SFontCFF.h"

#include "stoolkit/SIO.h"
#include "stoolkit/STypes.h"
#include "stoolkit/SMatrix.h"
#include "stoolkit/SBinHashtable.h"
#include "stoolkit/SHashtable.h"
#include "stoolkit/SProperties.h"
#include "stoolkit/SBinVector.h"
#include "stoolkit/SString.h"
#include "stoolkit/SProperties.h"
#include "stoolkit/SUniMap.h"


/**
 * This file will not be exposed so we can put some local junk here.
 */

// for coords.
typedef  SBinVector<unsigned short> SH_Vector;

/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 * This is the abstract widget toolkit font package
 */

class SFontTTF : public SFontLookup
{
public:
  enum SHardWire { 
     SS_NONE=0, SS_MSLVT, SS_NOJAMO,
     SS_INDIC, SS_JAMO,
     SS_DEVANAGARI, SS_BENGALI, SS_GURMUKHI,
     SS_GUJARATI, SS_ORIYA, SS_TAMIL, SS_TELUGU,
     SS_KANNADA, SS_MALAYALAM, SS_SINHALA, SS_THAI,
     SS_LAO, SS_TIBETAN
  };
  SFontTTF (const SFile& file, const SString& encoding); 
  virtual ~SFontTTF (); 

  bool draw (SCanvas* canvas, const SS_Matrix2D& matrix, SS_UCS4 uch, bool isLRContext);
  bool width (const SS_Matrix2D& matrix, SS_UCS4 uch, double* width_);
  bool isLeftAligned (SS_UCS4 uch) const;

  double width (const SS_Matrix2D& matrix);
  double ascent (const SS_Matrix2D& matrix);
  double descent (const SS_Matrix2D& matrix);
  double gap (const SS_Matrix2D& matrix);
  /* mulltiply this with size you want to get the matrix */
  double scale ();
  bool isOK();

  SS_GlyphIndex findGlyph (SS_UCS4 in, bool ownjoiners=true);

  static void setBase (SS_UCS4 base);
  void getBaseOffsets (const SS_Matrix2D& m, 
    SS_UCS4 mark, double* offx, double* offy);

public:
  // SFontLookup interface implementation
  SS_GlyphIndex gindex (SS_UCS4 in);
  int gwidth (SS_GlyphIndex in);
  unsigned int gsub (const char* script, const char* feature, 
      const SS_GlyphIndex* in, unsigned int in_size,
      unsigned int* start,  SS_GlyphIndex* out, unsigned int* out_size,
      bool* is_contextual);
  bool gpos (const char* script, const char* feature, 
      const SS_GlyphIndex* in, int* x,  int* y);
  unsigned int getGlyphClass(SS_GlyphIndex in);
  bool attach (SS_GlyphIndex base, SS_GlyphIndex mark, int where, int* x, int* y);
  bool getIsEmoji () {
    return isEmoji;
  }
protected:
  // https://docs.microsoft.com/en-us/typography/opentype/spec/otff
  // CFF fonts.
  SFontCFF*           cffFont;

  SHardWire		hardWire;
  SS_GlyphIndex         defaultGlyph;
  SProperties           char2Glyphs;
  SBinHashtable<int>    char2Width;
  SBinHashtable<int>    mark2Base;

  /* this is to position  many points, like Lao,Thai,Tibetan */
  SHashtable<SV_INT>    mark2BaseList;

  void drawGlyph (SCanvas* canvas, const SS_Matrix2D& matrix, 
      SS_GlyphIndex glyphno);

  double widthGlyph (const SS_Matrix2D& m, SS_GlyphIndex glyphno);
  int getWidth (SS_GlyphIndex glyphno);

  bool findGlyphs (SS_UCS4 in, SV_GlyphIndex* out);
  bool findJamoGlyphs (const SS_UCS4* in, unsigned int len, SV_GlyphIndex* out);
  bool findSouthIndicGlyphs (const SString& key, unsigned int scriptcode, 
      const char* script, const SS_UCS4* chars, unsigned int liglen, 
      SV_GlyphIndex* out);
  bool storeMarkPositions (const SString& key,
      const SS_GlyphIndex* gv, unsigned int liglen);

  unsigned int getOTFLigature (const char* script, const char* feature, 
    const SS_GlyphIndex* chars, unsigned int liglen, SS_GlyphIndex* out,
    unsigned int substtype = 4);

  bool hasOTFLigatures();

  void getOTFMarkToBase (SS_GlyphIndex baseGlyph, 
       SS_GlyphIndex markGlyph, int* ix, int* iy);

  void getOTFMarkToMark (SS_GlyphIndex baseGlyph, 
       SS_GlyphIndex markGlyph, int* ix, int* iy);

  bool getPositions(int feature, const SS_GlyphIndex* gv, unsigned int gvsize,
    const char* _feature, const char* _script, int* xpos, int* ypos);

  int getGlyphWidth (SS_GlyphIndex glyph);

  int getLeftSideBearing (SS_GlyphIndex glyphno) const;

  bool getBBOX (SS_GlyphIndex glyphno, 
      int* xMin, int* yMin, int* xMax, int* yMax) const;

  SS_GlyphIndex substituteOTFGlyph (const char* feature, SS_GlyphIndex g);

  bool doContextSubstitutions (SS_GlyphIndex* ino, unsigned int inlen, 
    unsigned int * olen, const char* script, const char* feature);

  unsigned int getOTFLigatures (SS_GlyphIndex* ino, 
   unsigned int len, const char* script, const char* feature, 
   SS_GlyphIndex halant, SS_GlyphIndex reord, bool* base);

  bool getContours(SS_GlyphIndex glyphno, SH_Vector * xc, SH_Vector *yc );

  SFileImage image;
  SString name;
  SString fontencoding;
  bool isEmoji;

  SFile file;
  bool ok;
  bool broken;

  bool init ();
  bool processName (); 
  bool checkTables ();
  void getName (long id, const char* str, int len);

  SProperties          names;
  SBinHashtable<void*> tables;

  SUniMap              charEncoder;
  unsigned int         charEncoderTable;

  //double  fontSize;
  double  scaleFactor;

  // line height is charAscent - charDescent + lineGap
  double  lineGap;
  double  charWidth;
  double  charAscent;
  double  charDescent;

  //double  scaleFactor;

  double  italicAngle;
  double  underlineThickness;
  double  underlinePosition;
  bool    isFixedPitch;
  int     longOffsets;

  static SS_UCS4       setBaseCharacter;
  SS_UCS4              baseCharacter;
  SS_GlyphIndex        baseGlyph;
  int                  baseWidth;
};


#endif /* SFontTTF_h */
