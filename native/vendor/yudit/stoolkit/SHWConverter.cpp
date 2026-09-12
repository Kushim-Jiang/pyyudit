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

#include "SHWConverter.h"

#include "SExcept.h"
#include "SBinHashtable.h"
#include "SUniMap.h"
#include "SIO.h"
#include "SUtil.h"
#include "SAutogen.h"

static SLineCurve lineCurve32;
static SLineCurve lineCurve12;

/* This allows 20% fuzziness */
#define SD_CADIDATE_PERCENT 20
#define SD_CADIDATE_QUANTITY 5

#include <stdio.h>
#include <string.h>


/*
 * Handwriting conversion module for Yudit.
 * Gaspar Sinai <gaspar@yudit.org>
 *
 * A total replacement of 
 * gtkkanjipad-0.07 Sven Verdoolaege <skimo@kotnet.org>
 * http://www.kotnet.org/~skimo/kanji/
 * that was mostly copied from Owen Taylor's KanjiPad
 * http://www.gtk.org/~otaylor/kanjipad/
 * The handwriting recognition algorithms were invented by Todd David
 * Rudick (TDR) for his program JavaDict:
 *   http://www.cs.arizona.edu/japan/JavaDict/
 * and translated into C by Robert Wells for his program JStroke:
 *  http://www.wellscs.com/pilot/
 *  JStroke runs on Pilot handheld computers. 
 * 
 * Changes made by Gaspar Sinai <gaspar@yudit.org>
 * o totally documented in the perl output file the 
 *   magic letters and numbers.
 * o a total rewrite/reinvention  of the scoring system 
 *   - renamed ranking system.
 *   - correctly calculates probabilities.
 * o a total rewrite of all the routines found in original programs,
 *   for clearity. 
 * A did not make any change to the original stroke format.
 * Reason: I want to be able to add stuff that is used for kanjipad.
 */
class SHWData
{
public:
  SHWData(const SString& data);
  ~SHWData();

  SStringVector convert(const SLineCurves& str, bool derected);
  unsigned int getIndex(unsigned int stroke);
  void       reindex();
  int        count;
  int        version;
  SV_UINT    index;
private:
 int scoreAll (const SLineCurves& strokes, 
   const char* buffer, unsigned int from, unsigned int to, 
   bool directed);

 bool rewriteGuideElementV1 (unsigned int element,  SV_UINT *guide);
 unsigned int rewriteGuideElementsV2 (const char* buffer, 
     unsigned int size,  SV_UINT *guide);

 int fitOneStroke (const SLineCurve& stroke,  
  unsigned int sfrom, unsigned int sto,
  const SV_UINT& guide, unsigned int pfrom, 
  unsigned int pto, int reversed);

 int getFilterValue (const SLineCurves& strokes, const char* filter, 
  unsigned int filterSize);

 int applyFilter (const SLineCurves& strokes, const char* operand, 
  const unsigned int * strokeIndex, bool must);

 int calculateFilter (const SLineCurve& stroke, char operand);
  SFileImage image;
  SString    name;
};

/* todo - on exist free this cache */
typedef SBinHashtable<SHWData*> SWTable;
static SWTable* hwcache=0;

/**
 * Create a new SHWConverter using name.
 * This is an empty converter.
 */
SHWConverter::SHWConverter(void)
{
  shared = 0;
}

static int debugLevel = 0;
void
SHWConverter::setDebugLevel (int level)
{
  debugLevel = level;
}

/**
 * Create a new SHWConverter using name.
 * @param name is the name of the conversion.
 * SUniMap::getPath() will be searched for name.hwd file.
 */
SHWConverter::SHWConverter(const SString& _name)
{
  shared = 0;
  /* is it cached ? */
  SHWData* data;
  name = _name;
  if (hwcache == 0) hwcache = new SWTable();
  if ((data = hwcache->get (name)) == 0)
  {
    data = new SHWData(name);
    CHECK_NEW (data);
    if (data->count==0)
    {
      delete data;
      data = 0;
    }
    else
    {
       hwcache->put (name, data);
    }
  }
  else
  {
    data->count++;
  }
  shared = data;
}

const SString& 
SHWConverter::getName() const
{
  return name;
}

/**
 * Copy constructor.
 * Just shared the existing buffer.
 */
SHWConverter::SHWConverter(const SHWConverter& c)
{
  if (&c != this)
  {
     SHWData* data = (SHWData*) c.shared;
     if (data) data->count++;
     this->shared = data;
     this->name = c.name;
  }
}

SHWConverter::~SHWConverter()
{
  SHWData* data = (SHWData*) shared;
  if (data) data->count--;
}

SHWConverter
SHWConverter::operator= (const SHWConverter& c)
{
  if (&c != this)
  {
     SHWData* data = (SHWData*) shared;
     if (data) data->count--;
     data = (SHWData*) c.shared;
     if (data) data->count++;
     this->shared = data;
     this->name = c.name;
  }
  return *this;
}

/**
 * Check if this converter is ok
 * @return false if this converter can not be used.
 */
bool
SHWConverter::isOK () const
{
  SHWData* data = (SHWData*) shared;
  if (data !=0) data->reindex();
  return (data != 0 && data->index.size() > 0);
}

/**
 * Convert hand-written data into a list of candidates.
 * @str is the strokelist
 * @param directed is true if direction is taken into account.
 * @return the candidate list in utf-8
 */
SStringVector
SHWConverter::convert(const SLineCurves& str, bool directed) const
{
  SHWData* data = (SHWData*) shared;
  if (data == 0) return SStringVector();
  return data->convert (str, directed);
}

/**
 *  Create a new SHWData and set index to 0 if failed, set index to
 *  1 if it was successful.
 */
SHWData::SHWData(const SString& data)
{
  count = 0;
  version = 0;
  name = data;
  SString n (data);
  n.append (".hwd");
  SStringVector p = SUniMap::getPath();
  SFile f (n, p);
  if (f.size() >0)
  {
    name = f.getName();
    image = f.getFileImage();
    if (image.size() > 0) count++;
  }
}

SHWData::~SHWData()
{
}

/**
 * Recalculate indexes so that the index will hash to
 * the appropriate stroke number.
 * If it can not be, reset the image.
 */
void
SHWData::reindex()
{
  if (image.size()<=0 || index.size() > 0) return;
  char* buffer = image.array();
  unsigned int length = (unsigned int) image.size();
  /* filetype check */
  if (length<6 || buffer[0] != '#' || buffer[1] != 'H'
         || buffer[2] != 'W' || buffer[3] != 'D'
         || buffer[4] != ' ')
  {
    /* empty fileimage */
    image = SFileImage();
    return;
  }
  /* Version 1 uses mystic letters for directions */
  if (buffer[5] == '1')
  {
    version = 1; 
  }
  /* Version 1 uses numbers for directions on a clock (0..11)*/
  else if (buffer[5] == '2') 
  {
    version = 2; 
  }
  else 
  {
    /* empty fileimage */
    image = SFileImage();
    return;
  }
  unsigned int i;
  unsigned int j;
  bool nl = true;
  unsigned int linecount=1;
  for (i=0; i<length; i++)
  {
    /* comments */
    if (nl && buffer[i] == '\n')
    {
      linecount++;
      continue;
    }
    if (nl && buffer[i] == '#')
    {
      nl = false;
      continue;
    }
    /* skip bad lines */
    if (nl && length>i + 2 + 1 + 1 + 1)
    {
      if (buffer[i]<'0' || buffer[i]>'9' 
          || buffer[i+1]<'0' || buffer[i+1]>'9'
          || buffer[i+2] != ' ')
      {
        if (debugLevel > 0)
        {
          fprintf (stderr, "FileFormat error in SHWConverter: %*.*s line: %u\n",
           SSARGS (name), linecount);
        }
        image = SFileImage();
        index.clear();
        return;
      }
      nl = false;
      unsigned int strokeCnt = 10 * (unsigned int)(buffer[i]-'0')
           + (unsigned int)(buffer[i+1]-'0');
      if (index.size() > strokeCnt+1)
      {
        if (debugLevel > 0)
        {
          fprintf (stderr, "SHWConverter: %*.*s line: %u unsorted!\n",
             SSARGS (name), linecount);
        }
        image = SFileImage();
        index.clear();
        return;
      }
      /* fill index with zeros */
      if (index.size() < strokeCnt+1)
      {
        j = index.size();
        while (j++<strokeCnt) {
          index.append (i); // YES, i
        }
        index.append (i);
      }
    }
    if (buffer[i] == '\n')
    {
      linecount++;
      nl = true;
      continue;
    }
  } 
}

/**
 * Get index for stroke. Return size if index is too high.
 */
unsigned int
SHWData::getIndex (unsigned int i)
{
  if (i>=index.size()) return (unsigned int) image.size();
  return index[i];
}

/**
 * Convert hand-written data into a list of candidates.
 * @str is the strokelist
 * @param directed is true if direction is taken into account.
 * @return the candidate list in utf-8
 */
SStringVector
SHWData::convert(const SLineCurves& strokes, bool directed)
{
  /* version 1.0 */
  if (lineCurve32.size() ==0)
  {
    SLineCurve clock32 = getCurveCircle32();
    /* scale it down, to fit on 1 byte */
    for (unsigned i=0; i<clock32.size(); i++)
    {
      const SLocation &l = clock32[i];
      lineCurve32.append (SLocation(l.x/512, l.y/512));
    }
  }
  /* version 2.0 */
  if (lineCurve12.size() ==0)
  {
    SLineCurve clock24 = getCurveCircle24();
    /* scale it down, to fit on 1 byte */
    for (unsigned i=0; i<clock24.size(); i++)
    {
      /* make it a twelve hour clock */
      if (i+1<clock24.size() && (i&1)) continue;

      const SLocation &l = clock24[i];
      lineCurve12.append (SLocation(l.x/512, l.y/512));
    }
  }

  if (image.size() <= 0 || strokes.size()==0) return SStringVector();
  if (index.size() == 0) reindex();
  /* FIXME */
  unsigned int i = getIndex (strokes.size());
  unsigned int j = getIndex (strokes.size()+1);
  char* buffer = image.array();

  SStringVector candidates;
  SV_INT        scores;

  bool nl = true;
  for (; i < j; i++) {
    if (nl && buffer[i] == '\n') continue;
    /* comments */
    if (nl && buffer[i] == '#') { nl = false; continue; }
    /* first, rest */
    /* space */
    if (nl)
    {
      unsigned int k=i+3;
      while (k<j && buffer[k] != '\n' && buffer[k] != ' ') k++;
      /* we are a bit corrupt */
      if (k>=j || buffer[k] != ' ')
      {
        break;
      }
      SString str (&buffer[i+3], k-i-3);
      candidates.append (str);

      i=k+1; while (k<j && buffer[k] != '\n') k++;
      /* add score */
      scores.append (scoreAll (strokes, buffer, i, k, directed));
      i=k;
    }
    if (buffer[i] == '\n') { nl = true; continue; }
  }
  SV_INT sorted;
  if (scores.size()==0) return SStringVector();

  /* sort stuff */
  SStringVector bestOnes;
  unsigned int vsize = scores.size();
  for (i=0; i<vsize; i++)
  {
     /* the bigger the score the better it is */
     unsigned int order = sorted.appendSorted(-scores[i]);
     bestOnes.insert (order, candidates[i]);
  }
  /* print out the length of full stuff and this length. */
  unsigned int fullsize = 0;
  for (i=0; i<strokes.size(); i++)
  {
    fullsize += strokes[i].length();
  }
  if (debugLevel > 2)
  {
    fprintf (stderr, "Stokes: count=%u sorted=%u, length=%u bestlength=%d\n",
     strokes.size(), sorted.size(), fullsize, -sorted[0]/lineCurve32[16].y);
  }

  /* the most negatives come first. */
  int delta = sorted[0] * SD_CADIDATE_PERCENT / 100; /* 20 percent, negative */
  if (delta<0) delta = -delta; // always
  for (i=0; i<sorted.size(); i++)
  {
    if (sorted[i] > sorted[0]+delta || i>SD_CADIDATE_QUANTITY)
    {
      sorted.truncate (i);
      bestOnes.truncate (i);
      break;
    }
  }
  return (SStringVector(bestOnes));
}

/*
 * Simply put - it calculates the distance of the projection
 * of the curve on the guideline. The best it fist the closest
 * it gets to the real line-length of strokes.
 * @param strokes contains all the strokes in the drawing.
 * @param buffer  contains the score description line (character guide)
 * @param from is the starting point in buffer
 * @param to is the ending point in buffer
 * @param directed  is true if writing direction matters.
 * @return a score number. The bigger the number the better it is.
 * at the best fit this should be the length of the whole strokes.
 */
int
SHWData::scoreAll (const SLineCurves& strokes, const char* buffer,
   unsigned int from, unsigned int to, bool directed)
{
  unsigned int cindex = from;
  int currentLength = 0;
  for (unsigned int i=0; i<strokes.size(); i++)
  {
    SV_UINT guide; /* start with a clear one */
    if (version == 2)
    {
      /* "1-11-12, 2-3-4" */
      cindex += rewriteGuideElementsV2 (&buffer[cindex], to-cindex, &guide);
      /* no more guide */
      if (cindex>to)
      {
        if (debugLevel > 0)
        {
          fprintf (stderr, "Not enough strokes in '%*.*s' expected=%u got=%u\n",
            (int)(to-from), (int)(to-from), &buffer[from], strokes.size(), i);
        }
        return -1;
      }
    }
    else
    {
      if (!rewriteGuideElementV1 ((unsigned int) buffer[cindex], &guide))
      {
        if (debugLevel > 0)
        {
          fprintf (stderr, "Not enough strokes in '%*.*s' expected=%u got=%u\n",
            (int)(to-from), (int)(to-from), &buffer[from], strokes.size(), i);
        }
        return -1;
      }
      cindex++;
      /* get the rest - all small letters - this is in version 1 only */
      while (cindex < to  && buffer[cindex] != '|'
          && (buffer[cindex] == ' ' || buffer[cindex] >= 'a'))
      {
        if (!rewriteGuideElementV1 ((unsigned int) buffer[cindex], &guide))
        {
          if (debugLevel > 0)
          {
            fprintf (stderr, "Bad syntax in '%*.*s' strokes=%u\n",
              (int)(to-from), (int)(to-from), &buffer[from], strokes.size());
          }
          return -1;
        }
        cindex++;
      }
    }
    /* this stroke is done */
    const SLineCurve& stroke = strokes[i];
    /**
     * Try positive
     */
    int projectedLength = fitOneStroke (stroke, 0, stroke.size(), 
      guide, 0, guide.size(),  1);
    /**
     * Try negative - reverse order
     */
    if (!directed && projectedLength < 0)
    {
      projectedLength = fitOneStroke (stroke, 0, stroke.size(),
        guide, 0, guide.size(), -1);
    }
    
    /*
     * Don't add ranks up. Probabilities are multiplied.
     * gaspar@yudit.org 2001-11-04
     */
    currentLength += projectedLength;
  }
  /* apply additional filtering - this also return a length type object */
  if (cindex+1 < to && buffer[cindex] == '|')
  {
    currentLength += getFilterValue (strokes, &buffer[cindex+1], to-cindex-1);
  }
  return currentLength;
}

/**
 * Try to fit the Stroke to the guide. This is 
 * Gaspar Sinai original <gaspar@yudit.org>
 * 2001-11-06, Tokyo
 * @param stroke is the stroke to rank.
 * @param strokefrom is the lower limit in stroke
 * @param strokeend is the upper, not including limit in stroke
 * @param guide is the angle path 0..31
 * @param guidestart is the lower limit in guide
 * @param guideend is the upper, not including limit in ppath
 * @param directed is
 *  <ul>
 *   <li> 1 if directed in positive direction </li> 
 *   <li> -1 if directed in negative direction </li> 
 *  </ul>
 * @return the length of the fit, that is projected with a 
 * unit vector to the angle. it can be negative if opposite 
 * fit fits.
 */
int 
SHWData::fitOneStroke (const SLineCurve& stroke,  unsigned int strokefrom, 
  unsigned int strokeend, const SV_UINT& guide, unsigned int guidestart, 
  unsigned int guideend, int directed)
{
  /* Should not happen unless we have a corrupt guide */
  if (guideend-guidestart < 1 || stroke.length()==0) return 0; 

  /* End of recursize loop - not dividable any more */
  if (strokeend-strokefrom < 2 || guideend-guidestart==1)
  {
    if (strokeend-strokefrom < 1) return 0;
    SLocation v = stroke.getVector (strokefrom, strokeend-1);

    const SLocation& l = (version==1)
          ? lineCurve32[guide[guidestart]] // 36 JavaDict, JStroke, Kanjipad
          : lineCurve12[guide[guidestart]]; // 12 Yudit version

    int ret = v.scalarProduct(l) * directed;
    if (debugLevel > 10)
    {
     fprintf (stderr,"fitOneStroke[%u..%u] guide [%u..%u] directed=%d ret=%d\n",
        strokefrom, strokeend, guidestart, guideend, directed, ret);
     fprintf (stderr, "unit=%d,%d stroke=%d,%d\n", l.x, l.y, v.x, v.y);
    }
    return ret;
  }
  /* 
   * cut the guide into two equal halves and find 
   * a partition that has longest projected length  
   * Gaspar Sinai <gaspar@yudit.org>
   */
  int maxlength = -1;
  unsigned int pmid = (guideend-guidestart) / 2 + guidestart;
  unsigned int s0 = guidestart;
  unsigned int s1 = pmid;
  unsigned int e0 = pmid;
  unsigned int e1 = guideend;

  /* reverse direction */
  if (directed<0)
  {
    s0=pmid;
    s1=guideend;
    e0=guidestart;
    e1=pmid;
  }
/*
  No need 
  unsigned int sstep = (strokeend - strokefrom) / 2;
  if (sstep <1) sstep = 1;
*/
  /* slowly move it */ 
  unsigned int sstep = 1;
  int fulllength = (int)stroke.length();
  for (unsigned int i=strokefrom; i+sstep<strokeend; i+=sstep)
  {
    /*
     * continue if not evenly distributed 
     */
    int l0 = (int)stroke.length (strokefrom, i+sstep);
    int l1 = (int)stroke.length (i+sstep, strokeend-1);
    /* l0/full is like s1-s0/guideend - guidestart*/
    /* best if: fulllength * (s1-s0) == l0 * (guideend - guidestart); */

    /* best if half factor 2 - but let's not be so strict */
#define SD_DIVIDE_FACT 4
    if ((fulllength * (s1-s0)) > SD_DIVIDE_FACT * l0 * (guideend - guidestart)
        || SD_DIVIDE_FACT * (fulllength * (s1-s0)) < l0 * (guideend - guidestart))
    {
      continue;
    }

    if ((fulllength * (e1-e0)) > SD_DIVIDE_FACT * l1 * (guideend - guidestart)
        || SD_DIVIDE_FACT * (fulllength * (e1-e0)) < l1 * (guideend - guidestart))
    {
      continue;
    }
    int length0 = fitOneStroke (stroke, strokefrom, i+sstep+1, 
        guide, s0, s1, directed);

    int length1 = fitOneStroke (stroke, i+sstep, strokeend,
        guide, e0, e1, directed);

    int thislength = length0 + length1;
    if (thislength > maxlength) maxlength = thislength; 
  }
  return maxlength;
}

/**
 * Filter is a very strong ranker.
 * @return a direction-like object.
 */
int
SHWData::getFilterValue (const SLineCurves& strokes, 
  const char* filter, unsigned int filterSize)
{
  char oparand[2];
  unsigned int  strokeIndex[2];
  
  /* init */
  oparand[0] = oparand[1] = 0;
  strokeIndex[0] = strokeIndex[1] = 0;
  bool must = false;

  unsigned idx=0; /* 0 or 1 */
  int currentLength = 0;
  int minLength = 0;
  int maxLength = 0;
  int length = 0;
  for (unsigned int i=0; i<filterSize; i++) {

    switch (filter[i])
    {
    case 'x':
    case 'y':
    case 'i':
    case 'j':
    case 'a':
    case 'b':
    case 'l':
      oparand [idx] = filter[i];
      break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': 
      strokeIndex[idx] = strokeIndex[idx] * 10 + (filter[i] - '0');
      break;
    case '-': /* next operand */
      idx = 1;
      break;
    case '!': /* filter must be applied */
      must = true;
      break;
    case ' ':
      if (idx==0) break;
    default:
      if (idx != 1 || strokeIndex[0] > strokes.size() || strokeIndex[0] == 0
         || strokeIndex[1] > strokes.size() || strokeIndex[1] == 0)
      {
        if (debugLevel > 0)
        {
          fprintf (stderr, "Filter syntax error in %*.*s at '%c'\n", 
               (int)filterSize, (int)filterSize, filter, filter[i]);
        }
        return 0;/* neutral */
      }
      /* apply filter */
  
      length = applyFilter(strokes, oparand, strokeIndex, must);
      currentLength += length;
      if (length < minLength) minLength = length;
      if (length > maxLength) maxLength = length;

      /* init */
      oparand[0] = oparand[1] = 0;
      strokeIndex[0] = strokeIndex[1] = 0;
      must = false;
      idx = 0;
    }
    /* end case */
  }
  /* last filter */
  if (idx == 1)
  {
      /* apply filter */
      length = applyFilter(strokes, oparand, strokeIndex, must);
      currentLength += length;
      if (length < minLength) minLength = length;
      if (length > maxLength) maxLength = length;
  }
  if (currentLength == 0) return 0;
  if (currentLength < 0) return minLength;
  return currentLength;
  
}

/**
 * @return 0..127 31 being neutral
 * @param strokes contains all the strokes.
 * @param operand contains the operands (2 of them)
 * @param strokeIndex contains the index in stoke array.
 *  1 being the first element.
 * @param must can enforce the filter.
 */
int
SHWData::applyFilter (const SLineCurves& strokes, const char* operand, 
   const unsigned int * strokeIndex, bool must)
{
  int val0 = calculateFilter (strokes[strokeIndex[0]-1], operand[0]);
  int val1 = calculateFilter (strokes[strokeIndex[1]-1], operand[1]);
  int diff = val0 - val1;
  /* plus is good, negative is a bad thing. */
  if (debugLevel > 10)
  {
    fprintf (stderr, "apply %c%d-%c%d%s = %d %d-%d\n", 
         operand[0], strokeIndex[0],
             operand[1], strokeIndex[1], (must?"!":""), diff, val0, val1);
  }
  if (must) return diff;
  return 2 * diff * SD_CADIDATE_PERCENT / 100;
  
  //return (must) ? diff : diff * SD_CADIDATE_PERCENT / 100 / 2;
}

/**
 * calculate one filter element.
 * @param stroke contains the stroke, only one.
 * @param operand is the operand 
 * @return length type argument. 
 */
int 
SHWData::calculateFilter(const SLineCurve& stroke, char operand)
{
  const SLocation& fl = stroke[0];
  const SLocation& ll = stroke[stroke.size()-1];
  switch (operand)
  {
  case 'x': return fl.x; /* first */
  case 'y': return fl.y; /* first */
  case 'i': return ll.x; /* second */
  case 'j': return ll.y; /* second */
  case 'a': return (ll.x+fl.x)/2; /* middle */
  case 'b': return (ll.y+fl.y)/2; /* middle */
  case 'l': return (ll-fl).distance(); /* length */
  }
  /* should not happen */
  return 0;    
}

/**
 * Try to rewrite version 2 syntax guide till size or 
 * '|'. '\n' and white space are ok. 
 * Otherwise update guide with values according to 12 hour clock
 * angles.
 * @return the next element after white space has been eaten up.
 * in case of error, return size+1.
 */
unsigned int
SHWData::rewriteGuideElementsV2 (const char* buffer, 
     unsigned int size,  SV_UINT *guide)
{
  bool param=false;
  bool done=false;
  bool separated=true;
  unsigned int currentAngle = 0;
  unsigned int i;
  
  for (i=0; i<size; i++)
  {
    switch (buffer[i])
    {
    case ' ':
    case '\t':
      if (param)
      {
        if (currentAngle > 12)
        {
          if (debugLevel > 0)
          {
            fprintf (stderr, "Guide: angle too large %u (max. is 12).\n", 
              currentAngle);
          }
          return size+1;
        }
        guide->append (currentAngle);
        param = false;
        currentAngle=0;
        separated = false;
      }
      break;
    case ',':
      done = true;
      i++;
      break;
    case '-':
      if (!param) return size+1;
      if (currentAngle > 12)
      {
        if (debugLevel > 0)
        {
          fprintf (stderr, "Guide: angle too large %u (max. is 12).\n", 
            currentAngle);
        }
        return size+1;
      }
      guide->append (currentAngle);
      param = false;
      currentAngle=0;
      separated=true;
      break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      if (!separated)
      {
        if (debugLevel > 0)
        {
          fprintf (stderr,"Guide: parameters should be separated with '-'.\n"); 
        }
        return size+1;
      }
      currentAngle=currentAngle*10 + (unsigned int) (buffer[i] - '0');
      param = true;
      break;
    case '|':
    case '\n':
      done = true;
      break;
    default:
      if (debugLevel > 0)
      {
        fprintf (stderr, "Guide: illegal character.\n"); 
      }
      return size+1;
    }
    if (done) break;
  }
  if (param)
  {
    if (currentAngle > 12)
    {
      if (debugLevel > 0)
      {
        fprintf (stderr, "Guide: angle-param too large %u (max. is 12).\n", 
          currentAngle);
      }
      return size+1;
    }
    guide->append (currentAngle);
  }
  if (guide->size()==0)
  {
    if (debugLevel > 0)
    {
      fprintf (stderr, "Guide: empty guide.\n"); 
    }
    return size+1;
  }
  while (i<size && (buffer[i] == ' ' || buffer[i] == '\t')) i++;
  return i;
}

/**
 * Rewrite the guide element to angle in a 0.31 scale.
 * 0 being 0 degrees 8 being 90
 * @param element is the character to rewrite
 * @param guide is the vector to put this.
 */
bool 
SHWData::rewriteGuideElementV1 (unsigned int element, SV_UINT *guide)
{
  switch ((char)element)
  {
  case ' ': 
    break;
  case 'A': 
  case 'a': 
    guide->append (20); break; /* TDR='1' CLK=07:30 DEG=225 */

  case 'B': 
  case 'b': 
    guide->append (16); break; /* TDR='2' CLK=06:00 DEG=180 */

  case 'C': 
  case 'c': 
    guide->append (12); break; /* TDR='3' CLK=04:30 DEG=135 */

  case 'D': 
  case 'd': 
    guide->append (24); break; /* TDR='4' CLK=09:00 DEG=270 */

  case 'F': 
  case 'f': 
    guide->append (8); break; /* TDR='6' CLK=03:00 DEG=090 */

  case 'G': 
  case 'g': 
    guide->append (28); break; /* TDR='7' CLK=10:30 DEG=315  */

  case 'H': 
  case 'h': 
    guide->append (0); break; /* TDR='8' CLK=12:00 DEG=360 */

  case 'I': 
  case 'i': 
    guide->append (4); break; /* TDR='9' CLK=01:30 DEG=045 */

  /* double ones */
  case 'J': 
  case 'j': 
    guide->append (16); guide->append (20);/* TDR='x' down  06:00 then 07:30 */
    break;

  case 'K': 
  case 'k': 
    guide->append (16); guide->append (12);/* TDR='y' down   06:00 then 04:30 */
    break;

  case 'L': 
  case 'l': 
    guide->append (16); guide->append (8);/* TDR='c' down   06:00 then 03:00 */
    break;

  case 'M': 
  case 'm': 
    guide->append (8); guide->append (16);/* TDR='b' across 03:00 then 06:00 */
    break;
  default:
    return false;
  }
  return true;
}

