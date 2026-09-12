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

#ifndef SBiDi_h
#define SBiDi_h

#include "stoolkit/STypes.h"
#include "stoolkit/SCharClass.h"

/**
 * I collected most of the Unicode Implicit bidi 
 * algorithm in this class, so it will be easier
 * to throw it away.
 */

class SBiDiSegment
{
public:
  SBiDiSegment(SD_BiDiClass* classes);
  ~SBiDiSegment ();
  void resolveWeakNeutral();

  SD_BiDiClass* classes; 
  unsigned int level;
  unsigned int from;
  unsigned int till;

  bool sor;
  bool eor;
};

class SBiDi
{
public:
  SBiDi (unsigned int initLevel, unsigned int size);
  ~SBiDi();

  void append (unsigned int embedLevel, SD_BiDiClass bidiType);
  void resolveWeakNeutral();
  /* this *must be called after* resolveWeakNeutral */
  void insertBN (unsigned int at);

  inline unsigned int segmentSize() const;
  inline const SBiDiSegment* getSegment (unsigned int at) const;
  inline SD_BiDiClass getBiDiType (unsigned int at) const;
private:
  unsigned int              initLevel;
  unsigned int              lastLevel;
  unsigned int              lastSize;
  SBiDiSegment*             lastSegment;
  SBinVector<SBiDiSegment*> segments;
  SD_BiDiClass*             classes;
};

const SBiDiSegment*
SBiDi::getSegment (unsigned int at) const
{
  return segments[at];
}

unsigned int 
SBiDi::segmentSize() const
{
  return segments.size();
}

SD_BiDiClass
SBiDi::getBiDiType (unsigned int at) const
{
  return classes[at];
}

#endif /* SBiDi_h */
