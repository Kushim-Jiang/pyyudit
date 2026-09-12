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

#include "stoolkit/STextData.h"

#include "stoolkit/SEncoder.h"
#include "stoolkit/SBinHashtable.h"
#include "stoolkit/SUtil.h"
#include "stoolkit/STypes.h"
#include "stoolkit/SCharClass.h"


/*------------------------------------------------------------------------------
 *                     STextDataEvent
 *------------------------------------------------------------------------------
 */
/**
 * @param attr is tru if no text was added or removed. only attribute changed.
 */
STextDataEvent::STextDataEvent (const STextIndex& _start, bool attr)
{
  start = _start;
  valid = false;
  attribute = attr;
}
/**
 * Create a stoolkit STextDataEvent
 */
STextDataEvent::STextDataEvent (void)
{
  clear ();
}

/**
 * Set the remaining.
 */
void
STextDataEvent::setRemaining (const STextIndex& _remaining)
{
  remaining = _remaining;
  valid = true;
}

/**
 * Nothing to destroy. Just in case
 */
STextDataEvent::~STextDataEvent ()
{
}
unsigned int
STextData::softSize (unsigned int line) const
{
  if (line >= size()) return 0;
  const SParagraph* l = lines[line];
  return l->softSize();
}

SS_UCS4
STextData::softCharAt (unsigned int line, unsigned int pos) const
{
  if (line >= size()) return 0;
  const SParagraph* l = lines[line];
  return l->softCharAt(pos);
}

/**
 * Clear all fields.
 */
void
STextDataEvent::clear ()
{
  start = STextIndex (0,0);
  remaining =  STextIndex (0,0);
  valid = false;
  attribute = true;
}

/**
 * Add a new event to this in a sane way.
 */
void
STextDataEvent::add (const STextDataEvent& event)
{
  if (!event.valid)
  {
    return;
  }
  if (!valid)
  {
    start = event.start;
    remaining = event.remaining;
    if (!event.attribute) attribute = false;
  }
  else 
  {  
    if (event.remaining < remaining)
    {
      remaining = event.remaining;
    }
    if (event.start < start)
    {
      start = event.start;
    }
  }
  valid = true;
}

/*------------------------------------------------------------------------------
 *                           STextData
 *------------------------------------------------------------------------------
 */

/**
 * Create and empty STextData 
 */
STextData::STextData (void)
{
  embedding = SS_EmbedNone;
  listener = 0;
  lineTracker = 0;
}

/**
 * Create a text data from utf8.
 * @param utf8 is the input text
 */
STextData::STextData (const SString& utf8)
{
  embedding = SS_EmbedNone;
  listener = 0;
  lineTracker = 0;
  insert (utf8);
}

/**
 * clear the data and assign new data
 */
STextData
STextData::operator = (const STextData & data)
{
  clear ();
  embedding = data.embedding;
  for (unsigned int i=0; i< lines.size(); i++)
  {
    SParagraph *p = new SParagraph ((*(data.lines[i])));
    CHECK_NEW (p);
    p->setEmbedding (embedding);
    p->underline (false);
    p->select (false);
    lines.append (p);
    if (lineTracker) lineTracker->lineInserted (this, lines.size()-1);
  }
  return *this;
}

/**
 * Glyph Lines are pointers
 */
STextData::~STextData ()
{
  for (unsigned int i=0; i<lines.size(); i++)
  {
     delete lines[i];
  }
}

/**
 * Get the contained text
 */
SString
STextData::getText () const
{
  if (size()==0) return SString();
  return getText (STextIndex (0, 0), STextIndex (size(), size(size()))); 
}

/**
 * Get the text between the cursor and here
 */
SString
STextData::getText (const STextIndex& index) const
{
  return getText (textIndex, index); 
}

/**
 * Extract a bi-di marked utf-8 text.
 * @param begin is the starting index
 * @param end is the ending index
 */
SString
STextData::getText (const STextIndex& begin, const STextIndex& end) const
{
  SString ret;
  if (size() == 0) return SString(ret);
  SEncoder ic("utf-8-s");


  STextIndex start;
  STextIndex stop;
  if (end > begin)
  {
    start = begin;
    stop = end;
  }
  else
  {
    start = end;
    stop = begin;
  }
  if (stop.line > size())
  {
     stop.line = size(); stop.index = size(size()-1);
  }
  for (unsigned int i=start.line; i<=stop.line && i<size(); i++)
  {
    if (i>start.line && i<stop.line)
    {
      /* select the whole line */
      SV_UCS4 ucs; ucs.append (lines[i]->getChars ());
      ret.append (ic.encode (ucs));
    }
    else
    {
      unsigned int b = (i==start.line) ? start.index : 0;
      unsigned int e = (i==stop.line)? stop.index : size (i);
      SParagraph* sub =  lines[i]->subParagraph (b, e);
      SV_UCS4 ucs; ucs.append (sub->getChars());
      delete sub;
      ret.append (ic.encode (ucs));
    }
  }
  return SString(ret);
}


/**
 * find string in text data from current position.
 * return an index to the end of that data.
 * move the current position to the beginning of the data.
 * it does not search across lines.
 */
STextIndex
STextData::find (const SString& string)
{
  STextData d (string);

  /**
   * go through all lines.
   */
  for (unsigned int i=textIndex.line; i<size(); i++)
  {
     unsigned int begin = (i==textIndex.line) ? textIndex.index : 0;
     unsigned int end = size (i);

     SParagraph* line = lines[i];
     /**
      * go through all glyphs
      */
     for (unsigned int j=begin; j+d.size(0)<=end; j++)
     {
        /* hack. We don't disclose the SGlyphLIne= */
        bool found = true;
        for (unsigned int k=0; k<d.size(0); k++)
        {
          const SGlyph* g = line->peek(j+k);
          const SGlyph& g2 = d.glyphAt (STextIndex(0, k));
          if (g2 != *g)
          {
            found = false;
          }
        }
        if (found)
        {
          move (STextIndex(i, j));
          return STextIndex (i, j+d.size(0));
        }
     }
  }
  return STextIndex (0,0);
}

/**
 * Get a glyph at a certain position
 * @param index is the glyph index
 * @return a reference to the glyph.
 */
const SGlyph&
STextData::glyphAt (const STextIndex& index) const
{
  return (*lines[index.line])[index.index];
}
/**
 * Get a glyph at a certain position
 * @param index is the glyph index
 * @return a reference to the glyph.
 */
const SGlyph*
STextData::peekGlyphAt (const STextIndex& index) const
{
  return &(*lines[index.line])[index.index];
}

/**
 * move the insertion point.
 * @param line is the line number.
 * @param index is the line index.
 */
void
STextData::move (const STextIndex& index)
{
  STextIndex ndx (index);
  if (ndx.line > size()) ndx.line = size();
  if (ndx.line > 0 && !isProperLine (ndx.line-1))
  {
     ndx.line--;
     ndx.index = size(ndx.line);
  }
  if (ndx.line == size())
  {
    ndx.index = 0;
  }
  else if (ndx.index >= size(ndx.line))
  {
    ndx.index = size(ndx.line);
    if (isProperLine (ndx.line) && ndx.index == size(ndx.line))
    {
      ndx.index = ndx.index-1;
    }
  }
  textIndex = ndx;
}

/**
 * set the text data
 */
void
STextData::setText (const SString& string)
{
  SEncoder ic("utf-8-s");
  SV_UCS4 ucs4 = ic.decode (string, false);
  setText (ucs4);
}

/**
 * set the text data
 */
void
STextData::setText (const SV_UCS4& ucs4)
{
  clear ();
  /* split into lines */
  unsigned int start = 0;
  while (true)
  {
    unsigned int from = start;
    SParagraph* gl = new SParagraph(ucs4, &start);
    CHECK_NEW(gl);
    gl->setEmbedding (embedding);
    if (from==start) 
    {
      delete gl; 
      break;
    }
    lines.append (gl);
    if (lineTracker) lineTracker->lineInserted (this, lines.size()-1);
  } 
  STextDataEvent nevt (STextIndex(0,0));
  event.add (nevt);
}

/**
 * insert one single glyph and move the cursor to the right
 * @param glyph is the glyph to insert
 */
void
STextData::insert (const SGlyph& glyph)
{

  if (size() <= textIndex.line)
  {
    SParagraph* p = new SParagraph();
    CHECK_NEW (p);
    p->setEmbedding (embedding);
    lines.append (p);
    if (lineTracker) lineTracker->lineInserted (this, lines.size()-1);
  }

  SParagraph* line = lines[textIndex.line];

  line->clearChange();
  line->insert (textIndex.index, glyph);
  if (lineTracker) lineTracker->lineChanged (this, textIndex.line);

  STextDataEvent nevt (STextIndex(textIndex.line, line->getChangeStart())); 
  STextIndex end(textIndex.line, line->getChangeEnd());

  textIndex.index = textIndex.index+1;
  /* there are two cases. we opened a new line or not */
  if (glyph.isEOP())
  {
    /* cut line in two */
    if (line->size() > textIndex.index)
    {
      SParagraph *newLine = new SParagraph (*line);
      newLine->remove (0, textIndex.index);
      line->truncate (textIndex.index);
      if (lineTracker) lineTracker->lineChanged (this, textIndex.line);

      lines.insert (textIndex.line+1, newLine);
      if (lineTracker) lineTracker->lineInserted (this, textIndex.line+1);

      textIndex.index = 0;
      textIndex.line = textIndex.line + 1;
      setMaxLimits (&nevt, textIndex);
    }
    else /* new line */
    {
      textIndex.index = 0;
      textIndex.line = textIndex.line + 1;
      setMaxLimits (&nevt, textIndex);
    }
  }
  else
  {
    setMaxLimits (&nevt, end);
  }
  event.add (nevt);
}


/**
 * Insert a text from utf8. Do fuzzy algorithm to determine
 * direction and all kinds of fuzzy properties, like composition.
 * The resulting text is added to the textbuffer in screen-order.
 * FIXME: 
 * - Implement a bidi algorithm. The current version treats everything
 *   as strong left-to-right character unless it is enclosed in  RLO-PDF.
 * - Get a table of all composing characters and break down the glyphs so
 *   that one glyph contains the base character and the compositng marks.
 * @param utf8 an utf8 encoded text.
 */
void
STextData::insert (const SString& utf8)
{
  SEncoder ic("utf-8-s");
  SV_UCS4 ucs4 = ic.decode (utf8, false);
  /* split into lines */
  unsigned int start = 0;
  unsigned int i;
  STextIndex old = textIndex;
  while (true)
  {
    unsigned int end = start;
    SParagraph gl (ucs4, &start);
    if (start == end) break;
    /* more to come - only lines inserted */
    if (start < ucs4.size() && textIndex.index == 0 && gl.isProperLine())
    {
      STextDataEvent nevt (textIndex);
      SParagraph *p = new SParagraph(gl);
      CHECK_NEW (p);
      p->setEmbedding(embedding);
      lines.insert (textIndex.line, p);
      if (lineTracker) lineTracker->lineInserted (this, textIndex.line);
      textIndex.line++;
      setMaxLimits (&nevt, textIndex);
      event.add (nevt);
      continue;
    }
    for (i=0; i<gl.size(); i++)
    {
      insert (gl[i]);
    }
  } 
  return;
}

/**
 * remove glyphs between current and line and size.
 * current can be less or greater.
 * @param index is the ending high end is non-exclusive
 */
void
STextData::remove (const STextIndex& index)
{
  if (size() == 0) return;

  STextIndex newIndex = reorder (index);
  unsigned int i;
  bool join = false;
  for (i=textIndex.line; i<=newIndex.line && i<size(); i++)
  {
     unsigned int begin = (i==textIndex.line) ? textIndex.index : 0;
     unsigned int end = (i==newIndex.line)? newIndex.index : size (i);
     /* do not check others because it would result in expand */
     if (i==newIndex.line && end > size (i))
     {
       end = size(i);
     }

     SParagraph* line = lines[i];
     if (begin == 0 && end == size(i))
     {
        if (isProperLine(i)) join = true;
        line->clear();
        if (lineTracker) lineTracker->lineChanged (this, i);
     }
     else for (unsigned int j=begin; j<end; j++)
     {
        /* hack. We don't disclose the SGlyphLine= */
        if (j+1==end)
        {
          SGlyph g = glyphAt (STextIndex(i, begin));
          if (g.isEOP()) join = true;
        }
        line->remove (begin);
        if (lineTracker) lineTracker->lineChanged (this, i);
     }
  }
  unsigned int lr = 0;

  /* Remove zeroes */
  for (i=textIndex.line+1; i<=newIndex.line && i<size() + lr; i++)
  {
     SParagraph* line = lines[i-lr];
     if (line->size() == 0)
     {
       lines.remove (i-lr);
       delete line;
       if (lineTracker) lineTracker->lineRemoved (this, i-lr);
       lr++;
     }
  }

  /* now we have everything collapsed. */

  STextDataEvent nevt (textIndex);
  setMaxLimits (&nevt, textIndex);
  event.add (nevt);

  /* a joining line was created */
  if (join)
  {
    /* wanted, but impossible */
    if (textIndex.line+1 == size())
    {
      SParagraph* line = lines[textIndex.line];
      if (line->size()==0)
      {
        lines.remove (textIndex.line);
        delete line;
        if (lineTracker) lineTracker->lineRemoved (this, textIndex.line);
      }
      return;
    }
    SParagraph* gl = lines[textIndex.line+1];
    lines.remove (textIndex.line+1);
    if (lineTracker) lineTracker->lineRemoved (this, textIndex.line+1);

    STextIndex sv = textIndex;
    for (i=0; i<gl->size(); i++)
    {
      insert ((*gl)[i]);
    }
    delete gl;
    move (sv);
  }
  else
  { /* we may not removed this */
    SParagraph* line = lines[textIndex.line];
    if (line->size()==0)
    {
      lines.remove (textIndex.line);
      delete line;
      if (lineTracker) lineTracker->lineRemoved (this, textIndex.line);
    }
  }
}

/**
 * Modify this glyph by adding extra composing characters
 * to it. 
 * @param c is a new composing character to be added.
 * @param toleft is true if we need to add it to the left of cursor.
 * @return true if this was a composing character.
 */
bool
STextData::addComposing(SS_UCS4 c, bool toleft)
{
  if (textIndex.line >= lines.size())
  {
     return false;
  }
  SParagraph* line = lines[textIndex.line];
  SGlyph* g = 0;
  STextIndex fromIndex = textIndex;
  STextIndex toIndex = textIndex;

  if (toleft && textIndex.index > 0 && textIndex.index-1 < line->size())
  {
    g = (SGlyph*) line->peek(textIndex.index-1);
    fromIndex.index = fromIndex.index-1;
  }
  if (!toleft && textIndex.index < line->size() && line->size() > 0)
  {
    g = (SGlyph*) line->peek(textIndex.index);
    toIndex.index = toIndex.index+1;
  }
  /* Hack - this is read only */
  bool status = g ? g->addComposing(c) : false;
  if (status)
  {
    STextDataEvent nevt (fromIndex, false);
    setMaxLimits (&nevt, toIndex);
    event.add (nevt);
  }
  return status;
}

/**
 * @return the removed composing character if any
 * @param toleft is true if we need to remove it from the left of cursor.
 * return 0 if there are no more composing characters.
 */
SS_UCS4
STextData::removeComposing(bool fromleft)
{
  if (textIndex.line >= lines.size())  return false;
  SParagraph* line = lines[textIndex.line];
  SGlyph* g = 0;
  STextIndex fromIndex = textIndex;
  STextIndex toIndex = textIndex;
  if (fromleft && textIndex.index > 0 && textIndex.index-1 < line->size())
  {
    g = (SGlyph*) line->peek(textIndex.index-1);
    fromIndex.index = fromIndex.index-1;
  }
  if (!fromleft && textIndex.index < line->size() && line->size() > 0)
  {
    g = (SGlyph*) line->peek(textIndex.index);
    toIndex.index = toIndex.index+1;
  }
  /* Hack - this is read only */
  SS_UCS4 status = g ? g->removeComposing() : false;
  if (status)
  {
    STextDataEvent nevt (fromIndex, false);
    setMaxLimits (&nevt, toIndex);
    event.add (nevt);
  }
  return status;
}
/**
 * select a region of text and move cursor to it.
 * @param index is the new index. high end is non-exclusive
 * @param is is true if select, unselect otherwise. 
 */
void
STextData::select (const STextIndex& index, bool is)
{
  if (size() == 0) return;

  STextIndex newIndex = reorder (index);
  bool reordered = (newIndex != index);


  if (newIndex.line > size())
  {
     newIndex.line = size(); newIndex.index = size(size()-1);
  }
  for (unsigned int i=textIndex.line; i<=newIndex.line && i<size(); i++)
  {
     if (i>textIndex.line && i<newIndex.line)
     {
       /* select the whole line */
       lines[i]->select (is);
     }
     else
     {
       unsigned int begin = (i==textIndex.line) ? textIndex.index : 0;
       unsigned int end = (i==newIndex.line)? newIndex.index : size (i);
       /* do not check others because it would result in expand */
       if (i==newIndex.line && end > size (i))
       {
         end = size(i);
       }
       lines[i]->select (is, begin, end);
     }
  }
  if (reordered)
  {
    STextDataEvent nevt (textIndex, true);
    setMaxLimits (&nevt, newIndex);
    event.add (nevt);
  }
  else
  {
    STextDataEvent nevt (textIndex, true);
    setMaxLimits (&nevt, newIndex);
    event.add (nevt);
    textIndex =  newIndex;
  }
}

bool
STextData::setParagraphSeparator (const SString& str)
{
  if (str==SS_LB_UNIX) return setParagraphSeparator (SS_PS_LF);
  if (str==SS_LB_MAC) return setParagraphSeparator (SS_PS_CR);
  if (str==SS_LB_DOS) return setParagraphSeparator (SS_PS_CRLF);
  if (str==SS_LB_PS) return setParagraphSeparator (SS_PS_PS);
  return false;
}

/**
 * Set the line type for the whole data structure.
 * @param type will be forced after each newline. 
 * @return true if anything changed.
 */
bool
STextData::setParagraphSeparator (SS_ParaSep sep)
{
  bool retvle = false;
  for (unsigned int i=0; i<lines.size(); i++)
  {
    if (lines[i]->setParagraphSeparator (sep))
    {
      retvle = true;
    }
  }
  return retvle;
}

/**
 * select a region of text and move cursor to it.
 * @param index is the new index - high end is non-exclusive
 * @param is is true if underline, un-underline otherwise.
 */
void
STextData::underline (const STextIndex& index, bool is)
{

  if (size() == 0) return;

  STextIndex newIndex = reorder (index);
  bool reordered = (newIndex != index);

  for (unsigned int i=textIndex.line; i<=newIndex.line && i<size(); i++)
  {
     if (i>textIndex.line && i<newIndex.line)
     {
       /* select the whole line */
       lines[i]->underline (is);
     }
     else
     {
       unsigned int begin = (i==textIndex.line) ? textIndex.index : 0;
       unsigned int end = (i==newIndex.line)? newIndex.index : size (i);
       /* do not check others because it would result in expand */
       if (i==newIndex.line && end > size (i))
       {
         end = size(i);
       }
       lines[i]->underline (is, begin, end);
     }
  }
  if (reordered)
  {
    STextDataEvent nevt (textIndex, true);
    setMaxLimits (&nevt, newIndex);
    event.add (nevt);
  }
  else
  {
    STextDataEvent nevt (textIndex, true);
    setMaxLimits (&nevt, newIndex);
    event.add (nevt);
    textIndex =  newIndex;
  }
}

/**
 * Clear the text
 */
void
STextData::clear ()
{
  for (unsigned int i=0; i<lines.size(); i++)
  {
     delete lines[i];
  }
  lines.clear();
  if (lineTracker)
  {
    lineTracker->lineRemoved (this, 0);
  }
  textIndex = STextIndex (0, 0);
  STextDataEvent nevt (STextIndex (0, 0));
  nevt.remaining = STextIndex (0, 0);
  nevt.valid = true;
  event.add (nevt);
}

void
STextData::fireEvent ()
{
  if (!event.valid || listener == 0) return ;
  listener->textChanged (this, event); 
  event.clear();
}

void
STextData::clearEvent ()
{
  event.clear();
}

/**
 * return the number of lines in text
 */
unsigned int
STextData::size() const
{
  return lines.size();
}

/**
 * return the number of lines in line
 */
unsigned int
STextData::size(unsigned int line) const
{
  if (line >= size()) return 0;
  const SParagraph* l = lines[line];
  // This will expand the line
  //bool exp = l->isExpanded ();
  unsigned int ret = l->size();
  // We are not const
  //STextData* th = (STextData*) this;
  // It is futile to track expanded lines in this class
  //if (!exp && lineTracker) th->lineTracker->lineExpanded (th, line);
  return ret;
}

/**
 * Get the text index relative to offset.
 * @param offset is character offset to textIndex.
 */
STextIndex
STextData::getTextIndex(int charOffset, bool logical) const
{
  return getTextIndex (textIndex, charOffset, logical);
}

#define SD_XBOTH(_a, _b) ((!(_a) && !(_b)) || ((_a) && (_b)))

/**
 * Get the text index relative to offset.
 * for  isLR() lines we increment from index
 * for !isLR() lines we decrement from index
 * @param offset is character offset to textIndex.
 */
STextIndex
STextData::getTextIndex(const STextIndex& base, int charOffset, bool logical) const
{
  STextIndex ret =  base;
  bool ltor = (charOffset > 0);
  unsigned int aco = (charOffset < 0) ? (unsigned int) (-charOffset) 
          : (unsigned int)charOffset;

  /* sanity check */
  if (ret.line > size()) ret.line = size();
  if (ret.index > properSize(ret.line)) ret.index = properSize(ret.line);

  if (charOffset == 0) return STextIndex (ret);

  /* comnvert to remaining */
  if (logical)
  {
    if (ltor) ret.index = properSize(ret.line) - ret.index;
  }
  else if (SD_XBOTH (isLR(ret.line), ltor))
  {
    ret.index = properSize(ret.line) - ret.index;
  }

  /* document embedding is SS_EmbedLeft, if this is lr then up */
  bool isup = SD_XBOTH ((embedding!=SS_EmbedRight), ltor); 
  if (logical) isup = ltor;

  /* multiline - now ret.index contains the remaining */
  while (aco > ret.index)
  {
    if (!isup && ret.line == 0)
    {
      break;
    }
    if (isup && ret.line == size())
    {
      break;
    }
    aco = aco - ret.index;
    if (isup) ret.line++;
    if (!isup) ret.line--;
    aco--;
    ret.index = properSize (ret.line); /* try whole line */
  }

  /* set remaining */
  ret.index =  (aco > ret.index) ? 0 : ret.index - aco;

  /* as ret.index contains the remaining. we need to normalize it back  */
  if (logical)
  {
    if (ltor) ret.index = properSize(ret.line) - ret.index;
  }
  else if (SD_XBOTH (isLR(ret.line), ltor))
  {
    ret.index = properSize(ret.line) - ret.index;
  }

  /* Now we should have the ret.index and re.line set. */
  return STextIndex (ret);
}

/**
 * Check if line ends with newline glyph
 */
bool
STextData::isProperLine (unsigned int line) const
{
  if (line >= size()) return false;
  const SParagraph * p = lines[line];
  return p->isProperLine();
}

/**
 * add data listener.
 * TODO: now it only sets it.
 */
void
STextData::addTextDataListener (STextDataListener* _listener)
{
  listener = _listener;
}

/**
 * Add a line tracker that tracks our line array.
 * TODO: now it only sets it.
 */
void
STextData::addLineTracker (SLineTracker* lt)
{
  lineTracker = lt;
}

/**
 * Set the maximum limits.
 * @param evt -> remainingLine and remainingPosition will be set
 * @param index is the one that needs to be converted to remaining index
 */
void
STextData::setMaxLimits (STextDataEvent* evt, const STextIndex& index)
{
  evt->valid = true;
  unsigned int lsize = size();
  if (index.line >= lsize)
  {
     evt->setRemaining (STextIndex (0,0));
     return;
  }
  unsigned int rem  = lsize - index.line - 1;
  unsigned int cindex = size(index.line);
  evt->setRemaining (STextIndex (rem, (index.index>=cindex) 
       ? 0 : cindex - index.index));
}

/**
 * Convert remainin-line to maxline
 */
STextIndex
STextData::getMaxTextIndex (const STextDataEvent& evt) const
{
  if (!evt.valid) return STextIndex (0,0);
  unsigned int lsize = size();

  if (evt.remaining.line >= lsize)
  {
     return STextIndex (lsize,0);
  }
  unsigned int line = lsize - evt.remaining.line-1;
  const SParagraph* p = lines[line];
  if (evt.remaining.index >= p->size())
  {
     return STextIndex (line, p->size());
  }
  /* should be 1 more for upper limit */
  unsigned int max  = p->size()+1-evt.remaining.index;
  return STextIndex (line,  max);
}

/**
 * Convert remainin-line to maxline
 */
STextIndex
STextData::getMinTextIndex (const STextDataEvent& evt) const
{
  if (!evt.valid) return STextIndex (0,0);
  unsigned int lsize = size();

  if (evt.start.line >= lsize)
  {
     return STextIndex (lsize,0);
  }
  const SParagraph* p = lines[evt.start.line];
  if (evt.start.index >= p->size())
  {
     return STextIndex (evt.start.line, p->size());
  }
  /* should be 1 more for upper limit */
  unsigned int min  = evt.start.index;
  return STextIndex (evt.start.line, min);
}

/**
 * Put the stuff in order, so that it will always be in increasing order.
 * @param end is the desired end
 */
STextIndex
STextData::reorder (const STextIndex& index)
{
  if (index > textIndex)
  {
     return STextIndex (index);
  }
  STextIndex tmp = textIndex;
  textIndex = index;
  return STextIndex (tmp);
}  

/**
 * return true if character is a whitespace
 */
bool
STextData::isWhiteSpace (const STextIndex& index) const
{
  return glyphAt (index).isWhiteSpace();
}


/**
 * return true if character is a number. Addition by Maarten van Gompel <proycon@anaproy.homeip.net>
 */
bool
STextData::isNumber (const STextIndex& index) const
{
  return glyphAt (index).isNumber();
}

/**
 * Used in syntax highlighting.
 */
bool
STextData::isLetter (const STextIndex& index) const
{
  return glyphAt (index).isLetter();
}

/**
 * return true if character is a target for select. Addition by Maarten van Gompel <proycon@anaproy.homeip.net>
 */
bool
STextData::isDelimiter (const STextIndex& index) const
{
  return glyphAt (index).isDelimiter();
}

// previously we used isDelimiter to decide weather the word can be broken
// at this point. Now I added CJK and KANA because they can wrap any time.
bool
STextData::canWrap (const STextIndex& index) const
{
  return glyphAt (index).canWrap();
}

unsigned int
STextData::properSize(unsigned int line) const
{
  if (line >= lines.size()) return 0;
  return lines[line]->properSize();
}

/**
 * pass an array, this array contains the positions after which
 * linebreaks should occur.
 */
void
STextData::setLineBreaks (unsigned int line, const SV_UCS4& breaks)
{
  if (line >= lines.size()) return;
  lines[line]->setLineBreaks(breaks);
}

unsigned int
STextData::toLogical (unsigned int line, unsigned int index)
{
  if (line >= lines.size()) return 0;
  return lines[line]->toLogical (index);
}

SV_UINT
STextData::getLogicalMap(unsigned int line) const
{
  if (line >= lines.size()) return SV_UINT();
  return SV_UINT(lines[line]->getLogicalMap());
}

bool
STextData::isVisible(unsigned int line) const
{
  if (line >= size()) return false;
  return lines[line]->isVisible();
}
void
STextData::setVisible(unsigned int line)
{
  if (line >= size()) return;
  lines[line]->setVisible();
}
bool
STextData::isReordered (unsigned int line) const
{
  if (line >= size()) return false;
  return lines[line]->isReordered();
}
void
STextData::setReordered(unsigned int line)
{
  if (line >= size()) return;
  lines[line]->setReordered();
}
void
STextData::setDocumentEmbedding(SS_Embedding e)
{
  embedding = e;
  if (size() ==0) return;
  for (unsigned int i=0; i<size(); i++)
  {
    lines[i]->setEmbedding(e);
  }
  STextDataEvent nevt (STextIndex(0,0));
  event.add (nevt);
}

SS_Embedding
STextData::getDocumentEmbedding() const
{
  return embedding;
}

/**
 * Check if current paragraph is rendered lr (left aligned) or
 * rl (right aligned).
 */
bool
STextData::isLR (unsigned int parag) const
{
  if (parag >= lines.size()) return embedding != SS_EmbedRight;
  return lines[parag]->isLR();
}

/**
 * return the directionality of this character.
 * @return true if the previous character is LR.
 */
bool
STextData::isLR (const STextIndex& index) const
{
  if (index.line >= lines.size()) return embedding != SS_EmbedRight;
  if (index.index >= lines[index.line]->size())
  {
    return STextData::isLR(index.line);
  }
  return lines[index.line]->peek(index.index)->isLR();
}

/**
 * Return the explicit embed state.
 */ 
SEmbedState
STextData::getEmbedState (const STextIndex& index) const
{
  SEmbedState state;
  if (index.line >= lines.size())
  {
    return SEmbedState(state);
  }
  if (index.index >= lines[index.line]->size())
  {
    return SEmbedState(state);
  }
  return SEmbedState (lines[index.line]->peek(index.index)->getEmbedState());
}
