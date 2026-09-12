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

#include "stoolkit/SParagraph.h"
#include "stoolkit/SUniMap.h"
#include "stoolkit/SCluster.h"
#include "stoolkit/SEncoder.h"
#include "stoolkit/SBiDi.h"

#define SD_MAX_COMPOSE 100
#define SD_MAX_EMBEDDING 61

static unsigned int 
split(const SV_UCS4& ucs4, SVector<SGlyph>* gl, unsigned int from);

SParagraph::SParagraph(void)
{
  visible = false;
  selected = false;
  underlined = false;
  expanded = true;
  iniLevel = 0;
  embedding = SS_EmbedNone;
  paraSep = SS_PS_None;
  reordered = true;
  clearChange();
}

SParagraph::SParagraph(const SParagraph& paragraph)
{
  visible = paragraph.visible;
  selected = paragraph.selected;
  underlined = paragraph.underlined;
  expanded = paragraph.expanded;
  embedding = paragraph.embedding;
  iniLevel = paragraph.iniLevel;

  glyphs = paragraph.glyphs;
  ucs4Glyphs = paragraph.ucs4Glyphs;

  paraSep = paragraph.paraSep;
  lineBreaks = paragraph.lineBreaks;
  clearChange();

  reordered = true;
}

/**
 * Lift off one paragraph from vector starting from start
 * This is the only constructor that works on SV_UCS4.
 * at the end the line size is zero.
 */
SParagraph::SParagraph(const SV_UCS4& buffer, unsigned int *index)
{
  visible = false;
  selected = false;
  underlined = false;
  expanded = false;

  paraSep = SS_PS_None;
  reordered = true;
  iniLevel = 0;
  embedding = SS_EmbedNone;

  unsigned int start = *index;
  unsigned int end = buffer.size();
  unsigned int i = start;
  bool eol = false;
  /* a quick split - well, could be quicker :) */
  while (i<end && !eol) 
  {
    switch (buffer[i])
    {
    case SD_CD_CR:
      ucs4Glyphs.append (buffer[i++]);
      paraSep = SS_PS_CR;
      if (i<end && buffer[i] == SD_CD_LF)
      {
        paraSep = SS_PS_CRLF;
        ucs4Glyphs.append (buffer[i++]);
      }
      eol = true;
      break;
    case SD_CD_LF:
      ucs4Glyphs.append (buffer[i++]);
      paraSep = SS_PS_LF;
      eol = true;
      break;
    case SD_CD_PS:
      ucs4Glyphs.append (buffer[i++]);
      paraSep = SS_PS_PS;
      eol = true;
      break;
    default:
      ucs4Glyphs.append (buffer[i++]);
    }
  }
  *index = i;
}

SParagraph::~SParagraph()
{
}

/**
 * This routine checks if the text buffer has been converted into
 * a glyph buffer, and converts it if necessary.
 */
void
SParagraph::expand() const // Well, not....
{
  if (expanded) return; /* already expanded */

  /* split the lines and do stoolkit composition */
  SParagraph* p = (SParagraph*) this; /* we are not const */
  SVector<SGlyph> gl;

  SV_UCS4 chrs = getChars();
  unsigned int end = split (chrs, &gl, 0);

  if (end != chrs.size()) 
  {
    fprintf (stderr, "SParagraph::expand() internal error end=%d, size=%d\n", 
        end, chrs.size());
  }

  p->ucs4Glyphs.clear();
  p->expanded = true;
  if (selected)
  {
    for (unsigned int i=0; i<gl.size(); i++)
    {
      /* hack. We don't disclose the SGlyphLIne= */
      SGlyph* g = (SGlyph*) gl.peek(i);
      g->selected = selected;
    }
  }
  if (underlined)
  {
    for (unsigned int i=0; i<gl.size(); i++)
    {
      /* hack. We don't disclose the SGlyphLIne= */
      SGlyph* g = (SGlyph*)gl.peek (i);
      g->underlined = underlined;
    }
  }
  
  p->glyphs  = gl; 
  p->clearChange();
  p->reordered = true;
  p->reShape (); 

  /* Yudit may add 202C before end of paragraph to fix 
     unmatched embedding and override
     U+202E u+2041 U+000A 
     becomes 
     U+202E u+0041 202C U+000A 
     It also can get rid of unpaired ones.
     therefore getChars().size() sometimes != chrs.size();
   */

}

/**
 * try to shape a single segment.
 * return true if it is possible.
 * @param before is the visual glyph before this line.
 * @param after is the visual glyph after this line.
 * Todo: do glyphline-breaking of shaping is not possible, use
 * lineShaper to get glyph breakdown.
 */
void
SParagraph::reShape ()
{
  expand();
  unsigned int gsize = size();
  const SGlyph* before;
  const SGlyph* after;
  for (unsigned int i=0; i<gsize; i++)
  {
    SGlyph* g = (SGlyph*) peek(i);
    if (g->getShapeArray()==0
      && getLigatureScriptCode (g->getChar())!=SD_BENGALI)
    {
      continue;
    }
    int cbefore = getNonTransparentBefore (i);
    int cafter = getNonTransparentAfter (i);

    before = (cbefore >= 0) ? peek((unsigned int)cbefore) : 0;
    after = (cafter >= 0) ? peek((unsigned int)cafter) : 0;
    if (g->setShape (before, after))
    {
      setChange (i, i+1);
    }
  }
  return;
}

/**
 * try to reShape at glypgh
 */
void
SParagraph::reShape (unsigned int index)
{
  if (size()==0) return;
  unsigned int i = index;
  if (i>=size()) i=size()-1;

  SGlyph* g = (SGlyph*) peek(i);
  const SGlyph* before;
  const SGlyph* after;

  int cbefore = getNonTransparentBefore (i);
  int cafter = getNonTransparentAfter (i);

  before = (cbefore >= 0) ? peek((unsigned int)cbefore) : 0;
  after = (cafter >= 0) ? peek((unsigned int)cafter) : 0;

  if (g->setShape (before, after))
  {
    setChange (i, i+1);
  }
  /* reShape previous */
  if (cbefore >= 0)
  {
    i = (unsigned int) cbefore;
    g = (SGlyph*) peek(i);
    if (g->getShapeArray()!=0
      || getLigatureScriptCode (g->getChar())==SD_BENGALI)
    {
      int cb2 = getNonTransparentBefore (i);
      int ca2 = getNonTransparentAfter (i);


      before = (cb2 >= 0) ? peek((unsigned int)cb2) : 0;
      after = (ca2 >= 0) ? peek((unsigned int)ca2) : 0;

      if (g->setShape (before, after))
      {
        setChange (i, i+1);
      }
    }
  }
  /* reShape after */
  if (cafter >= 0)
  {
    i = (unsigned int) cafter;
    g = (SGlyph*) peek(i);
    if (g->getShapeArray()!=0
      || getLigatureScriptCode (g->getChar())==SD_BENGALI)
    {
      int cb2 = getNonTransparentBefore (i);
      int ca2 = getNonTransparentAfter (i);
      before = (cb2 >= 0) ? peek((unsigned int)cb2) : 0;
      after = (ca2 >= 0) ? peek((unsigned int)ca2) : 0;
      if (g->setShape (before, after))
      {
        setChange (i, i+1);
      }
    }
  }
}

/**
 * Get the first non-spacing glyph left visual order in array, 
 * starting here and including this.
 * return the index or -1 if not found.
 */
int 
SParagraph::getNonTransparentBefore(unsigned int index) const
{
 if (index > size()) return -1;
 for (int i=((int)index) - 1; i >= 0; i--)
 {
   const SGlyph* g = peek((unsigned int)i);
   if (!g->isTransparent()) return i; 
 }
 return -1;
}

/**
 * Get the first non-spacing glyph right, in visual order, 
 * starting here and including this.
 * return the index or -1 if not found.
 */
int 
SParagraph::getNonTransparentAfter(unsigned int index) const
{
 for (int i=index +1; i < (int)size(); i++)
 {
   const SGlyph* g = peek((unsigned int)i);
   if (!g->isTransparent()) return i; 
 }
 return -1;
}

/**
 * Get a sub paragraph.
 */
SParagraph* 
SParagraph::subParagraph(unsigned int from, unsigned int to) const
{
  expand();
  /* return self */
  SParagraph* p = new SParagraph ();
  CHECK_NEW (p);

  p->visible = visible;
  p->underlined = underlined;
  p->expanded = true;
  p->paraSep = SS_PS_None;
  p->selected = selected;
  p->iniLevel = iniLevel;
  p->embedding = embedding;
  p->reordered = true;

  if (from > to) return p;

  p->glyphs = glyphs;
  p->logical = logical;

  if (from == 0 && to == size())
  {
    return p;
  }
  /* set the initial directionality here */
  if (to<p->size()) p->glyphs.truncate (to);
  if (from>0) p->glyphs.remove (0, from);
  p->reShape (); 
  return p;
}

/**
 * convert the paragraph to ucs4 string
 */
SV_UCS4
SParagraph::getChars() const
{
  SV_UCS4 ret;
  if (!expanded)
  {
    unsigned int len = ucs4Glyphs.size();
    ret = ucs4Glyphs;
    if (paraSep == SS_PS_None) {
      return SV_UCS4(ret);
    }
    else if (len >= 2 && ret[len-2] == SD_CD_CR && ret[len-1] == SD_CD_LF)
    {
      /* no change */
      if (paraSep == SS_PS_CRLF)  return SV_UCS4(ret);
      ret.truncate (len-2);
    }
    else if (len >= 1 && ret[len-1] == SD_CD_LF)
    {
      /* no change */
      if (paraSep == SS_PS_LF)  return SV_UCS4(ret);
      ret.truncate (len-1);
    }
    else if (len >= 1 && ret[len-1] == SD_CD_CR)
    {
      /* no change */
      if (paraSep == SS_PS_CR)  return SV_UCS4(ret);
      ret.truncate (len-1);
    }
    else if (len >= 1 && ret[len-1] == SD_CD_PS)
    {
      /* no change */
      if (paraSep == SS_PS_PS)  return SV_UCS4(ret);
      ret.truncate (len-1);
    } else { /* no eol */
      return SV_UCS4(ret);
    }
    /* we are here because we need to append paraSep */
    switch (paraSep)
    {
    case SS_PS_LF:
      ret.append (SD_CD_LF);
      break;
    case SS_PS_CR:
      ret.append (SD_CD_CR);
      break;
    case SS_PS_CRLF:
      ret.append (SD_CD_CR);
      ret.append (SD_CD_LF);
      break;
    case SS_PS_PS:
      ret.append (SD_CD_PS);
      break;
    default:
      return SV_UCS4(ret);;
    }
    return SV_UCS4(ret);;
  }
  const SGlyph* g0 = 0; 
  for (unsigned int i=0; i<size(); i++)
  {
    const SGlyph* g = glyphs.peek(i);
    ret.append (g->getEmbeddingMarks(g0));
    ret.append (g->getChars());
    g0 = g;
  }
  if (g0 != 0)
  {
     SV_UINT m = g0->getEmbeddingMarks (0);
     unsigned int i = m.size();
     while (i--) ret.append (SD_CD_PDF);
  }
  return SV_UCS4(ret);
}


/**
 * Set the initial embedding level
 */
void
SParagraph::setIniLevel ()
{
  expand();
  if (embedding!=SS_EmbedNone)
  {
    iniLevel = (embedding != SS_EmbedRight)?0:1; 
    return;
  }

  SD_BiDiClass dbclass = SD_BC_XX;
  unsigned int i=0;
  for (i=0; i<size(); i++)
  {
    dbclass = glyphs[i].getBiDiType();
    if (dbclass==SD_BC_L || dbclass==SD_BC_R || dbclass==SD_BC_AL) break;
  }
  if (dbclass==SD_BC_L)
  {
    iniLevel = 0;
  }
  else if (dbclass==SD_BC_R || dbclass==SD_BC_AL)
  {
    iniLevel = 1;
  }
  else
  {
    iniLevel = 0;
  }
}

/**
 * Resolve the levels between from and until (non-inclusive)
 */
void
SParagraph::resolveLevels()
{
  setIniLevel();
  unsigned int lsize = size();
  if (lsize==0) return;

  SBiDi impBiDi(iniLevel, lsize);

  /* build impBiDi */
  unsigned int i;
  unsigned int j;

  for (i=0; i<lsize; i++)
  {
    SGlyph *g = (SGlyph*) peek (i);
    if (iniLevel==0)
    {
      g->embedding = g->getExplicitLevel();
    }
 /* move initLevel = something else - 1 */
    else if (g->getExplicitLevel() != 0)
    {
      g->embedding = g->getExplicitLevel()+2;
    }
    else /* for not embedded, force our paragraph embedding */
    {
      g->embedding = g->getExplicitLevel()+1;
    }
    SD_BiDiClass bcls = g->getBiDiType();
    if (g->isOverride())
    {
      bcls = ((g->embedding %2)==0) ? SD_BC_L : SD_BC_R;
    } 
    /* we wont have LRE RLE RLO LRO and PDF codes here so it is
      anough to skip BN */
    if (bcls != SD_BC_BN)
    {
      impBiDi.append (g->embedding, bcls);
    }
  }

  impBiDi.resolveWeakNeutral();

  /* add back BN codes that we have not inserted */
  for (i=0; i<lsize; i++)
  {
    SGlyph *g = (SGlyph*) peek (i);
    /* we must put only the ones into impBiDi what the cycle above amitted */
    if (g->isOverride()) continue;
    SD_BiDiClass bcls = g->getBiDiType();
    if (bcls == SD_BC_BN)
    {
      impBiDi.insertBN(i);
    }
  }

  /* Resolving Implicit Embedding Levels */
  for (i=0; i<lsize; i++)
  {
    SGlyph *g = (SGlyph*) peek (i);
    SD_BiDiClass bdclass = impBiDi.getBiDiType(i);
    bool curlr = (g->embedding % 2)==0;
    
    if (curlr)
    {
      if (bdclass == SD_BC_R)
      {
        g->embedding = g->embedding +1;
      }
      if (bdclass == SD_BC_AN || bdclass == SD_BC_EN)
      {
        g->embedding = g->embedding +2;
      }
    }
    else
    {
      if (bdclass == SD_BC_L || bdclass == SD_BC_EN || bdclass == SD_BC_AN)
      {
        g->embedding = g->embedding +1;
      }
    }
  }

  /* currently we wont break before all SD_BC_S - we just do the alrgorithm. */
  for (i=0; i<lsize; i++)
  {
    SGlyph *g = (SGlyph*) peek (i);
    /* we should use the original type */
    SD_BiDiClass bdclass  = g->getBiDiType();
    if (bdclass==SD_BC_S) /* segment separator */
    {
      g->embedding = (char) iniLevel;
      for (j=i; j>0; j--)
      {
        SGlyph *g1 = (SGlyph*) peek (j-1);
        SD_BiDiClass c = g1->getBiDiType();
        if (c != SD_BC_WS && c != SD_BC_BN) break;
        g1->embedding = (char) iniLevel;
      }
    }
  }
  /* physical ending of line - lines with or without separators */
  for (j=lsize; j>0; j--)
  {
    SGlyph *g1 = (SGlyph*) peek (j-1);
    SD_BiDiClass c = g1->getBiDiType();
    if (c!=SD_BC_WS && c!=SD_BC_BN && !g1->isEOL() && !g1->isEOP()) break;
    g1->embedding = (char) iniLevel;
  }
   
  /* Set whitespaces before end of *line* to initLevel */
  for (i=0; i<lineBreaks.size(); i++)
  {
    unsigned int lb = lineBreaks[i];
    if (lb > lsize) lb = lsize;
    while (lb-- > 0)
    {
      SGlyph *g = (SGlyph*) peek (lb);
      SD_BiDiClass c  = g->getBiDiType();
      if (c!=SD_BC_WS && c!=SD_BC_BN) break;
      g->embedding = (char) iniLevel;
    }
  }
}

/**
 * Put the text back into logical order.
 * @param index is the visual index.
 */
unsigned int
SParagraph::toLogical (unsigned int index)
{
  expand();
  if (!reordered) return logical[index];

  unsigned int ccount = size();
  if (ccount == 0) return 0;

  /* resolve embedding levels */
  resolveLevels();

  unsigned int i;

  /* we create this array to make things faster */
  SS_UINT* logindex = new SS_UINT[ccount];
  CHECK_NEW(logindex);

  unsigned int biggest = 0;
  /* last one will map to last */
  for (i=0; i<ccount; i++)
  {
    SGlyph* g = (SGlyph*) peek(i);
    if ((unsigned int) g->embedding > biggest)
    { 
      biggest = (unsigned int) g->embedding;
    }
    logindex[i] = i;
  }

  /* go through embedding levels and reverse them for each line */
  for (unsigned int e=biggest; e>iniLevel; e--)
  {
    for (i=0; i<ccount; i++)
    {
      SGlyph* g = (SGlyph*) peek(i);
      if ((unsigned int)g->embedding >= e)
      {
        /* non-enclusive index */
        unsigned int j = i;
        while (++j<ccount)
        {
          SGlyph* g = (SGlyph*) peek(j);
          if ((unsigned int)g->embedding < e || isLineBreak(j))
          {
            j--; break;
          }
        }
        if (j >= ccount) j--;

        for (unsigned int k=0; k<=(j-i)/2; k++)
        {
          SS_UINT a0 = logindex[i+k];
          SS_UINT a1 = logindex[j-k];
          logindex[i+k] = a1;
          logindex[j-k] = a0;
        }
        i = j;
      }
    }
  }
  /* create the array that converts from logindex to visual */
  /* last one will map to last */
  logical.clear();
  for (i=0; i<ccount; i++)
  {
    logical.append (logindex[i]);
  }
  delete[] logindex;
  reordered = false;
  return logical[index];
}
/**
 * Check if line breaks before 'before'
 */
bool
SParagraph::isLineBreak(unsigned int before) const
{
  for (unsigned int i=0; i<lineBreaks.size(); i++)
  {
    if (before == lineBreaks[i]) return true;
  }
  return false;
}

/**
 * Check if line ends with newline glyph
 */
bool
SParagraph::isProperLine () const
{
  if (!expanded) return paraSep;
  if (glyphs.size() == 0 ) return false;
  return glyphs[glyphs.size()-1].isEOP();
}

/*
 * size without EOL
 */
unsigned int
SParagraph::properSize() const
{
  expand();
  return (isProperLine() ? glyphs.size()-1 : glyphs.size());
}

/**
 * Set new linebreaks. it also modifies the tovisual array.
 */
void
SParagraph::setLineBreaks (const SV_UCS4& breaks)
{
  lineBreaks = breaks;
}

void
SParagraph::clear()
{
  expand();
  iniLevel = 0;
  glyphs.clear();
  reordered = true;
  setChange (0, 0);
}

void
SParagraph::insert(unsigned int into, const SGlyph& glyph)
{
  expand();
  glyphs.insert (into, glyph);
  setChange (into, into+1);
  reShape(into);
  reordered = true;
}


void
SParagraph::append(const SGlyph& glyph)
{
  expand();
  glyphs.append (glyph);
  setChange (size()-1, size());
  reShape(size()-1);
  reordered = true;
}

void
SParagraph::remove(unsigned int from, unsigned int to)
{
  expand();
  glyphs.remove (from, to);
  setChange (from, from+1);
  reShape(from);
  reordered = true;
}

void
SParagraph::remove(unsigned int at)
{
  expand();
  glyphs.remove (at);
  setChange (at, at+1);
  reShape (at);
  reordered = true;
}

void
SParagraph::truncate(unsigned int to)
{
  expand();
  glyphs.truncate (to);
  setChange (to, to+1);
  reShape(to);
  reordered = true;
}

void
SParagraph::replace(unsigned int at, const SGlyph& glyph)
{
  expand();
  glyphs.replace (at, glyph);
  setChange (at, at+1);
  reShape(at);
  reordered = true;
}

/**
 * Return true if the the text is supposed to be rendered 
 * from left to right.
 */
bool
SParagraph::isLR() const
{
  return ((iniLevel%2)==0);
}

/**
 * Mark this change
 * @from is the text change start
 * @to is the text change end (not including)
 */
void
SParagraph::setChange(unsigned int from, unsigned int to)
{
  if (changeStart > from) changeStart = from;
  if (to > size())
  {
    changeRemaining = 0;
  }
  else if (changeRemaining > (size() - to))
  {
    changeRemaining = (size() - to);
  }
}

/**
 * clear the change indeces for events
 */
void
SParagraph::clearChange()
{
  changeStart = size();
  changeRemaining = 0;
}

/**
 * return the first index since clearChange
 * 0 means change is from beginning
 */
unsigned int
SParagraph::getChangeStart() const
{
  return changeStart;
}

/**
 * return the last index since clearChange - from the end
 * 0 means change is till end
 */
unsigned int
SParagraph::getChangeEnd() const
{
  if (changeRemaining >= size()) return size();
  return size() - changeRemaining;
}
bool
SParagraph::isVisible() const
{
  return visible;
}
void
SParagraph::setVisible()
{
  visible = true;
}
void
SParagraph::setReordered()
{
  reordered = true;
}

/**
 * Set the paragraph separator character, usually found at
 * the end of the paragraph.
 * @param glyph is the separator glyph.
 * @return true if it has changed.
 */
bool
SParagraph::setParagraphSeparator (SS_ParaSep ps)
{
  if (!isProperLine()) return false;
  if (!expanded)
  {
     SS_ParaSep old = paraSep;
     paraSep = ps;
     return (old!=ps);
  }
  SGlyph oldg = glyphs[glyphs.size()-1];
  SGlyph newg = oldg;
  SV_UCS4 v;
  switch (ps)
  {
  case SS_PS_LF:
    v.append (SD_CD_LF);
    newg = SGlyph(v, SD_CD_LF, false, 0, 0, false);
    break;
  case SS_PS_CR:
    v.append (SD_CD_CR);
    newg = SGlyph(v, SD_CD_CR, false, 0, 0, false);
    break;
  case SS_PS_CRLF:
    v.append (SD_CD_CR);
    v.append (SD_CD_LF);
    newg = SGlyph(v, 0, false, 0, 0, false);
    break;
  case SS_PS_PS:
    v.append (SD_CD_PS);
    newg = SGlyph(v, SD_CD_PS, false, 0, 0, false);
    break;
  default:
    return false;
  }
  if (oldg == newg) return false;
  newg.embedding = oldg.embedding;
  newg.selected = oldg.selected;
  glyphs.replace (glyphs.size()-1, newg);
  return true;
}


/**
 * @return true if the visual ordering has been altered.
 */
bool
SParagraph::isReordered () const
{
  /* this is re-set by toVisual() */
  return reordered;
}

/**
 * Set document embedding level
 * @param e is external embedding.
 */
void
SParagraph::setEmbedding(SS_Embedding e)
{
  reordered = true;
  embedding = e;
}

/**
 * Select the whole paragraph.
 */
void
SParagraph::select (bool is)
{
  if (!expanded) 
  {
    selected = is;
    return;
  }
  select (is, 0, size());
}

/**
 * set the selected flags
 * @param is is true if selecting false if de-selecting.
 * @param from is the starting index
 * @param to is the ending index (non-inclusive)
 */
void
SParagraph::select (bool is, unsigned int from, unsigned int to)
{
   unsigned int end = to;
   if (end > size()) end = size(); 
   for (unsigned int i=from; i<end; i++)
   {
      /* hack. We don't disclose the SGlyphLIne= */
      SGlyph* g = (SGlyph*) peek(i);
      g->selected = is;
   }
}


/**
 * Underline the whole paragraph.
 */
void
SParagraph::underline (bool is)
{
  if (!expanded) 
  {
    underlined = true;
    return;
  }
  underline (is, 0, size());
}
/**
 * set the underline flags
 * @param is is true if we are underlining false if un-underlining.
 * @param from is the starting index
 * @param to is the ending index (non-inclusive)
 */
void
SParagraph::underline (bool is, unsigned int from, unsigned int to)
{
   unsigned int end = to;
   if (end > size()) end = size(); 
   for (unsigned int i=from; i<end; i++)
   {
      /* hack. We don't disclose the SGlyphLIne= */
      SGlyph* g = (SGlyph*) peek(i);
      g->underlined = is;
   }
}

/**
 * Split a text into Glyphs
 * @param ucs4 is a text that can contain paragraph separators
 * @param from is the starting index.
 * @param gl is the return Glyphs.
 * @return ending index, that is equal to starting index 
 * if there is no more data.
 */
static unsigned int
split (const SV_UCS4& ucs4, SVector<SGlyph>* gl, unsigned int from)
{
  SUniMap composer ("precompose1");
  SUniMap shaper ("shape");

  gl->clear();
  unsigned int i=from;
  SS_UCS4 composition;
  bool    usePrecomposed;
  bool    isShaped;
  bool    isLineEnd;
  //gl->ensure (ucs4.size());
  unsigned int compIndex = 0;

  SV_UCS4   stack;
  while (i<ucs4.size())
  {
    unsigned int n =0;
    bool      embed=false;
    SV_UCS4 ret;
    composition = 0;
    compIndex = 0;
    usePrecomposed = false;
    isShaped = false;
    isLineEnd = true;
    unsigned int clusterIndex = 0;
    SV_UCS4 du4;
    switch (ucs4[i])
    {
    case SD_CD_RLO:
    case SD_CD_LRO:
    case SD_CD_RLE:
    case SD_CD_LRE:
    case SD_CD_PDF:
      embed = true;
      break;
    case SD_CD_CR:
      du4.append (ucs4[i++]);
      if (i<ucs4.size() && ucs4[i] == SD_CD_LF)
      {
        du4.append (ucs4[i++]);
      }
      break;
    case SD_CD_LF:
      du4.append (ucs4[i++]);
      break;
    case SD_CD_PS:
      du4.append (ucs4[i++]);
      break ;
    case SD_CD_FF:
      du4.append (ucs4[i++]);
      // Prevent adding composing characters to it.
      break;
    case SD_CD_LS:
      du4.append (ucs4[i++]);
      // Prevent adding composing characters to it.
      break;
    default:
      isLineEnd = false;
      if (shaper.isOK())
      {
        /* encode shapes if possible */
        unsigned int n = shaper.lift (ucs4, i, false, &ret);
        /* the composition comes at the end  - if any */
        if (n>=i+1 && ret.size()==4)
        {
          /* Try to identify composing ligatures and take them out */
          bool isCompLig = false;
          if (n < ucs4.size())
          {
            SS_UCS2 u2=getCharClass(ucs4[n]);
            if (u2 == SD_CC_Mn || u2 == SD_CC_Me)
            {
               SV_UCS4 try2;
               SV_UCS4 comb;
               unsigned int j;
               for (j=i; j<n; j++)
               {
                 try2.append (ucs4[j]);
               }
               /* skip combining */
               while (j+1<ucs4.size() && (u2 == SD_CC_Mn || u2 == SD_CC_Me))
               {
                 comb.append (ucs4[j]);
                 j++;
                 u2=getCharClass(ucs4[j]);
               }
               if (j<ucs4.size())
               {
                 try2.append (ucs4[j]);
                 j++; 
                 if (j<ucs4.size())
                 {
                   u2=getCharClass(ucs4[j]);
                   while (u2 != SD_CC_Mn && u2 != SD_CC_Me)
                   {
                     try2.append (ucs4[j]);
                     j++;
                     /* we will handle only 6 */
                     if (try2.size() > 6 || j>=ucs4.size()) break;
                     u2=getCharClass(ucs4[j]);
                   }
                 }
                 SV_UCS4 ret2;
                 unsigned int n2 = shaper.lift (try2, 0, false, &ret2);
                 /* found a ligature composition */
                 if (n2 > 1 && ret2.size() ==4)
                 {
                   /* add for all shapes */
                   SV_UCS4 cm; cm.append (ret2[0]); cm.append (comb);
                   /* FIXME: the first parameter should be memory 
                     representation. We are sloppy because no one is using it.*/
                   SS_UCS4 l0 = (cm[0]==0) ? 0 : addCombiningLigature (
                      cm.array(), cm.size(), cm.array(), cm.size());

                   cm.replace (0, ret2[1]);
                   SS_UCS4 l1 = (cm[0]==0) ? 0 : addCombiningLigature (
                      cm.array(), cm.size(), cm.array(), cm.size());

                   cm.replace (0, ret2[2]);
                   SS_UCS4 l2 = (cm[0]==0) ? 0 : addCombiningLigature (
                      cm.array(), cm.size(), cm.array(), cm.size());

                   cm.replace (0, ret2[3]);
                   SS_UCS4 l3 = (cm[0]==0) ? 0 : addCombiningLigature (
                      cm.array(), cm.size(), cm.array(), cm.size());

                   n = i+n2+comb.size();
                   ret.clear();
                   ret.append(l0);
                   ret.append(l1);
                   ret.append(l2);
                   ret.append(l3);
                   isCompLig = true;
                   /* combining marks are part of fallback */
                   addFallbackShapes (&shaper, ret2.array(), 
                      &ucs4.array()[i], n-i);
                 }
               }
            }
          }
          composition =  ucs4[i];
          /* The four variants for shaping */
          du4.append (ret[0]);
          du4.append (ret[1]);
          du4.append (ret[2]);
          du4.append (ret[3]);
          if (n>i+1 || isCompLig) // composed of several
          {
            usePrecomposed = false;
            composition = 0;
            /* append the extra bits at the end. */
            for (unsigned int j=i; j<n; j++) du4.append (ucs4[j]);

            /* this will make sure we will have something to 
               display if any of the shapes fails to render */
            if (!isCompLig)
            {
              addFallbackShapes (&shaper, du4.array(), 
                  &du4.array()[4], n-i);
            }
          }
          else /* n == i+1: only one character read */
          {
            usePrecomposed = true;
            /* we still have some extra characters. Is it a composition? */
            if (i+1<ucs4.size())
            {
              SV_UCS4 retc;
              /* good to have - for caching ...*/
              unsigned int np = composer.lift (ucs4, i, false, &retc);
              if (np>i+1 && retc.size() == 1)
              {
                composition = retc[0];
                usePrecomposed = false;
                ret.clear();
                /* replace the shapes for this newly composed glyph */
                unsigned int ns = shaper.lift (retc, 0, false, &ret);
                if (ns==1 && ret.size()==4)
                {
                  du4.replace (0, ret[0]);
                  du4.replace (1, ret[1]);
                  du4.replace (2, ret[2]);
                  du4.replace (3, ret[3]);
                }
                /* the composition comes at the end. */
                for (unsigned int j=i; j<np; j++) du4.append (ucs4[j]);
                n = np;
              }
            }
             /*  decompositions, if any */ 
            if (usePrecomposed) /* set if we decomposed this */
            {
              SV_UCS4 retc;
              unsigned int nd = composer.lift (ucs4, i, true, &retc);
              if (nd == i+1) /* fill in decomposition */
              {
                for (unsigned int j=0; j<retc.size(); j++) du4.append (retc[j]);
              }
            }
          }
          i = n;
          isShaped = true;
          ret.clear();
          break;
        }
        ret.clear();
      }
      /* check for clusters */
      if ((n = getCluster (ucs4, i, &ret)) > i
          && ret.size() > 0 && ret[0] < 0x80000000)
      {
         unsigned int j;
         for (j=0; j<ret.size(); j++)
         {
           if (ret[j] > 0x80000000 && ret[j] < 0xA0000000)
           {
              composition = ret[j];
              /* Save this unicode ligature. Fonts have unicode order.  */
              putLigatureCluster (composition, du4.array(), du4.size());
              putLigatureUnicode (composition, &ucs4.array()[i], n-i);
              break;
           }
           du4.append (ret[j]);
         }
         clusterIndex = j;
         if (i>=n) du4.append (ucs4[i++]); /* fallback - error */
         while (i<n)
         {
          du4.append (ucs4[i++]);
         }
         ret.clear ();
      }
      else if (composer.isOK ())
      {
        ret.clear();
        n = composer.lift (ucs4, i, false, &ret);
        if (ret.size()==1) /* it was  a composition */
        {
          composition = ret[0];
          if (i>=n) du4.append (ucs4[i++]); /* fallback - error */
          while (i<n)
          {
            du4.append (ucs4[i++]);
          }
          ret.clear();
        }
        else /* it was not a composition */
        {
          ret.clear();

          /* check if it can be broken down into a composition */
          unsigned int n = composer.lift (ucs4, i, true, &ret);

          if (ucs4[i] != 0 && n == i+1 
              && ret.size() > 0 && ret.size() <= SD_MAX_COMPOSE)
          {
            usePrecomposed = true;
            n = i+1;
            composition = ucs4[i++];
            du4.append (ret);
          }
          else
          {
            /* 
             * No composition, there is only a base. 
             * Extra compositions will be added later.
             */
            du4.append (ucs4[i++]);
          }
        }
      }
      else
      {
        du4.append (ucs4[i++]);
      }
      break;
    } /* End Switch */

    if (embed) /* PDF */
    {
      SS_UCS4 em = ucs4[i]; 
      i++;
      if (em == SD_CD_PDF)
      {
        if (stack.size() > 0) stack.truncate(stack.size()-1);
      }
      else
      {
        stack.append (em);
      }
      continue;
    }
    
    if (!isLineEnd && i<ucs4.size())
    {
      SS_UCS2 u2=getCharClass(ucs4[i]);
      if (u2 == SD_CC_Mn || u2 == SD_CC_Me)
      {
        /*
         * There are two major differences between a shaped glyph
         * and an unshaped glyph. 
         * 1. The shaped glyph does not contain the 'composition'
         *    extra characters at the end of du4 when usePrecomposed
         *    is set and there are no decompositions. 
         *    The unshaped glyphs always contains it.
         * 2. We can not merge <base><comp1> for shapes because base 
         *    is used for shaping. We must set compIndex after base
         *    for shaped, before base for unshaped glyphs. This
         *    is just a convention here. For unshaped we don't really
         *    care: XXXX YYYY ZZZZ or XXXX ; YYYY ZZZZ. So we use former.
         */
        if (isShaped) 
        {
          /* 1 */
          if (du4.size()==4) du4.append (composition);
          /* 2 */
          compIndex = du4.size();
        }
        else
        {
          if (du4.size()==0) /* just in case */
          {
            du4.append (composition);
            composition = 0;
            usePrecomposed = false;
            compIndex =0; /* also works with 1 - #2 is very weak */
          }
          else /* composing extras */
          {
            compIndex = du4.size();
          }
        }
        while (u2 == SD_CC_Mn || u2 == SD_CC_Me)
        {
          du4.append (ucs4[i]);
          i++; 
          if (i >= ucs4.size()) break;
          u2=getCharClass (ucs4[i]);
        }
      }
    }

    bool realUsePrecomposed = usePrecomposed; 
    if (du4.size()==1) {
        if (composition == 0) {
            composition = du4[0];
        }
        if (composition == du4[0])
        {
            du4.clear();
            compIndex = 0;
            realUsePrecomposed = true;
        }
    } 
    if (composition == 0 && compIndex == 1) {
        compIndex = 0;
        realUsePrecomposed = false;
    } 
    SGlyph g (du4, composition, isShaped, clusterIndex, compIndex, realUsePrecomposed);
    if (!g.isEOL() && !g.isEOP())
    {
      g.setEmbeddingMarks (stack);
    }
    gl->append (g);
    if (g.isEOP()) {
        break;
    }
  }
  return i;
}

const SV_UINT&
SParagraph::getLogicalMap() const
{
  return logical;
}
