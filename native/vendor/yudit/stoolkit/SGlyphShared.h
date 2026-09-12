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

#ifndef SGlyphShared_h
#define SGlyphShared_h

#include "stoolkit/STypes.h"

class SGlyphShared
{
public:
  char    cluster;
  char    composing;
  char    type;
  char    bidi;

  bool    lineend;
  bool    tab;
  bool    shaped;

  SS_UCS4 precomposed;
  SS_UCS4 mirror;
  /* 
   * This is ucs4v:
   *
   * For cluster == 0, precomposed is precomposed character if any.
   * shape[4],decomposition,extra-composed
   * ^                      ^
   * |                      +-- composing, or 0 
   * |                          
   * +------ if shaped it has 4 chars here. It can be private char.
   * 
   * For cluster == n, precomposed is cluster private charID if any
   * unicode-cluster,memory-cluster,extra-composed
   *                 ^              ^
   *                 |              +-------composing or 0
   *                 |
   *                 +---cluster-offset
   */
  SV_UCS4 ucs4v;
};


#endif /* SGlyphShared_h */
