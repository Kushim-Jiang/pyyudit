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

#ifndef SLocation_h
#define SLocation_h

#include <stoolkit/SObject.h>
#include <stoolkit/SVector.h>
#include <stoolkit/SDimension.h>

class SLocation : public SObject
{
public:
  SLocation (void);
  SLocation (int x, int y);

  SLocation (const SLocation& d);
  SLocation (const SDimension& d);

  SLocation operator = (const SLocation& d);
  SLocation operator - (const SLocation& d) const;
  SLocation operator + (const SLocation& d) const;
  SLocation operator + (const SDimension& d) const;
  SLocation operator * (const SDimension& d) const;
  SLocation operator / (const SDimension& d) const;

  bool operator == (const SLocation& d) const;
  bool operator != (const SLocation& d) const;
  bool operator > (const SLocation& d) const;
  bool operator < (const SLocation& d) const;
  bool operator >= (const SLocation& d) const;
  bool operator <= (const SLocation& d) const;

  SLocation minimize (const SLocation & d) const;
  SLocation maximize (const SLocation & d) const;
  SLocation minimize (const SDimension & d) const;
  SLocation maximize (const SDimension & d) const;

  SLocation minmerge (const SLocation & l) const;
  SLocation maxmerge (const SLocation & l) const;

  int angle32 () const;
  unsigned int distance () const;
  unsigned long distance2 () const;
  int scalarProduct (const SLocation &l) const;
  int vectorProduct (const SLocation &l) const;

  virtual ~SLocation ();
  virtual SObject*  clone() const;
  int x;
  int y;
};


#endif /* SLocation_h */
