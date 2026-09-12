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


#include "stoolkit/SUtil.h"
#include "stoolkit/STypes.h"
#include "stoolkit/SString.h"
#include "stoolkit/SStringVector.h"
#include "stoolkit/SProperties.h"
#include "stoolkit/SIO.h"
#include "stoolkit/SIOStream.h"
#include "stoolkit/SEncoder.h"
#include "stoolkit/SHashtable.h"
#include "swindow/SAwt.h"

#include <stdlib.h>

#ifdef USE_WINAPI
#include <wtypes.h>
#include <winreg.h> 
#include <ShlObj.h>
#else
#include <unistd.h> 
#include <pwd.h>
#include <sys/types.h>
#endif

#include <string.h>


#ifndef YUDIT_DATA
#define YUDIT_DATA "/usr/share/yudit"
#endif /* YUDIT_DATA  */

#ifndef YUDIT_PREFIX
#define YUDIT_PREFIX "/usr"
#endif /* YUDIT_PREFIX  */

#ifndef LOCALE_DIR
#define LOCALE_DIR "/usr/share/locale"
#endif /* LOCALE_DIR  */

#ifndef BIN_DIR
#define BIN_DIR "/usr/bin"
#endif /* BIN_DIR  */

#ifndef YUDIT_DOC_DIR
#define YUDIT_DOC_DIR "/usr/share/doc/yudit"
#endif /* YUDIT_DOC_DIR  */

static SString getUnusualProgramPrefix ();

// On Windows it is flat %INSTALLDIR% controlled by wininst.bat
// On Mac it is expanded ProgramPrefix/share/yudit
SString getYuditDataDir ()
{
    SString ret = getUnusualProgramPrefix();
    // Linux and Mac local install and Windows failure
    if (ret.size() == 0) {
        return SString (YUDIT_DATA);
    }
    // Windows, Mac
#ifdef USE_WINAPI
    return SString (ret);
#else 
    ret.append ("/share/yudit");
    return SString (ret);
#endif
}

// On Windows it is flat %INSTALLDIR%\locale controlled by wininst.bat
// On Mac it is expanded ProgramPrefix/share/locale
SString getLocaleDir ()
{
    SString ret = getUnusualProgramPrefix();
    // Linux and Mac local install and Windows failure
    if (ret.size() == 0) {
        return SString (LOCALE_DIR);
    }
    // Windows, Mac
#ifdef USE_WINAPI
    ret.append ("/locale");
    return SString (ret);
#else 
    ret.append ("/share/locale");
    return SString (ret);
#endif

}

// On Windows it is flat %INSTALLDIR%\bin controlled by wininst.bat
// On Mac it is expanded ProgramPrefix/bin
SString getBinDir ()
{
    SString ret = getUnusualProgramPrefix();
    // Linux and Mac local install and Windows failure
    if (ret.size() == 0) { 
        return SString (BIN_DIR);
    }
    // Windows, Mac
    ret.append ("/bin");
    return SString (ret);
}

// On Windows it is flat %INSTALLDIR%\doc controlled by wininst.bat
// On Mac it is expanded ProgramPrefix/share/doc/yudit
SString getYuditDocDir ()
{
    SString ret = getUnusualProgramPrefix();
    // Linux and Mac local install and Windows failure
    if (ret.size() == 0) {
        return SString (YUDIT_DOC_DIR);
    }
    // Windows, Mac
#ifdef USE_WINAPI
    ret.append ("/doc");
    return SString (ret);
#else 
    ret.append ("/share/yudit/doc");
    return SString (ret);
#endif
}

SString getHome ()
{
#ifdef USE_WINAPI
  // default
  SString ret ="C:/HOME";
  // First try the HOME environment
  WCHAR* henv = _wgetenv (L"HOME");
  if (henv)
  {
    SString u16fn ((const char*) henv, 2 * (unsigned int) wcslen (henv));
    SEncoder u8("utf-8");
    SEncoder u16("utf-16-le");
    SString h = u8.encode (u16.decode (u16fn));
    h.replaceAll ("\\", "/");
    SDir dir (h);
    if (dir.exists()) {
      return SString (h);
    }
    fprintf (stderr, "HOME environment variable is nonexistent directory : \"%*.*s\".\n", SSARGS (h));
  }

  WCHAR  *full = new WCHAR[MAX_PATH+1];
  CHECK_NEW (full);
  full[MAX_PATH] = 0;
  // Surpsisingly this works on Windows98SE
  if (SHGetSpecialFolderPathW (0, full, CSIDL_PERSONAL, false)) {
    SString u16fn ((const char*) full, 2 * (unsigned int) wcslen (full));
    SEncoder u8("utf-8");
    SEncoder u16("utf-16-le");
    ret = u8.encode (u16.decode (u16fn));
  } else if (SHGetSpecialFolderPathA (0, (char *) full, CSIDL_PERSONAL, false)) {
    SString s ((char*)full);
    ret = systemToUtf8 (s);
  }
  delete full;
  ret.replaceAll ("\\", "/");

  SDir dir (ret);
  if (dir.exists())
  {
     fprintf (stderr, "can not determine home directory. Using %*.*s\n",
       SSARGS(ret));
  }
  else
  {
     fprintf (stderr, "can not determine home directory. Creating %*.*s\n",
       SSARGS(ret));
     if (!dir.create())
     {
       fprintf (stderr, "Could not create %*.*s\n", SSARGS(ret));
     }
 
  }
  return ret;
#else
  // HOME comes first
  char* henv = getenv ("HOME");
  if (henv)
  {
    SString h (henv);
    if (h.size() != 0 && h[0] == '/') {
        SDir dir (h);
        if (dir.exists()) {
            return SString(h);
        } else {
           fprintf (stderr, "HOME environment variable is nonexistent directory : \"%*.*s\".\n", SSARGS (h));
        }
    } else {
       fprintf (stderr, "HOME environment variable is not absolute path: \"%*.*s\".\n", SSARGS (h));
    }
  }
  struct passwd * p = getpwuid(getuid());
  if (p!=0)
  {
    return SString (p->pw_dir);
  }
  fprintf (stderr, "can not determine home directory. Using /tmp");
  return "/tmp";
#endif
}


#define YUDIT_MAX_DATA 2048

#ifdef USE_WINAPI
static SString 
getReg (const SString& _base, const SString& _name)
{
  /* get registry entry first */
  SString key = _base; 
  SString name = _name;
  key.append ("\\"); key.append (name); key.append ((char)0);

  DWORD datasize = YUDIT_MAX_DATA;
  char* _registryBuffer = new char[datasize];
  CHECK_NEW (_registryBuffer);
  DWORD type;
  LONG opened;
  HKEY hkey=0;
  type = REG_SZ;
  SEncoder u8("utf-8");
  SEncoder u16("utf-16-le");
  SString utf16key = u16.encode (u8.decode (key));
  if (

     (opened = RegOpenKeyExW (
       HKEY_LOCAL_MACHINE, (WCHAR*) utf16key.array(), 0, KEY_READ, &hkey)
       == ERROR_SUCCESS) && 
     RegQueryValueExW (
       hkey, 0, 0, &type,
       (BYTE*) _registryBuffer,
       &datasize) == ERROR_SUCCESS  && datasize > 2 && datasize != YUDIT_MAX_DATA
       && (type == REG_SZ 
        || type == REG_EXPAND_SZ))
  {
    if (datasize < 2) {
      datasize = 2;
    }
    SString u16vle ((const char*) _registryBuffer, (unsigned int) (datasize-2));
    SString ret = u8.encode (u16.decode (u16vle));
    RegCloseKey (hkey);
    delete _registryBuffer;
    return SString(ret);
  } 
  if (opened == ERROR_SUCCESS)
  {
    RegCloseKey (hkey); hkey=0;
  }
  // Windows98SE does not implement RegOpneKeyExW and RegQueryValueExW
  if (
     (opened = RegOpenKeyExA (
       HKEY_LOCAL_MACHINE, key.array(), 0, KEY_READ, &hkey)
       == ERROR_SUCCESS) && 
     RegQueryValueExA (
       hkey, 0, 0, &type,
       (BYTE*) _registryBuffer,
       &datasize) == ERROR_SUCCESS  && datasize > 2 && datasize != YUDIT_MAX_DATA
       && (type == REG_SZ 
        || type == REG_EXPAND_SZ))
  {
    if (datasize < 1) {
      datasize = 1;
    }
    SString vle ((const char*) _registryBuffer, (unsigned int) (datasize-1));
    SString ret = systemToUtf8 (vle);
    RegCloseKey (hkey);
    delete _registryBuffer;
    return SString(ret);
  }  
  if (opened == ERROR_SUCCESS)
  {
    RegCloseKey (hkey); hkey=0;
  }
  delete _registryBuffer;
  return SString();
}

/* 
 * we dont have administrator privilege so this myght get virtualized.
 */
static bool 
setReg (const SString& _base, const SString& _name, const SString& _vle)
{
  SString key = _base; 
  SString name = _name;
  key.append ("\\"); key.append (name); key.append ((char)0);
  DWORD type;
  LONG opened;
  HKEY hkey=0;
  type = REG_SZ;
  SString vle = _vle;
  vle.append ((char)0);
  DWORD dispose=0;
  SECURITY_ATTRIBUTES sa;
  sa.nLength = sizeof (SECURITY_ATTRIBUTES);
  sa.lpSecurityDescriptor = NULL;
  sa.bInheritHandle = TRUE;
  SEncoder u8("utf-8");
  SEncoder u16("utf-16-le");
  SString utf16vle = u16.encode (u8.decode (vle));
  SString utf16key = u16.encode (u8.decode (key));
  if (
     // Windows98SE does not have the W version and it does not fail.
     (opened = RegCreateKeyExA (
       HKEY_LOCAL_MACHINE, (char*) key.array(), 0, 
       0, REG_OPTION_NON_VOLATILE,
       KEY_WRITE, &sa, &hkey, &dispose)
       == ERROR_SUCCESS) && 
     RegSetValueExA (
       hkey, 0, 0, type,
       (BYTE*)vle.array(),
       vle.size()) == ERROR_SUCCESS)
  {
    RegCloseKey (hkey);
    return true;
  } 
  if (opened == ERROR_SUCCESS)
  {
    fprintf (stderr, "Opened [%*.*s]\n", SSARGS(key));
    RegCloseKey (hkey); hkey=0;
  }
  else
  {
    fprintf (stderr, "Can not open [%*.*s]\n", SSARGS(key));
  }
  return false;
}
#endif

/* Get a different install dir only on Windows and Mac */
static SString getUnusualProgramPrefix ()
{
#ifdef USE_WINAPI
  SString akey = SString("Software\\Inui Yuko\\Yudit");
  SString key = akey;
  key.append ("\\");
  SString version = SD_YUDIT_VERSION;
  key.append (version);

  SString data=getReg(key, "Install Directory");
  if (data.size())
  {
    data.replaceAll("\\", "/");
    return SString(data);
  }
  data = getReg(akey, "Current Version");
  if (data.size())
  {
    fprintf (stderr, 
      "Can not find current version:%*.*s. trying version %*.*s\n", 
       SSARGS(version), SSARGS(data));
    SString key = akey;
    key.append ("\\");
    key.append (data);
    SString data=getReg(key, "Install Directory");
    if (data.size())
    {
      data.replaceAll("\\", "/");
      return SString(data);
    }
  }

  fprintf (stderr, "Can not find registry entry: [%*.*s\\Install Directory]\n", 
       SSARGS(key));
#else
# ifdef USE_OSX
  SDir dir("/Volumes/Yudit/Yudit.app/Contents/MacOS");
  if (dir.exists()) {
    fprintf (stderr, "Using %*.*s as prefix instead of %s\n",
      SSARGS(dir.getName()), YUDIT_PREFIX);
    return SString(dir.getName());
  } 
  SString comp(YUDIT_PREFIX);
  SDir dir2(comp);
 // This will tell to use LOCALE_DIR, YUDIT_DATA, BINDIR
  if (dir2.exists()) {
    return SString("");
  }
  SString home = getHome();
  home.append ("/Applications/Yudit.app/Contents/MacOS");
  SDir dir3(home);
  if (dir3.exists()) {
    fprintf (stderr, "Using %*.*s as datadir instead of %s\n",
      SSARGS(dir3.getName()), YUDIT_PREFIX);
    return SString(dir3.getName());
  }
 // This will tell to use LOCALE_DIR, YUDIT_DATA, BINDIR
  return SString("");
# endif
#endif
  return SString ("");
}

bool setProgramPrefix (const SString& str)
{
#ifdef USE_WINAPI
  SString key = SString("Software\\Inui Yuko\\Yudit");
  setReg(key, "Current Version", SD_YUDIT_VERSION);
  key.append ("\\");
  key.append (SD_YUDIT_VERSION);

  bool ret = setReg(key, "Install Directory", str);
  if (!ret)
  {
    fprintf (stderr, 
       "Can not set registry entry: [%*.*s\\Install Directory] to [%*.*s]\n", 
       SSARGS(key), SSARGS(str));
    return false;
  }
  fprintf (stderr, 
       "Registry entry [%*.*s\\Install Directory] has been set to [%*.*s]\n", 
       SSARGS(key), SSARGS(str));
  return ret;
#endif
  return false;
}

bool
loadProperties (const SString& file, SProperties* in)
{
  SFile f(file);
  if (f.size() < 0) 
  {
     return false;
  }
  SFileImage im = f.getFileImage ();
  if (im.size()<0) 
  {
     return false;
  }
  SProperties p(SString (im.array(), im.size()));
  in->merge(p);
  return true;
}

bool
loadProperties (SProperties* in)
{
  SString c1 = getHome();
  c1.append ("/.yudit/yudit.properties");
  SString c2 = getYuditDataDir();
  c2.append ("/config/yudit.properties");

  bool ret2 = loadProperties (c2, in);
  // override with home
  bool ret1 = loadProperties (c1, in);
  return ret1 | ret2;
}

bool
saveProperties (const SString& file, const SProperties& out)
{
  SProperties orig;
  loadProperties (file, &orig);
  orig.merge (out);
  SFile f(file);
  SOutputStream os = f.getOutputStream();
  if (!os.isOK()) return false;

  SWriter writer(os);
  if (!writer.isOK()) return false;
  SString s(orig.toString());
  s.append ("\n# End of Yudit-");
  s.append (SD_YUDIT_VERSION);
  s.append (" properties\n");
  if (!writer.write (s)) return false;
  if (!writer.close()) return false;
  return true;
}

static bool translateInited = false;

static SString currentBind;
static SString currentLanguage;
static SStringVector defaultDomains ("/usr/lib/locale,/usr/share/locale");
static SStringVector currentDomains;
static SFileImage moFileImage;
static SHashtable<SString> *moCache=0;

void
initTranslate (const SString& bindIn, const SString& domain)
{
  currentBind = bindIn;
  currentDomains = defaultDomains;
  currentDomains.insert (0, domain);
  translateInited=false;
}

void
setLanguage (const SString& _str)
{
  if (_str != currentLanguage)
  {
    translateInited=false;
  }
  currentLanguage = _str;
  return;
}

SString getLanguage ()
{
  return SString (currentLanguage);
  
}

/**
 * Get system language or fb 
 */
SString getSystemLanguage (const SString& fb)
{
  char* henv = getenv ("LANG");
  if (henv)
  {
    SString h (henv);
    /* no support for country yet */
    if (h.size()> 2 && h[2] == '_') h.truncate (2);
    return SString(h);
  }
  return SString(fb);
}

/* SS_WORD32 mingle */
#define reorder_bytes(b) \
  (((b>>24)&0xff) \
    | ((b>>8)&0xff00) \
    | ((b<<8)&0xff0000) \
    | ((b<<24)&0xff000000))

SString translate (const SString& str)
{
  if (moCache == 0) moCache = new SHashtable<SString>();
  if (!translateInited)
  {
    moCache->clear();
    translateInited = true;
    SStringVector plist;
    for (unsigned int i=0; i<currentDomains.size(); i++)
    {
      SString s = currentDomains[i];
      s.append ("/");
      s.append (currentLanguage);
      s.append ("/LC_MESSAGES");
      plist.append (s);
    } 
    SString s (currentBind);
    s.append (".mo");
    SFile file = SFile (s, plist);
    moFileImage = file.getFileImage();
  }
  if (moFileImage.size()<=0) return SString(str);
  if (str.size() == 0) return SString(str);
  /* find text */
  if (moCache->get(str) != 0)
  {
    return SString(*moCache->get (str));
  }

  /* Direct processing of gettext mo file.
        byte
             +------------------------------------------+
          0  | magic number = 0x950412de                |
             |                                          |
          4  | file format revision = 0                 |
             |                                          |
          8  | number of strings                        |  == N
             |                                          |
         12  | offset of table with original strings    |  == O
             |                                          |
         16  | offset of table with translation strings |  == T
             |                                          |
         20  | size of hashing table                    |  == S
             |                                          |
         24  | offset of hashing table                  |  == H
             |                                          |
             .                                          .
             .    (possibly more entries later)         .
             .                                          .
             |                                          |
          O  | length & offset 0th string  ----------------.
      O + 8  | length & offset 1st string  ------------------.
              ...                                    ...   | |
O + ((N-1)*8)| length & offset (N-1)th string           |  | |
             |                                          |  | |
          T  | length & offset 0th translation  ---------------.
      T + 8  | length & offset 1st translation  -----------------.
              ...                                    ...   | | | |
T + ((N-1)*8)| length & offset (N-1)th translation      |  | | | |
             |                                          |  | | | |
          H  | start hash table                         |  | | | |
              ...                                    ...   | | | |
  H + S * 4  | end hash table                           |  | | | |
             |                                          |  | | | |
             | NUL terminated 0th string  <----------------' | | |
             |                                          |    | | |
             | NUL terminated 1st string  <------------------' | |
             |                                          |      | |
              ...                                    ...       | |
             |                                          |      | |
             | NUL terminated 0th translation  <---------------' |
             |                                          |        |
             | NUL terminated 1st translation  <-----------------'
             |                                          |
              ...                                    ...
             |                                          |
             +------------------------------------------+
  */
  
  unsigned char* array = (unsigned char*)moFileImage.array();
  SS_WORD32  magic = *((SS_WORD32*)&array[0]);
  bool swap = (magic != 0x950412de);
  if (swap) magic = reorder_bytes (magic);
  if (magic != 0x950412de)
  {
    moCache->put (str, str);
    return SString(str);
  }
  SS_WORD32 count = *((SS_WORD32*)&array[8]);
  SS_WORD32 koffset = *((SS_WORD32*)&array[12]);
  SS_WORD32 toffset = *((SS_WORD32*)&array[16]);
  if (swap)
  {
     count = reorder_bytes (count);
     koffset = reorder_bytes (koffset);
     toffset = reorder_bytes (toffset);
  }
  SString trans (str);
  for (unsigned int i=0; i<count; i++)
  {
    SS_WORD32 cklen = *((SS_WORD32*)&array[koffset + i * 8]);
    SS_WORD32 ckoffs = *((SS_WORD32*)&array[koffset + i * 8 + 4]);
    if (swap)
    {
       cklen = reorder_bytes (cklen);
       ckoffs = reorder_bytes (ckoffs);
    }
    SString key((char*)&array[ckoffs], cklen);
    if (key == str)
    {
      SS_WORD32 ctlen = *((SS_WORD32*)&array[toffset + i * 8]);
      SS_WORD32 ctoffs = *((SS_WORD32*)&array[toffset + i * 8 + 4]);
      if (swap)
      {
         ctlen = reorder_bytes (ctlen);
         ctoffs = reorder_bytes (ctoffs);
      }
      trans = SString((char*)&array[ctoffs], ctlen);
      break;
    }
  }
  moCache->put (str, trans);
  return SString(trans);
}

/**
 * print the unicode value in XXXX or XXXXX or XXXXXXform
 */
SString
unicodeValueOf (const SV_UCS4& ucs4)
{
  char a[64];
  SString s;
  for (unsigned int i=0; i<ucs4.size(); i++)
  {
    if (ucs4[i] <= 0xffff) {
        snprintf (a, 64, "%04X", (unsigned int) ucs4[i]);
    } else if (ucs4[i] <= 0xfffff) {
        snprintf (a, 64, "%05X", (unsigned int) ucs4[i]);
    } else if (ucs4[i] <= 0xffffff) {
        snprintf (a, 64, "%06X", (unsigned int) ucs4[i]);
    } else if (ucs4[i] <= 0xfffffff) {
        snprintf (a, 64, "%07X", (unsigned int) ucs4[i]);
    } else {
        snprintf (a, 64, "%08X", (unsigned int) ucs4[i]);
    }
    if (i!=0) s.append (" ");
    s.append (a);
  }
  return SString (s);

}

SString
unicodeValueOf (const SString& str)
{
  SEncoder enc("utf-8");
  SV_UCS4 ucs4 = enc.decode (str);
  return unicodeValueOf (ucs4);
}
   
/**
 * Get the square root or if there are decimals, the value that is less
 * than square root.
 * @param sq is the square
 */
unsigned long
ss_sqrtlong (unsigned long sq)
{
  /* we don't check for negative. assume...*/
  if (sq<2) return sq; /* zero or 1 */

  unsigned long x=sq;
  unsigned long r=x-1;
  while (r < x)
  {
      x = r; r= (x+(sq/x))/2;
  }
  return x;
}
/**
 * arcus tangent in a scale of 32 
 * @param x is the vector x param
 * @param y is the vector y param
 * @return  the angle with step 1
 * 0 => 12:00, 8 => 3:00, 16 => 6:00, 24 => 9:00
 * returns 32 if x and y are both zero.
 */
int
ss_atan32 (int x, int y)
{
  if (x==0 && y ==0) return 32;
  /* normalize data - x and y positive and x < y */
  bool xneg = x < 0; if (xneg) x = -x;
  bool yneg = y < 0; if (yneg) y = -y;
  bool xyflip = (y < x);
  if (xyflip)
  {
    int tmp = x; x = y; y = tmp;
  }
  int slope = (100 * x) / y;

  int ret = 4;
  /* ../bin/angle.pl data is used */
  if (slope < 10) ret = 0; 
  else if  (slope < 31) ret = 1; 
  else if  (slope < 54) ret = 2;
  else if  (slope < 83) ret = 3;
  /* now 0..45 degrees is nicely split up into 0..4 */

  /* go back the transform we did at the beginning in reverse order */
  if (xyflip) ret = 8 - ret; 
  if (yneg) ret = 16 - ret; 
  if (xneg) ret = 32 - ret;

  return ret % 32;
}

static long maxlong=0;
static long maxsqrtlong=0;

/**
 * Get the maximum long value on the machine. 
 * The hard way.  Don't trust header files.  Gaspar Sinai 
 */
long getMaxLong()
{
  if (maxlong==0)
  {
    long l = 1;
    long ml = 1;
    while (l>0)
    {
      maxlong = ml;
      l = l<<1;
      ml = (ml<<1)|1;
    }
  }
  return maxlong;
}

long getMaxSqrtLong ()
{
  if (maxsqrtlong==0)
  {
    getMaxLong();
    maxsqrtlong = ss_sqrtlong (maxlong)-1;
  }
  return maxsqrtlong;
}

static long maxint=0;
static long maxsqrtint=0;
/**
 * Get the maximum long value on the machine. 
 * The hard way.  Don't trust header files.  Gaspar Sinai 
 */
int getMaxInt()
{
  if (maxint==0)
  {
    int i = 1;
    int mi = 1;
    while (i>0)
    {
      maxint = i;
      i = i<<1;
      mi = (mi<<1)|1;
    }
  }
  return maxint;
}

int getMaxSqrtInt ()
{
  if (maxsqrtint==0)
  {
    getMaxInt();
    maxsqrtint = (int)ss_sqrtlong ((long)maxlong)-1;
  }
  return maxsqrtint;
}

bool
isWindows ()
{
#ifdef USE_WINAPI
  OSVERSIONINFO ovi;
  ovi.dwOSVersionInfoSize = sizeof (ovi);
  GetVersionEx (&ovi);
  return (ovi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);
#else /*USE_WINAPI*/
  return false;
#endif
}

bool
isWinAPI ()
{
#ifdef USE_WINAPI
  return true;
#else /*USE_WINAPI*/
  return false;
#endif
}

/**
 * check if the command could be executed in a shell as is.
 */
bool
commandExists (const SString& str)
{
  SString comm = str;
  comm.replaceAll ("\\", "/");
  comm.replaceAll ("//", "/");
  char* path = getenv ("PATH");
  if (!path)
  {
    path = getenv ("path");
    if (!path)
    {
      path = getenv ("Path");
    }
  }
  SStringVector pv;
  if (path && path[0] != 0)
  {
#ifdef USE_WINAPI
    SStringVector v(path, ";");
#else
    SStringVector v(path, ":");
#endif
    for (unsigned int i=0; i<v.size(); i++)
    {
      SString s = v[i];
      s.replaceAll ("\\", "/");
      s.replaceAll ("//", "/");
      pv.append (s);
    }
  }
  SFile f0(comm, pv);
  if (f0.size()>0) return true;
#ifdef USE_WINAPI
  SString ecomm = comm;
  ecomm.append (".exe");
  SFile f1(ecomm, pv);
  if (f1.size()>0) return true;
  comm.append (".com");
  SFile f2(ecomm, pv);
  if (f2.size()>0) return true;
#endif
  return false;
}

SString systemToUtf8 (const SString& str) {
#ifdef USE_WINAPI
  if (str.size() == 0) {
    return SString (str);
  }

  int  blen = 4 * str.size ();
  WCHAR*  utf16 = new WCHAR[blen];
  CHECK_NEW (utf16);

  char* cstr = str.cString();

  int rlen = MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, cstr, str.size(), utf16, blen);
  if (rlen > 0) {
    int i;
    SV_UCS4 uni;
    for (i=0; i<rlen; i++) {
      // FIXME UTF16 surrogates 
      SS_UCS4 u16 = (SS_UCS4) utf16[i];
      // 0xd800 and 0xdc00
      if (u16 >= 0xd800 &&  u16 < 0xe000) {
         if (i+1 == rlen) {
           uni.append (u16);
         } else if (u16 < 0xdc00) {
           // Sequence is 
           // U+D800-U+DBFF (High Surrogates)
           // U+DC00-U+DFFF (Low Surrogates) 
           SS_UCS4 full = (u16 & 0x3ff) << 10;
           i++;
           u16 = (SS_UCS4) utf16[i];
           full += (u16 & 0x3ff);
           full += 0x10000;
           uni.append (full);
         } else {
           uni.append (u16);
           i++;
           u16 = (SS_UCS4) utf16[i];
           uni.append (u16);
         }
      } else {
        uni.append (u16);
      }
    }
    delete [] utf16;
    delete [] cstr;
    SEncoder enc("utf-8");
    return (enc.encode (uni));
  } 
  delete [] utf16;
  delete [] cstr;
  return SString (str);
#else
  SString ret;
  for (unsigned int i=0; i<str.size(); i++) {
     if (str[i] >= 'a' && str[i] <= 'z') {
        ret.append ((char) (str[i] - 'a' + 'A'));
     } else {
        ret.append ((char) (str[i]));
     }
  }
  return SString (ret);
#endif
}

SString utf8ToSystem (const SString& str) {
#ifdef USE_WINAPI
  if (str.size() == 0) {
    return SString (str);
  }

  SEncoder encoder ("utf-8");
  SV_UCS4 input = encoder.decode (str);

  int  blen = 2 * input.size ();
  WCHAR*  utf16 = new WCHAR[blen];
  CHECK_NEW (utf16);
  int i;
  int count = 0;
  for (i=0; i<input.size(); i++) {
     SS_UCS2 c = (SS_UCS2) input[i];
     if (c < 0x10000) {
        utf16[count++] = c;
     } else {
        c = (c - 0x10000);
        WCHAR h = ((c >> 10) & 0x3ff) | 0xd800;
        WCHAR l = (c & 0x3ff) | 0xdc00;
        utf16[count++] = h;
        utf16[count++] = l;
     }
  }
  blen = 16 * count + 1;
  char*  out = new char[blen];
  CHECK_NEW (out);
  out[blen-1] = 0;

  int rlen = WideCharToMultiByte (CP_ACP, 0, utf16, count, out, blen, NULL, NULL);
  if (rlen > 0) {
    SString ret (out, rlen);
    delete [] utf16;
    delete [] out;
    return (SString(ret));
  } 
  delete [] utf16;
  delete [] out;
  return SString (str);
#else
  SString ret;
  for (unsigned int i=0; i<str.size(); i++) {
     if (str[i] >= 'A' && str[i] <= 'Z') {
        ret.append ((char) (str[i] - 'A' + 'a'));
     } else {
        ret.append ((char) (str[i]));
     }
  }
  return SString (ret);
#endif
}

SStringVector getUserPath (const SString& property, const SString& directory) {
  SStringVector outDataPath;

  SString homeDir = getHome();
  homeDir.append ("/");
  SString c1 = homeDir;
  c1.append (".yudit/");
  c1.append (directory);
  outDataPath.append (c1);

  SProperties p;
  loadProperties (&p);
  if (p.get (property))
  {
// C: drive hack
#ifdef USE_WINAPI
      SStringVector v(p[property], ",;");
#else
      SStringVector v(p[property], ",:;");
#endif
      for (unsigned int j=0; j<v.size(); j++)
      {
         SString vle = v[j];
         if (vle.size() == 0) continue;
         if (vle.find ("~/") == 0) {
            vle.replace("~/", homeDir);
         }
         outDataPath.append (vle);
      }
  }
  SString c2 = getYuditDataDir();
  c2.append ("/");
  c2.append (directory);
  outDataPath.append (c2);
  return SStringVector(outDataPath);
} 
