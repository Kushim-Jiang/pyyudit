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


/**
 * I collected most of the Unicode Implicit bidi 
 * algorithm in this class, so it will be easier
 * to throw it away.
 */

#include "stoolkit/SBiDi.h"

/**
 * This is the segment, that can independently resolve types.
 */
SBiDiSegment::SBiDiSegment (SD_BiDiClass* _classes)
{
  classes = _classes;
  from = 0;
  till = 0;
  sor = true;
  eor = true;
}
SBiDiSegment::~SBiDiSegment()
{
}

static bool debugFlag = false;

/**
 * This will resolve the weak and the neutral types.
 * FIXME: This is the only thing that is not yet finished.
 */
void
SBiDiSegment::resolveWeakNeutral()
{
  unsigned int i;
  unsigned int j;

  SD_BiDiClass first = sor ? SD_BC_L : SD_BC_R;
  SD_BiDiClass last = eor ? SD_BC_L : SD_BC_R;

  if (debugFlag)
  {
    fprintf (stderr, "RES from=%u till=%u sor=%d eor=%d level=%u\n",
       from, till, (int)sor, (int)eor, level); 
  }

  /* W1 */
  SD_BiDiClass curr = first;
  for (i=from; i<till; i++)
  {
    if (classes[i] == SD_BC_NSM)
    {
      classes[i] = curr;
    }
    else
    {
      curr = classes[i];
    }
  }
  /* W2 */
  /* Search backwards from each instance of a European number until the
   * first strong type (R,L AL or sor is found. If an AL is found, change
   * the type of the European number to Arabic number.
   * I think this is strange: in Arabic context, at the beginng of the line:
   * TSET CIBARA -10% 
   * TSET %10- CIBARA 
   * is the output. Isn't that strange? sor will never be AL....
   */
  for (i=from; i<till; i++)
  {
    if (classes[i] == SD_BC_EN)
    {
      for (j=i; j>0; j--)
      {
        SD_BiDiClass c = classes[j-1];
        if (c==SD_BC_L || c==SD_BC_R || c==SD_BC_AL)
        {
          if (c==SD_BC_AL) classes[i] = SD_BC_AN;
          break;
        }
      }
      // first is never AL, it is either L or R
      //if (j==0) classes[i] = first;
    }
  }
  /* W3 */
  for (i=from; i<till; i++)
  {
    if (classes[i] == SD_BC_AL) classes[i] = SD_BC_R;
  }
  /* W4 */
  for (i=from+1; i+1<till; i++)
  {
    if (classes[i] == SD_BC_ES)
    {
      if (classes[i-1] == SD_BC_EN && classes[i+1] == SD_BC_EN)
      {
        classes[i] = SD_BC_EN;
      }
      i++;
    }
    if (classes[i] == SD_BC_CS && i+1<till)
    {
      if (classes[i-1] == SD_BC_EN && classes[i+1] == SD_BC_EN)
      {
        classes[i] = SD_BC_EN;
      }
      if (classes[i-1] == SD_BC_AN && classes[i+1] == SD_BC_AN)
      {
        classes[i] = SD_BC_AN;
      }
      i++;
    }
  }
  /* W5 */
  for (i=from; i<till; i++)
  {
    if (classes[i] == SD_BC_ET)
    {
      bool ok = false;
      /* find an EN. only ET is allowed. */
      for (j=i+1; j<till; j++)
      {
        if (classes[j] == SD_BC_EN)
        {
          ok = true;
          break;
        }
        if (classes[j] != SD_BC_ET) break;
      }
      if (ok)
      {
        while (i<j) classes[i++] = SD_BC_EN;
      }
      /* the routine below will continue if ok */
    }
    if (classes[i] == SD_BC_EN)
    {
      i++;
      while (i<till && classes[i] == SD_BC_ET)
      {
        classes[i] = SD_BC_EN;
      }
      /* increment in upper cycle */
      i--;
    }
  }

  /* W6 */
  for (i=from; i<till; i++)
  {
    if (classes[i] == SD_BC_ET) classes[i] = SD_BC_ON;
    if (classes[i] == SD_BC_ES) classes[i] = SD_BC_ON;
    if (classes[i] == SD_BC_CS) classes[i] = SD_BC_ON;
  }
  /* W7 */
  for (i=from; i<till; i++)
  {
    if (classes[i] == SD_BC_EN)
    {
      for (j=i; j>0; j--)
      {
        SD_BiDiClass c = classes[j-1];
        if (c==SD_BC_L || c==SD_BC_R)
        {
          if (c==SD_BC_L) classes[i] = SD_BC_L;
          break;
        }
      }
      if (j==0 && first==SD_BC_L) classes[i] = first;
    }
  }
  /* Resolving Neutral Types */

  /* Neutral is non: R L AN EN */
  /* N1 */
  curr = first;
  for (i=from; i<till; i++)
  {
    SD_BiDiClass c = classes[i];
    if (c == SD_BC_EN || c==SD_BC_AN)
    {
      c = SD_BC_R;
    } 
    if (c==SD_BC_R ||  c==SD_BC_L)
    {
      curr = c; 
      continue;
    }
    /* This is the beginning of neutral sequence */
    SD_BiDiClass found = last; 
    for (j=i+1; j<till; j++)
    {
      SD_BiDiClass c0 = classes[j];
      if (c0 == SD_BC_EN || c0==SD_BC_AN)
      {
        c0 = SD_BC_R;
      }
      if (c0==SD_BC_R || c0==SD_BC_L)
      {
        found = c0; 
        break;
      }
    }
    if (found == curr)
    {
      while (i<j) classes[i++] = found;
      curr = found; 
    }
  }
  /* N2 */
  for (i=from; i<till; i++)
  {
    SD_BiDiClass c = classes[i];
    if (c == SD_BC_EN || c==SD_BC_AN) c = SD_BC_R;
    if (c!=SD_BC_R && c!=SD_BC_L)
    {
      classes[i] = ((level%2)==0) ? SD_BC_L : SD_BC_R;
    }
  }
}


/**
 * Create a new holder for the bidi algorithm.
 */
SBiDi::SBiDi (unsigned int _initLevel, unsigned int size)
{
  initLevel = _initLevel;
  lastLevel = _initLevel;

  /* To make things a bit faster we use arrays */
  classes = new SD_BiDiClass[size];
  CHECK_NEW (classes);

  lastSegment = new SBiDiSegment(classes);
  CHECK_NEW (lastSegment);
  lastSegment->sor = (initLevel%2) == 0;
  lastSegment->level = lastLevel;
  lastSize = 0;
}

SBiDi::~SBiDi ()
{
  delete classes;
  for (unsigned int i=0; i<segments.size(); i++)
  {
    delete segments[i];
  }
  if (lastSegment) delete lastSegment;
}

void
SBiDi::append (unsigned int embedLevel, SD_BiDiClass bidiType)
{
  classes[lastSize] = bidiType;
  lastSize++;

  if (embedLevel == lastLevel)
  {
    lastSegment->till = lastSize;
    lastLevel = embedLevel;
    return;
  }
  /* change of level at the beginning */
  if (lastSegment->till==0)
  {
    lastSegment->sor = (lastLevel>embedLevel) 
       ? ((lastLevel %2)==0) : ((embedLevel%2)==0);
    lastSegment->till = lastSize;
    lastSegment->level = lastLevel;

    lastLevel = embedLevel;
    return;
  }
  /* finish old segment */
  lastSegment->eor = (lastLevel>embedLevel) 
     ? ((lastLevel %2)==0) : ((embedLevel%2)==0);
  lastSegment->till = lastSize-1; /* sorry last one does not belong to us */
  segments.append (lastSegment);


  /* new segment needed */
  bool sor = (lastLevel>embedLevel)
       ? ((lastLevel %2)==0) : ((embedLevel%2)==0);
  lastLevel = embedLevel;

  lastSegment = new SBiDiSegment(classes);
  CHECK_NEW (lastSegment);

  lastSegment->sor = sor;
  /* we incremented lastSize at the beginning */
  lastSegment->from = lastSize-1;
  lastSegment->till = lastSize;
  lastSegment->level = embedLevel;
}

void
SBiDi::resolveWeakNeutral()
{
  if (lastSize == 0) return;
  if (lastSegment)
  {
    lastSegment->eor = (lastLevel>initLevel) 
       ? ((lastLevel %2)==0) : ((initLevel%2)==0);
    segments.append (lastSegment);
    /* This is how I debug */
    if (segments.size()>1)
    {
      // debugFlag = true;
    }
    lastSegment = 0;
  }
  for (unsigned int i=0; i<segments.size(); i++)
  {
    segments[i]->resolveWeakNeutral();
  }
  if (segments.size()>1) debugFlag = false;
}

void
SBiDi::insertBN (unsigned int at)
{
  /* move up */
  for (unsigned int i=lastSize; i>at; i--)
  {
    classes[i] = classes[i-1];
  }
  lastSize++;
  //classes[at] = SD_BC_BN;

  /* make this stupid BN the type of the previous char */
  /* The standard does not require any particular placement */
  if (at == 0)
  {
    classes[at] = ((initLevel%2) == 0) ? SD_BC_L : SD_BC_R;
    return;
  }
  /* be the previous. */
  classes[at] = classes[at-1];
}
