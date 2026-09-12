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
 
#ifndef SIO_h
#define SIO_h

#include "SString.h"
#include "SStringVector.h"
#include "SEvent.h"
#include "SBinVector.h"

/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 * This library is not multi-threaded. Therefor we need event handlers.
 */
class SIO 
{
public:
  SIO (SEventSource::Type type);
  virtual ~SIO();

  const SInputStream& getInputStream();
  const SOutputStream& getOutputStream();
protected:
  SInputStream  in;
  SOutputStream out;
};

class SFileImage 
{
public:
  SFileImage (void);
  SFileImage (long fd, long size, bool write);
  SFileImage (const SFileImage& in);
  SFileImage& operator = (const SFileImage& in);
  long size() const;
  // Writing is not supported yet.
  char* array();
  char operator[] (unsigned int pos)  { return array()[pos]; }
  ~SFileImage();
private:
  void* shared;
};

/**
 * A directory in a filesystem
 */
class SDir
{
public:
  enum SEntry { SE_DIR, SE_FILE };
  SDir (void);
  SDir (const SString& name);
  SDir (const SDir& dir);
  SDir& operator = (const SDir& dir);
  virtual ~SDir ();
  SStringVector list (SEntry e);
  SStringVector list (const SStringVector &patterns, SEntry e=SE_FILE);
  SString getName() const;
  SString getUnixName() const;
  bool cd (const SString& newDir);
  bool exists() const;
  bool readable() const;
  bool create() const;
private:
  SString           name;
};

class SStdIO : public SIO
{
public:
  SStdIO ();
  virtual ~SStdIO ();
  const SInputStream& getInputStream();
  const SOutputStream& getOutputStream();
  const SOutputStream& getErrorOutputStream();
protected:
  SOutputStream err;
};

class SPipe : public SIO
{
public:
  SPipe (const SString& command);
  SPipe (void);
  virtual ~SPipe ();
  const SInputStream& getInputStream();
  const SOutputStream& getOutputStream();
  int wait ();
protected:
  long openPipe(bool output);
  SString command;
  SBinVector<long> waitHandles;
};


class SFile : public SIO
{
public:
  SFile (const SString& filename);
  SFile (const SString& filen, const SStringVector& pt);
  SFile (const SFile&);
  SFile& operator = (const SFile&);
  virtual ~SFile ();

  long        size();
  bool        truncate (long _size);

  const SString& getName() const;
  // Writing is not supported for non-mmap platforms.
  const SFileImage&   getFileImage(bool write=false);
  const SInputStream& getInputStream();
  const SOutputStream& getOutputStream();

private:
  bool recursiveFind (const SString& fileName, SString dirName, int level);
  SString   name;
  SFileImage map;
};

SString getTemporaryFileName ();

class SSocket  : public SIO
{
public:
  SSocket (const SString host, int port);
  SSocket (const SSocket&);
  SSocket& operator = (const SSocket&);
  virtual ~SSocket ();

  const SInputStream& getInputStream();
  const SOutputStream& getOutputStream();
private:
  void openSocket(SOutputStream* o);
  unsigned int address[6]; // Four is used now.
  SString host;
  int port;
};

class SServer  : public SIO
{
public:
  SServer (int port);
  virtual ~SServer ();
};


#endif /* SIO_h */
