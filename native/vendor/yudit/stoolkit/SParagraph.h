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

#ifndef SParagraph_h
#define SParagraph_h

#include "stoolkit/SGlyph.h"
#include "stoolkit/STypes.h"

class SParagraph 
{
public: 
  SParagraph(void);
  SParagraph(const SParagraph& paragraph);

  ~SParagraph();

  SParagraph(const SV_UCS4& buffer, unsigned int *index);
  SParagraph* subParagraph(unsigned int from, unsigned int to) const;

  inline unsigned int size() const;
  inline unsigned int softSize() const;
  inline SS_UCS4 softCharAt (unsigned int pos) const;

  inline const SGlyph& operator [] (unsigned int index) const;
  inline const SGlyph* peek (unsigned int index) const;
  inline bool isExpanded () const;

  void append(const SGlyph& glyph);
  void insert(unsigned int into, const SGlyph& glyph);
  void remove(unsigned int from, unsigned int to);
  void remove(unsigned int at);
  void truncate(unsigned int to);
  void replace(unsigned int at, const SGlyph& glyph);

  bool setParagraphSeparator (SS_ParaSep ps);
  SS_ParaSep getParagraphSeparator () { return paraSep; };
  void setEmbedding(SS_Embedding e);
  
  void clear();

  unsigned int toLogical (unsigned int index);
  const SV_UINT& getLogicalMap() const;

  void setLineBreaks (const SV_UCS4& breaks);

  void select (bool is);
  void select (bool is, unsigned int from, unsigned int to);

  void underline (bool is);
  void underline (bool is, unsigned int from, unsigned int to);

  bool          isLR() const;

  bool          isProperLine () const;

  bool          isVisible() const;
  void          setVisible();

  bool          isReordered () const;
  void          setReordered();

  unsigned int  properSize() const;
  SV_UCS4       getChars() const;

  void          clearChange();
  unsigned int  getChangeStart() const;
  unsigned int  getChangeEnd() const;

private:

  void           setIniLevel ();

  void   setChange(unsigned int from, unsigned int to);
  void   reShape (unsigned int at);
  void   reShape ();
  int    getNonTransparentBefore (unsigned int index) const;
  int    getNonTransparentAfter (unsigned int index) const;

  bool   isLineBreak(unsigned int index) const;
  void   resolveLevels();

  void   expand() const;

  bool           visible;
  bool           reordered;
  bool           selected;
  bool           underlined;
  bool           expanded;

  SS_Embedding   embedding;
  unsigned int   iniLevel;

  SVector<SGlyph>          glyphs;
  SV_UCS4                  ucs4Glyphs;
  SV_UINT                  logical;

  SS_ParaSep     paraSep;
  SV_UCS4        lineBreaks;

  unsigned int   changeStart;
  unsigned int   changeRemaining;

};


unsigned int
SParagraph::size() const
{
  if (!expanded) expand();
  return glyphs.size();
}

unsigned int
SParagraph::softSize() const
{
  if (!expanded) return ucs4Glyphs.size();
  return glyphs.size();
}

SS_UCS4
SParagraph::softCharAt(unsigned int pos) const
{
  if (!expanded) return ucs4Glyphs[pos];
  return glyphs.peek(pos)->getFirstChar();
}

/**
 * Hopefully no one will do this without expending
 * thus calling size()
 */
const SGlyph&
SParagraph::operator [] (unsigned int index) const
{
  if (!expanded) expand();
  return glyphs[index];
}

/**
 * Hopefully no one will do this without expending
 * thus calling size()
 */
const SGlyph*
SParagraph::peek (unsigned int index) const
{
  if (!expanded) expand();
  return glyphs.peek(index);
}

//
// Return true if the line has been split into glyphs
// already. getChars () will not touch expanded flag,
// you can use that to safely get chars without 
// forcing expansion for unexpanded lines.
//
bool
SParagraph::isExpanded () const
{
  return expanded;
}


#endif /* SParagraph_h */
