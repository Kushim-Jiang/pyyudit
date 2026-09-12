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
 
#ifndef SUtil_h
#define SUtil_h
#include "stoolkit/STypes.h"
#include "stoolkit/SString.h"
#include "stoolkit/SProperties.h"

SString getHome ();

// Normally /usr/share/locale/
SString getLocaleDir ();
// Normally /usr/bin/
SString getBinDir ();
// Normally  /usr/share/yudit/data
SString getYuditDataDir ();
// Normally /usr/share/doc/yudit
SString getYuditDocDir ();
// Normally  /usr/share/yudit/data
SString getD ();

// On windows istaller sets C:\Program Files\Yudit, mytool can also set it. 
bool setProgramPrefix (const SString& str);

bool loadProperties (SProperties* in);
bool loadProperties (const SString& file, SProperties* in);
bool saveProperties (const SString& file, const SProperties& out);

SStringVector getUserPath (const SString& property, const SString& directory);

/* call them in this order */
void initTranslate (const SString& bindIn, const SString& domain);
void setLanguage (const SString& lang);
SString getLanguage ();
SString getSystemLanguage (const SString& fb);
SString translate (const SString& str);

SString unicodeValueOf (const SV_UCS4& str);
SString unicodeValueOf (const SString& str);

/* cost effective math routines */
unsigned long ss_sqrtlong (unsigned long sq);

/*
 * arcus tangent in a scale of 32 
 * 0 => 12:00, 8 => 3:00, 16 => 6:00, 24 => 9:00
 */
int ss_atan32 (int x, int y);

long getMaxLong();
long getMaxSqrtLong();

int getMaxInt();
int getMaxSqrtInt();

bool isWindows (); /* for nt this is false */
bool isWinAPI (); /* Microsoft tthis is true */
bool commandExists (const SString& str);

/* Filnames with codepages are converted to utf-8 here. */
SString systemToUtf8 (const SString& str);
SString utf8ToSystem (const SString& str);


#endif /* Util_h */
