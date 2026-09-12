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
 
#ifndef SIOStream_h
#define SIOStream_h

#include "SEvent.h"
#include "SIO.h"
#include "SStringVector.h"
#include "SString.h"

class SReader : public SJob, public SEventTarget
{
public:
  // If target is zero this will be a blocking function.
  SReader (const SInputStream& i, SEventTarget* t=0);
  SReader (const SInputStream& i, const SStringVector& sep, SEventTarget* t=0);
  ~SReader ();
  // These are for public use
  bool read (SString* s);

  virtual int run ();
  virtual bool read (const SEventSource* s, const SString& m);
  virtual void error (const SEventSource* s);
  bool isOK();
  bool close();
protected:
  bool finishFlag; 
  bool errFlag;
  bool more;
  SString buffer;
  SStringVector line;
  SStringVector sepVector;
  SEventTarget* trg;
  SInputStream in;
};

class SWriter : public SJob, public SEventTarget
{
public:
  SWriter (const SOutputStream& i, SEventTarget* t=0);
  ~SWriter ();
  // This is for public use
  bool write (const SString& s);
  bool isErrror() { return errFlag; }
  bool isFinished() { return writeCount==0; }

  virtual int run ();
  virtual bool write (const SEventSource* s);
  virtual void error (const SEventSource* s);
  bool isOK();
  bool close();
protected:
  int writeCount;
  bool finishFlag; 
  bool errFlag;
  SOutputStream out;
  SEventTarget* trg;
};
#endif /* SIOStream_h */
