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

#ifndef SLineCurve_h
#define SLineCurve_h

#include <stoolkit/SObject.h>
#include <stoolkit/SVector.h>
#include <stoolkit/SLocation.h>

class SLineCurve : public SObject
{
public:
  /* Obligatory stuff */
  SLineCurve (void);
  SLineCurve (const SLineCurve& d);
  SLineCurve operator = (const SLineCurve& d);
  virtual ~SLineCurve ();
  virtual SObject*  clone() const;


  unsigned int length () const;
  unsigned int length (unsigned int segment) const;
  unsigned int length (unsigned int from, unsigned int to) const;

  unsigned int distance () const;
  unsigned int distance (unsigned int p0, unsigned int p1) const;

  const SLocation& operator[] (unsigned int index) const;

  SLocation getVector (unsigned int p0, unsigned int p1) const;
  unsigned int size() const;
  void clear();
  void append (const SLocation& l);
protected:
  
  SVector<SLocation>         vectors;
  SBinVector<unsigned int>   lengths;
};


typedef SVector<SLineCurve>  SLineCurves;


#endif /* SLineCurve_h */
