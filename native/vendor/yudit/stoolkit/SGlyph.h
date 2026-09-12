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

#ifndef SGlyph_h
#define SGlyph_h

#include "stoolkit/SVector.h"
#include "stoolkit/STypes.h"
#include "stoolkit/SUniMap.h"
#include "stoolkit/SString.h"
#include "stoolkit/SCharClass.h"
#include "stoolkit/SEmbedState.h"
#include "stoolkit/SGlyphShared.h"

/**
 * Cache
 */
SGlyphShared* getGlyphShared (SS_UCS4 c);

class SGlyph : public SObject
{
public:
  SGlyph (const SV_UCS4 &decomp, SS_UCS4 comp, bool shaped, 
          unsigned int clusterindex, 
          unsigned int composingIndex, 
          bool usePrecomposed); 

  SGlyph (const SGlyph& glyph);
  SGlyph (SGlyphShared* _shared);

  SGlyph operator=(const SGlyph& glyph);
  virtual ~SGlyph ();
  virtual SObject*  clone() const;

  bool operator == (const SGlyph& g2) const;
  bool operator != (const SGlyph& g2) const;

  bool isWhiteSpace() const;
  bool isDelimiter() const;
  bool canWrap () const;
  bool isNumber() const;
  bool isLetter() const;
  bool isTransparent() const;
  bool isEOP() const;
  bool isEOL() const;
  bool isFF() const;
  bool isTab() const;

  bool isCluster() const;
  bool isYuditLigature() const;
  bool isYuditComposition() const;
  bool isSpecial()  const;
  bool isMirrorable() const;
  SD_CharClass getType()  const;
  SD_BiDiClass getBiDiType() const;

  SString charKey() const;

  bool setShape (const SGlyph* before, const SGlyph* after);

  
  /* Work on characters */
  const SS_UCS4 getChar() const;
  const SS_UCS4 getMirroredChar() const;
  const SS_UCS4 getShapedChar() const;
  const SS_UCS4 getFirstChar () const;

  /* Work on decompositions */
  SV_UCS4 getChars() const; /* returns decomposed or precomposed  */

  unsigned int decompSize() const;
  unsigned int compSize() const;

  const SS_UCS4* getShapeArray() const; /* fast */
  const SS_UCS4* getDecompArray() const; /* fast */
  const SS_UCS4* getCompArray() const; /* fast */
  const SS_UCS4* getShapeFallback() const; /* fast */

  /* array operator works on memory representation */
  SS_UCS4 operator[] (unsigned int index) const; /* fast */

  /* now comes the real hack. */
  bool addComposing(SS_UCS4 c);
  SS_UCS4 removeComposing();

  inline bool isLR() const;

  /* conformant algorithm needs it */
  inline unsigned int getExplicitLevel() const;
  inline bool isOverride() const;

  inline void setEmbeddingMarks (const SV_UCS4& stack);
  inline SV_UCS4 getEmbeddingMarks (const SGlyph* from) const;

  /* for convenience */
  inline const SEmbedState& getEmbedState() const;

  /* These should be constants */
  bool    underlined;
  bool    selected;
  bool    usePrecomp;
  char    currentShape;
  SD_CharClass decompCharClass;
  

  /* These are states  */
  char    embedding;  /* resolved embedding level */

private:
  char getShape (const SGlyph* before, const SGlyph* after);

  SEmbedState    state;
  SGlyphShared * shared;

};

bool
SGlyph::isOverride() const
{
  return state.isOverride();
}

unsigned int
SGlyph::getExplicitLevel() const
{
  return state.getExplicitLevel();
}

/**
 * Return and vector that contains LRO,RLO,LRE,RLE and PDF marks 
 * This array can bring this to from level.
 * @from is a glyph relative to which we consume embedding levels.
 * If from is zero zero level is assumed.
 */ 
SV_UCS4
SGlyph::getEmbeddingMarks (const SGlyph* from) const
{
  return SV_UCS4(state.getEmbeddingMarks((from==0)?0:&from->state));
}

void
SGlyph::setEmbeddingMarks (const SV_UCS4& stack)
{
  state.setEmbeddingMarks(stack);
}

/**
 * return the embedding level, as calculated by bidi
 */
bool
SGlyph::isLR() const
{
  return ((embedding % 2) == 0);
}

/**
 * return the explicit embedding state of this glyph
 */
const SEmbedState&
SGlyph::getEmbedState() const
{
  return state;
}

void addFallbackShapes (SUniMap* shaper, const SS_UCS4* shapes,
  const SS_UCS4* chars, unsigned int size);

#endif /* SGlyph_h */
