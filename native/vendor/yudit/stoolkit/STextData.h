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

#ifndef STextData_h
#define STextData_h

#include "stoolkit/SVector.h"
#include "stoolkit/STypes.h"
#include "stoolkit/SString.h"
#include "stoolkit/STextIndex.h"
#include "stoolkit/SCharClass.h"
#include "swindow/SColor.h"
#include "stoolkit/SGlyph.h"
#include "stoolkit/SParagraph.h"
#include "stoolkit/SLineTracker.h"

class STextDataEvent
{
public:
  /* line per line events */
  STextDataEvent (void);
  STextDataEvent (const STextIndex& start, bool attribute=false);
  ~STextDataEvent ();

  void clear();
  void add (const STextDataEvent& evt);
  void setRemaining (const STextIndex& remain);

  bool          valid;
  bool          attribute; // only attribute of text has changed.

  STextIndex    start;
  /* a reversely calculated value */
  STextIndex    remaining;
};

class STextDataListener
{
public:
  virtual void textChanged(void* src, const STextDataEvent& event) = 0;
  virtual ~STextDataListener() {}
};

/**
 * This STextData has a notion about glyphs and compositions.
 */
class STextData
{
public:
  STextData (void);
  STextData (const SString& utf8);
  STextData operator = (const STextData & data);
  virtual ~STextData ();

  SString getText () const;
  SString getText (const STextIndex& index) const;
  SString getText (const STextIndex& begin, const STextIndex& end) const;

  SV_UCS4 getChars (unsigned int line) const
   { return lines[line]->getChars (); }
  STextIndex find (const SString& string);
  const SGlyph& glyphAt (const STextIndex& index) const;
  // we need only this
  const SGlyph* peekGlyphAt (const STextIndex& index) const;

  /* these, till fireEvent all work with events */
  void move (const STextIndex& index);

  /* moves the cursor to the end */
  void setText (const SString& utf8);

  void insert (const SString& data);

  /* remove moves the cursor */
  void remove (const STextIndex& index);

  /* add a composing character */
  bool addComposing(SS_UCS4 c, bool toleft);

  /* remove a composing character */
  SS_UCS4 removeComposing(bool toleft);

  /* These just change the text */
  void select (const STextIndex& index, bool is=true);
  void underline (const STextIndex& index, bool is=true);
  bool setParagraphSeparator (const SString& str);

  bool isLR (unsigned int parag) const;
  bool isLR (const STextIndex& index) const;
  SEmbedState getEmbedState (const STextIndex& index) const;

  void clear ();
  void fireEvent ();
  void clearEvent ();

  unsigned int  size() const;
  unsigned int  size(unsigned int line) const;
  // For syntax, It will not expand line
  unsigned int  softSize (unsigned int line) const;
  SS_UCS4       softCharAt (unsigned int line, unsigned int pos) const;

  STextIndex getTextIndex (int charOffset=0, bool logical=false) const;
  STextIndex getTextIndex(const STextIndex& base, int charOffset=0, bool logical=false) const;
  STextIndex getMaxTextIndex(const STextDataEvent& evt) const;
  STextIndex getMinTextIndex(const STextDataEvent& evt) const;

  unsigned int toLogical (unsigned int line, unsigned int index);
  SV_UINT getLogicalMap(unsigned int line) const;

  // Set only
  void addTextDataListener (STextDataListener* listener);
  // Set only - lineExpanded is never called, it mmust be handled separately
  void addLineTracker (SLineTracker* lt);

  bool isProperLine (unsigned int line) const;
  bool isVisible(unsigned int line) const;
  void setVisible(unsigned int line);

  bool isReordered (unsigned int line) const;
  void setReordered(unsigned int line);

  void setDocumentEmbedding(SS_Embedding e);
  SS_Embedding getDocumentEmbedding() const;

  bool isWhiteSpace (const STextIndex& index) const;
  bool isDelimiter (const STextIndex& index) const;
  bool canWrap (const STextIndex& index) const;
  bool isNumber (const STextIndex& index) const;
  bool isLetter (const STextIndex& index) const;
  bool isTransparent (const STextIndex& index) const;

  void setLineBreaks (unsigned int line, const SV_UCS4& breaks);

  bool getInitialLR() const;
  void setInitialLR(bool lr);

  const STextDataEvent& getCurrentEvent () const { return  event; }

  bool isExpanded (unsigned int line) const
     { return lines[line]->isExpanded(); }

  SS_ParaSep getParagraphSeparator (unsigned int line) const
     { return lines[line]->getParagraphSeparator (); }

private:
  SS_Embedding         embedding;

  /* insert moves the cursor */
  unsigned int properSize(unsigned int line) const;
  void insert (const SGlyph& glyph);

  bool setParagraphSeparator (SS_ParaSep ps);
  void reShape (const STextIndex& index);
  void reShapeOne (const STextIndex& index);

  /* moves the cursor to the end */
  void setText (const SV_UCS4& ucs4);

  void setMaxLimits (STextDataEvent* evt, const STextIndex& index);
  STextIndex reorder (const STextIndex& index);

  /* These belong to the event being generated */
  STextDataEvent     event;
  STextIndex         textIndex;

  /* should be a linked list */
  SBinVector<SParagraph*>  lines;
  STextDataListener*     listener;
  SLineTracker*          lineTracker;
};

#endif /* STextData_h */
