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
 
#ifndef SAwt_h
#define SAwt_h

#include "swindow/SWindow.h"
#include "swindow/SFontNative.h"

#include "stoolkit/SExcept.h"
#include "stoolkit/SString.h"

/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 * This is the abstract widget toolkit
 */

/**
 * This class should be implemented for a window toolkit implementation
 */
class SAwtImpl
{
public:
  SAwtImpl();
  virtual ~SAwtImpl();
  virtual SWindow* getWindow (SWindowListener*l, const SString& name);
  virtual SFontNative* getFont (const SString& enc);
  virtual void setEncoding (const SString& str);
};

/**
 * This is the awt handler.
 */
class SAwt
{
public:
  // changed cache size from 2000 to 4000 because of syntax highlighting
  // changed cache size from 4000 to 6000
  SAwt (bool cacheOn=true, unsigned int cacheSize=6000);
  ~SAwt (); // Destroys the implementation.
  static void setImpl (SAwtImpl* impl);
  static SWindow* getWindow (SWindowListener* l, const SString& name);
  static void setEncoding (const SString& str);
  static SFontNative* getFont (const SString& enc);
  static bool implemented();
  static bool hasGUI ();
  static void setScale (double _scale);
  static double getScale ();
  static int getRasterScale ();
  static SAwtImpl* getDelegate ();
private:
  static SAwtImpl* delegate;
  static double scale;
};

#endif /* SAwt_h */
