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

#ifndef SRendClass_h
#define SRendClass_h

#include <stoolkit/STypes.h>
/*!
 * \file SRendClass.h
 * \brief Character Rendering Properties
 * \author: Gaspar Sinai <gaspar@yudit.org>
 * \version: 2000-04-23
 */

class SRendClass 
{
public:
  /*!
   * \enum SType 
   * \brief A list of character properties that are useful
   *      when rendering text. 
   *  There are inherent properties, and derived properties. 
   *  Inherent properties can be determined from the unicode
   *  codepoint. Derived properties are derived, for instance
   *  the surrounding character. 
   *
   *  Joining class, for instance, is an inherent property
   *  and isolated form is a derived property. For simplicity
   *  we just mixed the whole set into one SType as there can
   *  be a lot of overlaps.
   */
  enum RType 
  {
     None=0,  //!< Placeholder
     //-----------------------------------------------------------------------
     //  Indic - Inherent 
     //-----------------------------------------------------------------------
     Cbase,
     Cbelow,
     Cpost,
     Cdead,  // This will be rewritten as Cbase or Cpre

     Mpre,
     Mabove,
     Mpost,
     Mbelow,
     Vsplit,

     VMbelow,
     VMabove,
     VMpost,

     LMpost,

     SMabove,
     SMbelow,

     VO,
     Nukta,
     Halant,

     ZWJ,
     ZWNJ,
     //-----------------------------------------------------------------------
     // Indic - Derived 
     //-----------------------------------------------------------------------
     Cpre=100,    
     Cra,
     Creph,
     CMra,
     CMreph,

     Chbase,
     Cfirst,    
     Clast,    
     JamoL,  
     JamoV,  
     JamoT,  
     Any    
  };
  static RType get (SS_UCS4 u);
  static bool  split (SS_UCS4 u, SS_UCS4* left, SS_UCS4* right);
  static const char*  string (RType);
};

#endif /*SRendClass_h*/

