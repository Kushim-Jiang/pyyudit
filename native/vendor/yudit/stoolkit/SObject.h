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
 
#ifndef SObject_h
#define SObject_h

/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 */

/*
 * This is the basic class for SBinVector
 * ONLY SOBJECTS CAN BE AUTODELETED
 */
class SObject
{
public:
  virtual SObject*  clone() const =0;
  //virtual int compare (const SObject& o) const = 0;
#if SS_DEBUG_MEMORY_LEAK
  inline SObject(void);
  virtual ~SObject();
  static int debug (int level);
private:
  static int created;
  static int debugLevel;
#else
  // PUBLIC
  inline virtual ~SObject();
#endif
};

#if SS_DEBUG_MEMORY_LEAK
//PUBLIC/INLINE
SObject::SObject(void)
{
  created++;
}
#else
SObject::~SObject()
{
}
#endif
#endif /* SObject_h */
