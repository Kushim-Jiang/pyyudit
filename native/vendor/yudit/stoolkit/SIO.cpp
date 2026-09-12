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

/*
 For windows debugging #define UNICODE
*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stoolkit/SUtil.h>

#define STDIN 0
#define STDOUT 1

#ifndef USE_WINAPI
#include <unistd.h>
#include <sys/wait.h>
#else
#include <io.h>
#include <direct.h>
#endif

#include "SIO.h"
#include "SExcept.h"
#include "SEncoder.h"

#ifdef USE_WINAPI
#define S_MYOFLAGS O_BINARY
#else
#include <dirent.h>
#define S_MYOFLAGS 0
#endif

#ifdef  USE_WINAPI
# include <winsock.h>
# include <io.h>
# define ERRNO WSAGetLastError()
#else
# include <errno.h>
# include <netdb.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <netinet/in.h>
# include <unistd.h>
# define ERRNO errno
#endif

#ifdef HAVE_MMAP
#include <fcntl.h>
#include <sys/mman.h>
#if !defined (__svr4__) && !defined (__SVR4) && defined (sun)
extern "C" {
extern int munmap(void *, size_t);
}
#endif
#if !defined (MAP_FAILED)
#define MAP_FAILED ((void *)-1)
#endif
#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#endif
#endif

#ifdef USE_WINAPI
// Hacked for winapi This is in SEventBSD hacked to hide dependency !!!!
int gettimeofday (struct timeval* tv, void* tz);
int initsockets(bool init);
#define SBAD_SOCKET  0
#define EWOULDBLOCK TRY_AGAIN
#else
#define SBAD_SOCKET  -1
#endif

#ifdef MAX_PATH
# define SS_MAX_DIR_LENGTH          MAX_PATH
#else
# define SS_MAX_DIR_LENGTH          4096
#endif

#ifdef USE_WINAPI
typedef HANDLE SS_DIR;
#define SS_INVALID_DIR INVALID_HANDLE_VALUE
#define SS_S_ISDIR(_m) ((_m & _S_IFMT)==_S_IFDIR)
#define SS_S_ISREG(_m) ((_m & _S_IFMT)==_S_IFREG)
#else
#define SS_S_ISDIR(_m) (S_ISDIR(_m))
#define SS_S_ISREG(_m) (S_ISREG(_m))
#define SS_S_ISLNK(_m) (S_ISLNK(_m))
typedef DIR* SS_DIR;
#define SS_INVALID_DIR 0
#endif

#ifdef USE_WINAPI
static WCHAR   _currDirW [SS_MAX_DIR_LENGTH];
#else
static char    _currDir [SS_MAX_DIR_LENGTH];
#endif


static char zeroSizedReadBuffer[8];

class SFileImageBuffer
{
public:
  SFileImageBuffer(long fd, long size, bool writeflag);
  ~SFileImageBuffer();
#ifdef USE_WINAPI
  void* handle;
#endif  
  int   count;
  char* buffer;
  long  bufferSize;
  long  fd;
  bool  writeflag;
};

SFileImage::~SFileImage ()
{
  if (shared)
  {
    SFileImageBuffer* b = (SFileImageBuffer*) shared;
    b->count--;
    if (b->count==0) delete b;
  }
}

SFileImage&
SFileImage::operator = (const SFileImage& in)
{
  if (&in != this)
  {
    if (shared)
    {
      SFileImageBuffer* b = (SFileImageBuffer*) shared;
      b->count--;
      if (b->count==0) delete b;
      shared = 0;
    }
    if (in.shared)
    {
      SFileImageBuffer* b = (SFileImageBuffer*) in.shared;
      b->count++;
      shared = b;
    }
  }
  return *this;
}

SFileImage::SFileImage (void)
{
  shared = 0;
}

SFileImage::SFileImage (long fd, long size, bool writeflag)
{
  SFileImageBuffer* b = new SFileImageBuffer (fd, size, writeflag);
  if (b->bufferSize < 0)
  {
    delete b;
    shared = 0;
  }
  else
  {
    shared = b;
  }
}

long
SFileImage::size() const
{
  if (shared == 0) return -1;
  SFileImageBuffer* b = (SFileImageBuffer*) shared;
  return b->bufferSize;
}

char*
SFileImage::array()
{
  if (shared == 0) return 0;
  SFileImageBuffer* b = (SFileImageBuffer*) shared;
  return b->buffer;
}


SFileImage::SFileImage (const SFileImage& in)
{
  if (in.shared)
  {
    SFileImageBuffer* b = (SFileImageBuffer*) in.shared;
    b->count++;
    shared = b;
  }
  else
  {
    shared = 0;
  }
}


static int sharedCount=0;
SFileImageBuffer::SFileImageBuffer(long _fd, long size, bool _writeflag)
{
  fd = _fd;
  bufferSize = size;
  count = 1;
  if (size == 0 && !_writeflag) {
    writeflag = _writeflag;
    buffer = zeroSizedReadBuffer;
    sharedCount++;
    return;
  }
#ifdef USE_WINAPI
  writeflag = _writeflag;
  handle = CreateFileMapping ((void*)fd, 0, 
		(_writeflag) ? PAGE_READWRITE : PAGE_READONLY, 
		0, 0, 0);
  if (handle==0)
  {
    CloseHandle ((void*) fd);
    fd = -1;
    buffer = 0;
    bufferSize = -1;
  }
  else
  {
    buffer = (char*) MapViewOfFile (handle, 
		(_writeflag) ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ, 
		0, 0, bufferSize);
    if (buffer ==0)
    {
      CloseHandle (handle);
      CloseHandle ((void*) fd);
      fd = -1;
      buffer = 0;
      bufferSize = -1;
    }
  }
#else
# ifdef HAVE_MMAP
  writeflag = _writeflag;
  buffer=(char*) mmap (0, bufferSize, 
	(_writeflag) ? (PROT_WRITE|PROT_READ) : PROT_READ, MAP_SHARED, fd, 0);
  if (buffer==0 || buffer==(char*)MAP_FAILED)
  {
    if (fd >=0) close (fd);
    fd = -1;
    buffer = 0;
    bufferSize = -1;
  }
# else
  writeflag = 0;
  buffer = new char[bufferSize];
  CHECK_NEW (buffer);
  if (read (fd, buffer, bufferSize)!= bufferSize)
  {
    if (fd >=0) close (fd);
    delete buffer;
    fd = -1;
    buffer = 0;
    bufferSize = -1;
  }
  if (fd >=0) close (fd);
  close (fd);
  fd = -1;
# endif
#endif
  sharedCount++;
}

SFileImageBuffer::~SFileImageBuffer()
{
  if (buffer !=0)
  {
    if (fd >= 0)
    {
#ifdef USE_WINAPI
      if (buffer != zeroSizedReadBuffer)
      {
        UnmapViewOfFile (buffer);
      }
      CloseHandle (handle);
      CloseHandle ((void*) fd);
#else
# ifdef HAVE_MMAP
      if (buffer != zeroSizedReadBuffer)
      {
        munmap ((char*)buffer, bufferSize);
      }
      ::close (fd);
# else
# endif
#endif
    }
    else
    {
      if (buffer != zeroSizedReadBuffer) {
        delete buffer;
      }
    }
  }
  sharedCount--;
  if (sharedCount==0) 
  {
    //fprintf (stderr, "SFileImageBuffer::~SFileImageBuffer OK\n");
  }
}

/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 * This library is not multi-threaded. Therefor we need event handlers.
 */

/**
 * This is the base class for all io classes
 */
SIO::SIO (SEventSource::Type t)  : in(t, -1),  out(t, -1)
{
}

/**
 * The descturctor.
 */
SIO::~SIO()
{
}

/**
 * Get an input stream
 */
const SInputStream&
SIO::getInputStream()
{
  return in;
}

/**
 * Get an output stream
 */
const SOutputStream&
SIO::getOutputStream()
{
  return out;
}

/**
 * Create an stdio object. This will have
 */
SStdIO::SStdIO () : SIO(SEventSource::FILE), err(SEventSource::FILE, -1)
{
}

SStdIO::~SStdIO ()
{
}

/**
 * Get an input stream that can be used an a standard input.
 */
const SInputStream&
SStdIO::getInputStream()
{
  if (in.getId()<0)
  {
    in = SInputStream (SEventSource::FILE, (long)0);
  }
  return in;
}

/**
 * Get an output stream that can be used an a standard output.
 */
const SOutputStream&
SStdIO::getOutputStream()
{
  if (out.getId()<0)
  {
    out = SOutputStream (SEventSource::FILE, (long)1);
  }
  return out;
}

/**
 * Get an output stream that can be used an a standard error output.
 */
const SOutputStream&
SStdIO::getErrorOutputStream()
{
  if (err.getId()<0)
  {
    err = SOutputStream (SEventSource::FILE, (long)2);
  }
  return err;
}

/**
 * Create an stdio object. This will have
 * @param c is the command to execute.
 */
SPipe::SPipe (const SString& c) : SIO(SEventSource::PIPE), command (c)
{
}

/**
 * Create an stdio object. This will have
 * @param c is the command to execute.
 */
SPipe::SPipe (void) : SIO(SEventSource::PIPE), command ("")
{
}

SPipe::~SPipe ()
{
#ifdef USE_WINAPI
/* FIMXE: You need to modify SYudit.cpp for this. 
*/
  for (unsigned int i=0; i< waitHandles.size(); i++)
  {
    PROCESS_INFORMATION* pinfo = (PROCESS_INFORMATION*) waitHandles[i];
    CloseHandle (pinfo->hThread);
    CloseHandle (pinfo->hProcess);
  }
#endif
}

/**
 * Get an input stream that can be used an a standard input.
 */
const SInputStream&
SPipe::getInputStream()
{
  if (in.getId()<0)
  {
    if (command.size()==0)
    {
      /* STDIO in */
//fprintf (stderr, "STDIO input.\n");
      in = SInputStream (SEventSource::FILE, (long)0);
      in.setOK(true);
    }
    else
    {
      long d = openPipe (false);
      in = SInputStream (SEventSource::PIPE, d);
#ifdef USE_WINAPI
      in.setOK(d>0);
#else
      in.setOK(d>=0);
#endif        
    }
  }
  return in;
}

/**
 * Get an output stream that can be used an a standard output.
 */
const SOutputStream&
SPipe::getOutputStream()
{
  if (out.getId()<0)
  {
    if (command.size()==0)
    {
      out = SOutputStream (SEventSource::FILE, (long)1);
//fprintf (stderr, "SOutputStream stdout\n");
      out.setOK(true);
    }
    else
    {
//fprintf (stderr, "SOutputStream command\n");
      long d = openPipe (true);
      out = SOutputStream (SEventSource::PIPE, d);
#ifdef USE_WINAPI
      out.setOK(d>0);
#else
      out.setOK(d>=0);
#endif        
    }
  }
  return out;
}

/**
 * Create a file from a fixed filename
 * @param f is the filename in unix format.
 */
SFile::SFile (const SString& f) : SIO(SEventSource::FILE)
{
  name = f;
}

/**
 * Create a file from a file. The file is really just the name
 * @param f is the original file
 */
SFile::SFile (const SFile& f) : SIO(SEventSource::FILE)
{
  name = f.name;
  map = f.map;
}

/**
 * Assign a file.
 */
SFile&
SFile::operator = (const SFile& f)
{
  name = f.name;
  if (this != &f) map = f.map;
  return *this;
}

/**
 * Open the file for reading and return the input stream, that
 * can be sued by a SReader
 */
const SInputStream&
SFile::getInputStream()
{
  long fd = in.getId();
  if (fd<0)
  {
    SString filename=name;

#ifdef USE_WINAPI
    filename.replaceAll("/", "\\");
    filename.append ((char) 0);
    SEncoder u8("utf-8");
    SEncoder u16("utf-16-le");
    SString res = u16.encode (u8.decode (filename));
    WCHAR* filenameW = (WCHAR *) res.array();
#else
    filename.append ((char) 0);
    char * filenameU = (char*) filename.array();
#endif
    if (filename.size()==0)
    {
      fd = (long) STDIN;
    }
    else
    {
#ifdef USE_WINAPI

      fd = (long) _wopen (filenameW, O_RDONLY | S_MYOFLAGS);
      // Windows98SE does not implement _wopen
      if (fd < 0) {
         SString filenameA = utf8ToSystem (filename);
         filenameA.append ((char)0);
         fd = (long) open (filenameA.array(), O_RDONLY | S_MYOFLAGS);
      }
#else
      fd = (long) open (filenameU, O_RDONLY | S_MYOFLAGS );
#endif
    }
  }
  SInputStream ins(SEventSource::FILE, fd);
#ifdef USE_WINAPI
  ins.setOK(fd>0);
#else
  ins.setOK(fd>=0);
#endif        
  in = ins;
  return in;
}

/**
 * Open the file for writing and return the output stream, that
 * can be sued by a SWriter
 */
const SOutputStream&
SFile::getOutputStream()
{
  int fd;
  if (out.getId()< 0) 
  {
    SString filename=name;
#ifdef USE_WINAPI
    filename.replaceAll("/", "\\");
    filename.append ((char) 0);
    SEncoder u8("utf-8");
    SEncoder u16("utf-16-le");
    SString res = u16.encode (u8.decode (filename));
    WCHAR* filenameW = (WCHAR *) res.array();
#else
    filename.append ((char) 0);
    char * filenameU = (char*) filename.array();
#endif
    if (name.size()==0)
    {
      fd = STDOUT;
    }
    else
    {
#ifdef USE_WINAPI
      fd = _wopen (filenameW, O_WRONLY | O_CREAT | O_TRUNC | S_MYOFLAGS, 0666);
      // Windows98SE does not implement _wopen
      if (fd < 0) {
         SString filenameA = utf8ToSystem (filename);
         filenameA.append ((char)0);
         fd = open (filenameA.array(), O_WRONLY | O_CREAT | O_TRUNC | S_MYOFLAGS, 0666);
      }
#else
      fd = open (filenameU, O_WRONLY | O_CREAT | O_TRUNC | S_MYOFLAGS, 0666);
#endif
    }
    SOutputStream outs (SEventSource::FILE, (long)fd);
#ifdef USE_WINAPI
    outs.setOK(fd>0);
#else
    outs.setOK(fd>=0);
#endif        
    out = outs;
  }
  return out;
}
/*
 * Go to max level and try to find the file in dir.
 * return true if found.
*/
bool
SFile::recursiveFind (const SString& fileName, SString dirName, int level)
{
    if (fileName.size() == 0) return false;
    if (level < 0) return false;
    SDir current = SDir(dirName); 
    if (!current.exists()) return false;
    if (!current.readable()) return false;
    SString currentName = current.getName();
    SString fullPath = currentName;

    fullPath.append("/");
    fullPath.append(fileName);
    fullPath.replaceAll("//", "/");

    name = currentName;
    name.append("/");
    name.append(fileName);
    name.replaceAll("//", "/");

    if (size() >= 0) 
    {
      //fprintf (stderr, "Found %*.*s level=%d\n", SSARGS (name), level);     
        return true;
    }
    //fprintf (stderr, "Tried %*.*s level=%d\n", SSARGS (name), level);     
    name = "";
    if (level == 0) return false;

    SStringVector list = current.list (SDir::SE_DIR);

    for (unsigned int i=0; i<list.size(); i++) 
    {
        SString now = currentName;
        SString newDir = list[i];
        if (newDir == "." || newDir == "..") continue;
        now.append ("/");
        now.append (newDir);
        if (recursiveFind (fileName, now, level-1))
        {
            return true;
        }
    }
    return false;
}

/**
 * Find the file by the path..
 * @parma filen is the base filename. 
 * @param pt is the list of paths it could be found.
 * @aram lookcwd is true if we also need to look into current working dir
 */
SFile::SFile (const SString& filen, const SStringVector& dataPath): SIO(SEventSource::FILE) 
{
  for (unsigned int i=0; i<dataPath.size(); i++)
  {
    SString element = dataPath[i];
    if (element.size() > 3 && element.find("/**") > 0) {
        element.truncate(element.size()-3);
        if (recursiveFind (filen, element, SB_YUDIT_SUBDIR_LEVEL))
        {
            break;
        }
    }
    if (recursiveFind (filen, element, 0))
    {
        break;
    }
  }
}

/**
 * Try to mmap the file. If does not work, try to read it.
 * @return the buffer that is a representation of the file.
 * call the size() function after this call to determine the
 * real size in a multi-user environment.
 * write flag is present but only read is allowed. Write is not
 * implemented.
 */
const SFileImage&
SFile::getFileImage(bool writeflag)
{
  if (map.size()>=0) return map;
  long s = size();
// SGC bug if ws s <= 0
  if (s <0) return map;

  SString fn=name;
#ifdef USE_WINAPI
  fn.replaceAll("/", "\\");
#endif
  fn.append ((char) 0);

#ifdef USE_WINAPI
  //SString p = ("\\\\?\\");
  //p.append (utf8ToSystem (fn));
  SEncoder u8("utf-8");
  SEncoder u16("utf-16-le");
  SString res = u16.encode (u8.decode (fn));
  WCHAR* fileNameW = (WCHAR *) res.array();
#else
  const char* fileName = fn.array();
#endif


#ifdef USE_WINAPI
  //void* fd=CreateFileW (fileNameW, 0, OF_READ | OF_SHARE_DENY_NONE);
  void* fd=CreateFileW (fileNameW, 
	(writeflag) ? (GENERIC_READ| GENERIC_WRITE) : GENERIC_READ, 
	(writeflag) ? (FILE_SHARE_READ | FILE_SHARE_WRITE) : FILE_SHARE_READ,
	0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, 0);
  if (fd==0)
  {
    // Windows98SE does not implement CreateFileW
    SString fileNameA = utf8ToSystem (fn);
    fileNameA.append ((char)0);
    fd=CreateFileA (fileNameA.array(), 
 	(writeflag) ? (GENERIC_READ| GENERIC_WRITE) : GENERIC_READ, 
	(writeflag) ? (FILE_SHARE_READ | FILE_SHARE_WRITE) : FILE_SHARE_READ,
	0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, 0);
    if (fd == 0) return map;
  }
  if (fd==0)
#else
  int fd=open ((const char*) fileName, (writeflag) ? O_RDWR : O_RDONLY);
  if (fd<0)
#endif
  {
    return map;
  }
  SFileImage newMap((long) fd, s, writeflag);
  map = newMap;
  if (map.size() < 0)
  {
#ifndef USE_WINAPI
    fprintf (stderr, "SIO: Open succeeded but mmap failed for %s\n", fileName);
#endif
  }
  return map;
}

/**
 * Get the resolved filename in unix format.
 */
const SString&
SFile::getName() const
{
   return name;
}

/**
 * return the file size or negative if it does not exist
 */
long
SFile::size()
{
  struct stat     buf;
  SString newName = name;
  newName.append ((char)0);
#ifdef USE_WINAPI
  newName.replaceAll("/", "\\");
  SEncoder u8("utf-8");
  SEncoder u16("utf-16-le");
  SString u16str = u16.encode (u8.decode (newName));
  WCHAR* currentW = (WCHAR *) u16str.array();
 
  WIN32_FIND_DATAW entry;
  SS_DIR  dir=FindFirstFileW (currentW, &entry);
  if (dir == SS_INVALID_DIR)
  {
    WIN32_FIND_DATAA entryA;
    SString system = utf8ToSystem (newName);
    system.append ((char)0);
    // Fall back to system
    char* currentA = (char *) system.array();

    // Windows98SE does not implement FindFirstFileW
    dir=FindFirstFileA (currentA, &entryA);
    if (dir == SS_INVALID_DIR)
    {
      return -1;
    }
    FindClose (dir);
    return ((long)entryA.nFileSizeHigh * (MAXDWORD+1)) + (long)entryA.nFileSizeLow;
  }
  FindClose (dir);
  return ((long)entry.nFileSizeHigh * (MAXDWORD+1)) + (long)entry.nFileSizeLow;
#else
  if (stat ((char*)newName.array(), &buf) != 0)
  {
    return -1;
  }
  return (long) buf.st_size;
#endif
}

bool
SFile::truncate (long _size)
{
  SString newName = name;
#ifdef USE_WINAPI
  newName.replaceAll("/", "\\");
  newName.replaceAll("/", "\\");
  newName.append ((char) 0);
  SEncoder u8("utf-8");
  SEncoder u16("utf-16-le");
  SString res = u16.encode (u8.decode (newName));
  WCHAR* filenameW = (WCHAR *) res.array();
#endif
  SString newNameU = newName;
  newNameU.append ((char) 0);
  char * filenameU = (char*) newName.array();
#ifdef USE_WINAPI
  int d = _wopen (filenameW, O_WRONLY | O_CREAT | O_APPEND | S_MYOFLAGS, 0666);
  // Windows98SE does not implement _wopen
  if (d < 0) {
     SString filenameA = utf8ToSystem (newName);
     filenameA.append ((char)0);
      d = open (filenameA.array(), O_WRONLY | O_CREAT | O_APPEND | S_MYOFLAGS, 0666);
  }
#else
  int d = open (filenameU, O_WRONLY | O_CREAT | O_APPEND | S_MYOFLAGS, 0666);
#endif
  if (d <=0)
  {
    return false;
  }
#ifdef USE_WINAPI
  if (::_chsize (d, _size) != 0)
  {
    close (d);
    return false;
  }
#else
  if (::ftruncate (d, _size) != 0)
  {
    close (d);
    return false;
  }
#endif
  close (d);
  return true;
}

/**
 * Destruct the file.
 */
SFile::~SFile ()
{
}

/**
 * Make a new socket abstraction class.
 * @param h is the hostname or ip address.
 * @param p is the port.
 */
SSocket::SSocket (const SString h, int p): SIO(SEventSource::SOCKET)
{
  host = h;
  port = p;
}

/**
 * Make a new server socket abstraction class.
 * @param h is the hostname or ip address.
 */
SSocket::SSocket (const SSocket& s) : SIO(SEventSource::SOCKET)
{
  host = s.host;
  port = s.port;
}

/**
 * Assign a socket.
 * do not assign streams
 */
SSocket&
SSocket::operator = (const SSocket& s)
{
  host = s.host;
  port = s.port;
  return *this;
}

/**
 * Destroy a socket.
 * The copied streams may survive the socket.
 */
SSocket::~SSocket ()
{
}

/**
 * Get an input stream that could be used by a SReader
 */
const SInputStream&
SSocket::getInputStream()
{
  if (port < 0) return out;
  if (out.isOK())
  {
     //SInputStream s (SEventSource::SOCKET, (long)dup((int)out.getId()));
     //if (s.getId() > 0) s.setOK(true);
     SOutputStream s(out);
     in = s;
     return in;
  }
  openSocket(&in);
  return in;
}

/**
 * Get an input stream that could be used by a SWriter
 */
const SOutputStream&
SSocket::getOutputStream()
{
  if (port < 0) return out;
  if (in.isOK())
  {
     //SOutputStream s (SEventSource::SOCKET, (long)dup((int)in.getId()));
     //if (s.getId() > 0) s.setOK(true);
     SOutputStream s(in);
     out = s;
     return out;
  }
  openSocket(&out);
  return out;
}
  
/**
 * Try to open a socket. Used internally.
 * For ip addressed, ipv6 is not yet supported.
 */
void
SSocket::openSocket(SOutputStream* o)
{
#ifdef USE_WINAPI 
  initsockets (true);
#endif
  int sd = socket (AF_INET, SOCK_STREAM, IPPROTO_IP);
  SOutputStream s (SEventSource::SOCKET, (long)sd);
  *o = s;
#ifdef USE_WINAPI
  if (sd == 0)
  {
#else 
  if (sd == -1)
  {
#endif
    port = -1;
    return;
  }
  char* hstring = host.cString();

  // We could use saddr.s_addr = inet_addr(address);
  // But we don't want at this stage
  if (sscanf (hstring, "%d.%d.%d.%d", 
     &address[0], &address[1], &address[2], &address[3])!=4)
  {
    struct hostent*  hostEntry = gethostbyname (hstring);
    if (hostEntry==0 || hostEntry->h_addr_list==0)
    {
      fprintf (stderr, "error: can not get ip address for \"%s\"\n", hstring);
      port = -1;
#ifdef USE_WINAPI
      closesocket(sd);
#else
      close (sd);
#endif
      delete hstring;
      return;
    }
    address[0] = hostEntry->h_addr_list[0][0];
    address[1] = hostEntry->h_addr_list[0][1];
    address[2] = hostEntry->h_addr_list[0][2];
    address[3] = hostEntry->h_addr_list[0][3];
  }
  delete hstring;
  address[0] &= 0xff;
  address[1] &= 0xff;
  address[2] &= 0xff;
  address[3] &= 0xff;

  struct sockaddr_in server;
  server.sin_addr.s_addr = htonl ((address[0]<<24) +
      (address[1] << 16) + (address[2] << 8) + address[3]);
  server.sin_port = htons (port);
  server.sin_family = AF_INET;
  
  if (connect (sd, (struct sockaddr*) &server, sizeof (server))!=0)
  {
    fprintf (stderr, "error: failed to connect to %d.%d.%d.%d:%d (%d)\n",
        address[0], address[1], address[2], address[3], port, ERRNO);
#ifdef USE_WINAPI
    closesocket(sd);
#else
    close (sd);
#endif
    return;
  }
  o->setOK(true);
}


/**
 * make a server socket. Not yet implemented.
 */
/* TODO: implement this */
SServer::SServer (int port): SIO(SEventSource::SERVER)
{
}

/**
 * Delete a server socket.
 */
SServer::~SServer ()
{
}


/**
 * Try to guess the current directory
 * GetTempFileName
 */
SDir::SDir(void) : name("/home/gsinai")
{
#ifdef USE_WINAPI
/*
  //char* buff=0;
  //if (GetFullPathNameQ ("Gazsi Bacsi", SS_MAX_DIR_LENGTH-1, _currDirW, &buff))
*/
  if (GetCurrentDirectoryW (SS_MAX_DIR_LENGTH-1, _currDirW))
  {
    
    SString u16fn ((const char*) _currDirW, (unsigned int) 2 * wcslen (_currDirW));
    SEncoder u8("utf-8");
    SEncoder u16("utf-16-le");

    name = u8.encode (u16.decode (u16fn));

  // Windows98 does not implement getCurrentDirectoryW
  } else if (GetCurrentDirectoryA (SS_MAX_DIR_LENGTH-1, (char*) _currDirW)) {
    SString ansi ((const char*) _currDirW);
    name = systemToUtf8 (ansi);
  }
  else 
  {
    name = (SString) "Z:/";
  }
  name.replaceAll("\\", "/");
  name.insert (0, "/");
#else
  if (getcwd(_currDir, SS_MAX_DIR_LENGTH-1)==0)
  {
    name = (SString) "/";
  }
  else
  {
    _currDir[SS_MAX_DIR_LENGTH-1] = 0;
    name = (SString) _currDir;
  }
#endif
}
/**
 * The directory abstraction layer
 */
SDir::SDir(const SString& _name) : name (_name)
{
  /* convert it to a real name */
  name.replaceAll ("\\", "/");
  name.replaceAll ("//", "/");
  if (name.size() == 0 || name[0] != '/')
  {
    name.insert (0, "/");
  }
#ifdef USE_WINAPI
  if (name.size() >= 2 && name[1] == ':' 
    && name[0] >= 'A' && name[0] <= 'Z')
  {
    /* C:WINDOWS */
    if (name.size() > 2 && name[2] != '/')
    {
      name.insert (2, "/");
    }
    name.insert (0, "/");
  }
  else if (name.size() >= 2 && name[1] == ':' 
    && name[0] >= 'a' && name[0] <= 'z')
  {
    char dletter = name[0] - 'a' + 'A';
    name.remove (0);
    name.insert (0, SString(&dletter, 1));
    /* d:yuko */
    if (name.size() > 2 && name[2] != '/')
    {
      name.insert (2, "/");
    }
    name.insert (0, "/");
    name.insert (0, "/");
  }
#endif
}

/**
 * Copy
 */
SDir::SDir(const SDir& dir)
{
  name = dir.name;
}

/**
 * Assign
 */
SDir&
SDir::operator = (const SDir& dir)
{
  name = dir.name;
  return *this;
}

/**
 * Delete
 */
SDir::~SDir ()
{
}

/**
 * Return the directory list.
 * @param e is SE_FILE or SE_DIR
 */
SStringVector
SDir::list (SEntry e)
{
  return list ("*", e);
}

/**
 * Return the directory list.
 * @param pattern is the pattern to look for. * is all.
 * @param e is SE_FILE or SE_DIR
 * It makes a smart list= checking checkTime 
 */
SStringVector
SDir::list (const SStringVector &patterns, SEntry e)
{
  if (name.size()==0) return SStringVector();

  SStringVector entries;

  SString _name = name;
#ifdef USE_WINAPI
  if (_name.size() > 0 && _name[0] == '/')
  {
    _name.remove (0); 
  }
  if (_name.size () == 0)
  {
    if (e!=SE_DIR)  return SStringVector();
    /* get drive letters */
    DWORD letter =GetLogicalDrives();
    SStringVector v;
    for (int i=(int)'A'; i<=(int)'Z' && letter; i++)
    {
      if (letter &1)
      {
        SString str;
        str.append ((char)i);
        str.append (':');
        v.append (str);
      }
      letter = letter >> 1;
    }
    return SStringVector (v);

  }
   _name.append("/*.*");
   _name.replaceAll("//", "/");
   _name.replaceAll("/", "\\");
   _name.append ((char)0);

   SEncoder u8("utf-8");
   SEncoder u16("utf-16-le");
   SString u16str = u16.encode (u8.decode (_name));
   WCHAR* currentW = (WCHAR *) u16str.array();

#else 
   _name.append ((char)0);
   char* currentU = (char*) _name.array();
#endif

   SS_DIR  dir;
 

#ifdef USE_WINAPI
  WIN32_FIND_DATAW entry;
  WIN32_FIND_DATAA entryA;
  dir=FindFirstFileW (currentW, &entry);
  boolean ansi = false;
  if (dir == SS_INVALID_DIR)
  {
    SString system = utf8ToSystem (_name);
    system.append ((char)0);
    // Fall back to system
    char* currentA = (char *) system.array();

    // Windows98SE does not implement FindFirstFileW
    dir=FindFirstFileA (currentA, &entryA);
    ansi = true;
  }
#else
  struct dirent*  entry;
  dir = opendir (currentU);
#endif
  if (dir == SS_INVALID_DIR)
  {
    //fprintf (stderr, "can not opendir: %s\n", currentU);
    return SStringVector(entries);
  }
  struct stat buf;
  while (true)
  {

#ifdef USE_WINAPI

    SString dnameStr;

    if (ansi) {
       dnameStr = systemToUtf8 (SString (entryA.cFileName));
       entry.dwFileAttributes = entryA.dwFileAttributes;
    } else {
       SString u16fn ((const char*) entry.cFileName, (unsigned int) 2 * wcslen (entry.cFileName));
       dnameStr = u8.encode (u16.decode (u16fn));
    }

    if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
      if (e==SE_DIR && dnameStr != ".." && dnameStr != ".")
      {
        for(unsigned int i=0; i<patterns.size(); i++)
        {
          if (dnameStr.match(patterns[i]))
          {
            entries.append (dnameStr); break;
          }
        }
      }
    }
    else if (!(entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
      if (e==SE_FILE)
      {
        for(unsigned int i=0; i<patterns.size(); i++)
        {
          if (dnameStr.match(patterns[i]))
          {
            entries.append (dnameStr); break;
          }
        }
      }
    }
    if (ansi) {
      if (!FindNextFileA (dir, &entryA))
      {
        if (GetLastError()==ERROR_NO_MORE_FILES) break;
        fprintf (stderr, "some errors...%d\n", (int) GetLastError());
        break;
      }
    } else {
      if (!FindNextFileW (dir, &entry))
      {
        if (GetLastError()==ERROR_NO_MORE_FILES) break;
        fprintf (stderr, "some errors...%d\n", (int) GetLastError());
        break;
      }
   }

#else

    if ((entry=readdir (dir))==0) break;

    SString fullName = name;
    fullName.append ('/');
    char* dname = entry->d_name;
    fullName.append (dname);
    fullName.replaceAll("//", "/");
    char* cFullName = fullName.cString();
    CHECK_NEW (cFullName);

    if (stat (cFullName, &buf) != 0 && lstat (cFullName, &buf) != 0)
    {
      fprintf (stderr, "can not stat: %s\n", cFullName);
      delete cFullName; 
      continue;
    }

    if (SS_S_ISDIR( buf.st_mode))
    {
      if (e==SE_DIR && strcmp (dname, ".")!=0 
        && strcmp (dname, "..")!=0)
      {
        SString n (dname);
        for(unsigned int i=0; i<patterns.size(); i++)
        {
          if (n.match(patterns[i]))
          {
            entries.append (n); break;
          }
        }
      }
    }
    else if ( SS_S_ISREG (buf.st_mode) || SS_S_ISLNK (buf.st_mode))
    {
      if (e==SE_FILE)
      {
        SString n (dname);
        for(unsigned int i=0; i<patterns.size(); i++)
        {
          if (n.match(patterns[i]))
          {
            entries.append (n); break;
          }
        }
      }
    }
    delete cFullName;
#endif

  }

#ifdef USE_WINAPI
  FindClose (dir);
#else
  closedir (dir);
#endif
  return SStringVector(entries);
}

/**
 * return the name of this directory
 */
SString
SDir::getUnixName() const
{
  return SString (name);
}
/**
 * return the name of this directory
 * Expos C: D: as they are.
 */
SString
SDir::getName() const
{
  SString str = name;
#ifdef USE_WINAPI
  if (str.size() > 0)
  {
    str.remove (0);
  }
#endif
  return SString (str);
}

/**
 * check if directory exists 
 */
bool
SDir::exists() const
{
  SString _name = name;
#ifdef USE_WINAPI
  if (_name == "/") return true;
  if (_name.size() > 0 && _name[0] == '/')
  {
    _name.remove (0); 
  }
  if (_name.size() == 2 && _name[1] == ':' 
      && _name[0] >= 'A' && _name[0] <= 'Z')
  {
    DWORD letter=GetLogicalDrives();
    return ((letter & (1 << (_name[0] - 'A'))) != 0);
  }
  if (_name.size() == 2 && _name[1] == ':' 
      && _name[0] >= 'a' && _name[0] <= 'z')
  {
    DWORD letter=GetLogicalDrives();
    return ((letter & (1 << (_name[0] - 'a'))) != 0);
  }
  // TODO Network 'Places'.
  if (_name.find ("/") < 0) 
  {
  }
  _name.replaceAll("/", "\\");
  _name.append ((char)0);
  SEncoder u8("utf-8");
  SEncoder u16("utf-16-le");
  SString u16str = u16.encode (u8.decode (_name));
  WCHAR* currentW = (WCHAR *) u16str.array();
 
  WIN32_FIND_DATAW entry;
  SS_DIR  dir=FindFirstFileW (currentW, &entry);
  if (dir == SS_INVALID_DIR)
  {
    WIN32_FIND_DATAA entryA;
    SString system = utf8ToSystem (_name);
    system.append ((char)0);
    // Fall back to system
    char* currentA = (char *) system.array();

    // Windows98SE does not implement FindFirstFileW
    dir=FindFirstFileA (currentA, &entryA);
    if (dir == SS_INVALID_DIR)
    {
      return false;
    }
    if (entryA.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) return true;
    return false;
  }
  if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) return true;
  return false;
#else
  struct stat buf;
  char* current = _name.cString();
  CHECK_NEW (current);
  if (stat (current, &buf) != 0)
  {
    delete current;
    return false;
  }
  if (!SS_S_ISDIR( buf.st_mode))
  {
    delete current;
    return false;
  }
  delete current;
  return true;
#endif
}

/**
 * Try to create the directory. 
 * @return true on success
 */
bool
SDir::create () const
{
  SString s = name;

#ifdef USE_WINAPI
  if (s.size() > 0 && s[0] == '/')
  {
    s.remove (0); 
  }
  s.replaceAll("/", "\\");
  s.append ((char) 0);
  SEncoder u8("utf-8");
  SEncoder u16("utf-16-le");
  SString u16str = u16.encode (u8.decode (s));

  WIN32_FIND_DATAW entry;
  int status= _wmkdir ((WCHAR*) u16str.array());
  if (status != 0) {
     // Windows98SE does not implement _wmkdir
     SString ansi = utf8ToSystem (s);
     ansi.append ((char)0);
     status= mkdir (ansi.array ());
     if (status != 0) {
       return false;
     }
  }
#else
  s.append ((char) 0);
  int status = mkdir (s.array(), 0755);
  if (status == -1 && errno != EEXIST) return false;
#endif
  return exists();
}

/**
 * check if directory readable 
 */
bool
SDir::readable() const
{
  SString _name = name;
#ifdef USE_WINAPI
  if (_name == "/") return true;
  if (_name.size() > 0 && _name[0] == '/')
  {
    _name.remove (0); 
  }
  if (_name.size() == 2 && _name[1] == ':' 
      && _name[0] >= 'A' && _name[0] <= 'Z')
  {
    DWORD letter=GetLogicalDrives();
    return ((letter & (1 << (_name[0] - 'A'))) != 0);
  }
  if (_name.size() == 2 && _name[1] == ':' 
      && _name[0] >= 'a' && _name[0] <= 'z')
  {
    DWORD letter=GetLogicalDrives();
    return ((letter & (1 << (_name[0] - 'a'))) != 0);
  }
  // TODO Network 'Places'.
  if (_name.find ("/") < 0) 
  {
  }
  _name.replaceAll("/", "\\");

#else

  char* current = _name.cString();
  CHECK_NEW (current);

#endif

  SS_DIR dir;

#ifdef USE_WINAPI

  WIN32_FIND_DATAW entry;
  SEncoder u8("utf-8");
  SEncoder u16("utf-16-le");
  _name.append ((char)0);
  SString u16str = u16.encode (u8.decode (_name));
  dir=FindFirstFileW ((WCHAR*) u16str.array(), &entry);
  if (dir==SS_INVALID_DIR)
  {
    WIN32_FIND_DATAA entryA;
    SString system = utf8ToSystem (_name);
    // Fall back to system
    char* currentA = (char *) system.array();

    // Windows98SE does not implement FindFirstFileW
    dir=FindFirstFileA (currentA, &entryA);
    if (dir==SS_INVALID_DIR) return false;
  }
#else
  dir = opendir (current);
  if (dir==SS_INVALID_DIR)
  {
    delete current;
    return false;
  }
#endif

#ifdef USE_WINAPI
  FindClose (dir);
#else
  closedir(dir);
  delete current;
#endif

  return true;
}

/**
 * Change directory 
 * ToDO DWORD GetLogicalDriveStrings (DWROD, char* lpBuffer);
 */
bool
SDir::cd (const SString& _newDir)
{
  SString newDir = _newDir;
  newDir.replaceAll("\\", "/");
  newDir.replaceAll("//", "/");
  
  if (newDir.size() == 0)
  {
    return false;
  }
  SStringVector cw = SStringVector(name, "/");
#ifdef USE_WINAPI
  if (newDir.size() >= 2 && newDir[1] == ':' 
    && newDir[0] >= 'A' && newDir[0] <= 'Z')
  {
    /* C:WINDOWS */
    if (newDir.size() > 2 && newDir[2] != '/')
    {
      newDir.insert (2, "/");
    }
    newDir.insert (0, "/");
    cw.clear();
  }
  else if (newDir.size() >= 2 && newDir[1] == ':' 
    && newDir[0] >= 'a' && newDir[0] <= 'z')
  {
    char dletter = newDir[0] - 'a' + 'A';
    newDir.remove (0);
    newDir.insert (0, SString(&dletter, 1));
    /* d:yuko */
    if (newDir.size() > 2 && newDir[2] != '/')
    {
      newDir.insert (2, "/");
    }
    newDir.insert (0, "/");
    newDir.insert (0, "/");
    cw.clear();
  }
#else
  if (newDir[0] == '/')
  {
    cw.clear();
  }
#endif
  /* relative */ 
  SStringVector nw = SStringVector (newDir, "/");
  for (unsigned int i=0; i<nw.size(); i++)
  {
     if (nw[i].size()==0) continue;
     if (nw[i]==".") continue;
     if (nw[i]=="..")
     {
        if  (cw.size()==0)
        {
          //fprintf (stderr, "cwsize..\n");
          return false;
        }
        if (cw.size()==1) 
        {
          cw.clear();
        }
        else
        {
          cw.truncate (cw.size()-1);
        }
        continue;
     }
     cw.append (nw[i]);
  }
  SString nd = cw.join ("/");
  nd.insert (0, "/");
  SDir d (nd);
  /* cd up till we find a directory */
  bool good = true;
  while (!d.exists() && cw.size()>0)
  {
    cw.truncate (cw.size()-1);
    nd = cw.join ("/");
    nd.insert (0, "/");
    d = SDir(nd);
    good = false;
  }
  name = d.name;
  return good;
}

/**
 * Wait for all waithandles 
 * return false if at least on of them failed.
 */
int 
SPipe::wait ()
{
  int exitcode = 0;
#ifdef USE_WINAPI
  for (unsigned int i=0; i< waitHandles.size(); i++)
  {
    DWORD ret;
    PROCESS_INFORMATION* pinfo = (PROCESS_INFORMATION*) waitHandles[i];
    WaitForSingleObject (pinfo->hProcess, INFINITE);
    GetExitCodeProcess (pinfo->hProcess, &ret);
    if (ret) exitcode = (int) ret;
    CloseHandle (pinfo->hThread);
    CloseHandle (pinfo->hProcess);
  }
  waitHandles.clear();
  return exitcode;
#else
  for (unsigned int i=0; i< waitHandles.size(); i++)
  {
    int status;
    //pid_t ret = 
    waitpid ((pid_t) waitHandles[i], &status, 0);
    if (status) exitcode = status;
  }
  waitHandles.clear();
  return exitcode; /* FIXME: did not need it - not implemented. */
#endif
}
/**
 * Open a pipe and return the file descriptor to it.
 * Chances are that 
 * @param command the shell command line.
 * @param output is true if we write to the command.
 */
long 
SPipe::openPipe(bool output)
{
#ifdef USE_WINAPI
   HANDLE pp[2];
   SECURITY_ATTRIBUTES sa;
   memset (&sa, 0, sizeof (sa)); 
   sa.nLength = sizeof (sa);
   sa.lpSecurityDescriptor = 0; 
   sa.bInheritHandle = 0; /* without this pipe would close */
   if (!CreatePipe (&pp[0], &pp[1], &sa, 0))
   {
     fprintf (stderr, "openPipe failed.\n");
     return -1;
   }
   STARTUPINFOW sinfo;
   memset (&sinfo, 0, sizeof (sinfo));
   sinfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
   sinfo.wShowWindow = SW_SHOWDEFAULT;

   STARTUPINFOA sinfoA;
   memset (&sinfoA, 0, sizeof (sinfoA));
   sinfoA.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
   sinfoA.wShowWindow = SW_SHOWDEFAULT;

   HANDLE hProcess = GetCurrentProcess();
   HANDLE dupedHandle = INVALID_HANDLE_VALUE; 
   HANDLE myHandle = INVALID_HANDLE_VALUE; 
   /* ugly but works...*/
   if (output)
   {
     /* dup the read side, make it inheritible, and close the original */
     myHandle = pp[1];
     DuplicateHandle(hProcess, pp[0], hProcess, &dupedHandle,
                0, TRUE, DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);
     sinfo.hStdInput = dupedHandle;
     sinfo.hStdOutput = INVALID_HANDLE_VALUE;
     sinfo.hStdError = INVALID_HANDLE_VALUE;

     sinfoA.hStdInput = dupedHandle;
     sinfoA.hStdOutput = INVALID_HANDLE_VALUE;
     sinfoA.hStdError = INVALID_HANDLE_VALUE;
   }
   else
   {
     /* dup the read side, make it inheritible, and close the original */
     myHandle = pp[0];
     DuplicateHandle(hProcess, pp[1], hProcess, &dupedHandle,
                0, TRUE, DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);
     sinfo.hStdInput = INVALID_HANDLE_VALUE;
     sinfo.hStdOutput = dupedHandle;
     sinfo.hStdError = INVALID_HANDLE_VALUE;

     sinfoA.hStdInput = INVALID_HANDLE_VALUE;
     sinfoA.hStdOutput = dupedHandle;
     sinfoA.hStdError = INVALID_HANDLE_VALUE;
   }

   PROCESS_INFORMATION* pinfo = new PROCESS_INFORMATION[1];

   SEncoder u8("utf-8");
   SEncoder u16("utf-16-le");
   SString cutf8 = command;
   cutf8.append ((char)0);
   SString cmdlineString = u16.encode (u8.decode (cutf8));
   SString cmdlineStringA = utf8ToSystem(cutf8);
   WCHAR* clW = (WCHAR *) cmdlineString.cString();

   if (!CreateProcessW (
        NULL, // module name 
        clW, // command line 
        NULL, // not inheritable handles
        NULL, // not inheritable thread handle
        TRUE, // handles not inherited 
        CREATE_DEFAULT_ERROR_MODE | DETACHED_PROCESS, // no console
        NULL, // use parent's environment
        NULL, // use parent's starting directory
        &sinfo, // STARTUP_INFO
        pinfo) // PROCESS_INFO
     // Windows98SE does not implement CreateProcessW

    && !CreateProcessA (
        NULL, // module name 
        cmdlineStringA.cString(), // command line 
        NULL, // not inheritable handles
        NULL, // not inheritable thread handle
        TRUE, // handles not inherited 
        CREATE_DEFAULT_ERROR_MODE | DETACHED_PROCESS, // no console
        NULL, // use parent's environment
        NULL, // use parent's starting directory
        &sinfoA, // STARTUP_INFO
        pinfo)) // PROCESS_INFO
   {

     fprintf (stderr, "Could not create process [%s]\n", clW);
     CloseHandle (myHandle);
     CloseHandle (dupedHandle);
     delete pinfo;
     return -1;
   }
   /* we need to close this because it got duped */
   CloseHandle (dupedHandle);

   /* wait till program starts */
   // wont link ::WaitForInputIdle (pinfo->hProcess, 5000);

   waitHandles.append ((long)pinfo);
   return (long) myHandle;
#else
   SString cmdlineString (command);
   int pp[2];
   if (pipe (pp) == -1)
   {
     fprintf (stderr, "openPipe failed.\n");
     return -1;
   }
   char* clString = cmdlineString.cString();
   int pid = fork();
   if (pid>0)
   {
     delete clString;
     waitHandles.append (pid);
     if (output)
     {
       close (pp[0]);
       //fprintf (stderr, "return parent %d\n", pp[1]);
       return pp[1];
     }
     //fprintf (stderr, "return parent %d\n", pp[0]);
     close (pp[1]);
     return pp[0];
   }
   else if (pid<0)
   {
     close (pp[1]);
     close (pp[0]);
     return -1;
   }
   else
   {
     for (int i=3; i<1024; i++)
     {
       if (i!=pp[0] && i!= pp[1])
       {
         close (i);
       }
     }
     /**
      * TODO: check if fdup2 works
      */
     if (output)
     {
       close (pp[1]);
       close (0);
       if (dup (pp[0]) !=0)
       {
         fprintf (stderr, "fdup failed\n");
       }
//fprintf (stderr, "child %d %d\n", pp[0], pp[1]);
       close (pp[0]);
     }
     else
     {
       close (pp[0]);
       close (1);
       if (dup (pp[1]) != 1)
       {
         fprintf (stderr, "fdup failed\n");
       }
//fprintf (stderr, "return child %d\n", pp[1]);
       close (pp[1]);
     }
     // it will goo via mytool -pipecmd
     execl ("/bin/sh", "sh", "-c", clString, (char*) 0);
     delete clString;
     exit (127);
   }
#endif
} 

/**
 * Create a temporary file.
 */
#ifdef USE_WINAPI
static WCHAR   _tmpFileW [SS_MAX_DIR_LENGTH];
#endif

SString
getTemporaryFileName ()
{
#ifdef USE_WINAPI
  // Windows98SE W does not work.
  if (!GetTempPathW (SS_MAX_DIR_LENGTH-1, _currDirW))
  {
    if (!GetTempPathA (SS_MAX_DIR_LENGTH-1, (char*) _currDirW))
    {
      fprintf (stderr, "Can not get temporary directory.\n");
      return SString ("");
    }
    if (!GetTempFileNameA ((char*)_currDirW, "yudit", 0, (char*) _tmpFileW))
    {
      fprintf (stderr, "Can not get temporary filename.\n");
      return SString ("");
    }
    SString ansi ((char*) _tmpFileW); 
    SString utf8 = systemToUtf8 (ansi);
    return SString (utf8);
  }
  if (!GetTempFileNameW (_currDirW, L"yudit", 0, _tmpFileW))
  {
    fprintf (stderr, "Can not get temporary filename.\n");
    return SString ("");
  }
  SString u16fn ((const char*) _tmpFileW, (unsigned int) 2 * wcslen (_tmpFileW));
  SEncoder u8("utf-8");
  SEncoder u16("utf-16-le");

  SString name = u8.encode (u16.decode (u16fn));
  return SString (name);

#else
  SString ret("/tmp");
  int pid= getpid();
  ret.append ("/yudit");
  ret.print (pid);
  ret.append (".tmp");
  return SString (ret);
#endif
}
