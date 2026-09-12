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
 
#ifndef SEventBSD_h
#define SEventBSD_h

#include "SEvent.h"
#include "SBinVector.h"
#include "SBinHashtable.h"
#include "SHashtable.h"
#include "SUniqueID.h"

/**
 * Tieouts are searched sequenctially when removing source or target
 */
class SClient
{
public:
  enum ClientType {READER, WRITER, SERVER, JOB, TIMER};
  SClient (SEventSource* s, SEventTarget* t, ClientType type);
  ~SClient();
  ClientType    getType();
  SEventSource* source;
  SEventTarget* target;
  SString       data;
  unsigned int  progress;
  long          value;
  SUniqueID     id;
private:
  ClientType    type;
};

typedef SBinHashtable<SClient*> SClientHashtable;
typedef SBinVector<SClient*> SClientVector;

typedef SBinVector<long> STimeoutVector;
typedef SBinVector<long> SFDVector;

/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 * This is a default unix event handler implementation.
 */
class SEventBSD : public SEventHandlerImpl
{
public:
  SEventBSD(void);
  virtual ~SEventBSD();
  /* One source can have only one target! Same target
   * can be used for more source */
  virtual void addJob (SJob* s, SEventTarget* t);
  virtual void addServer (SServerStream* s, SEventTarget* t);
  virtual void addTimer (STimer* s, SEventTarget* t);
  virtual void addInput (SInputStream* s, SEventTarget* t);

  virtual void addOutput (SOutputStream* s, SEventTarget* t, const SString& m);

  virtual void remove (SEventTarget* t); 
  virtual void remove (SEventSource* s); 

  // Send async write back to SEventSource

  virtual void start();
  virtual void exit();
  virtual bool next();

private:
  virtual void addTimer (SEventSource* s, SEventTarget* t);
  void remove (SClient* s); 
  bool sendData (SClient* c, char* buffer, int len);

  // This will move the timeouts. You may need it if the app
  // is up for more than a year.
  void            seedTimer();

  STimeoutVector  timeoutVector;
  SFDVector       readVector;
  SFDVector       writeVector;
  SFDVector       jobVector;

  SClientVector   timeoutClient;
  SClientVector   readClient;
  SClientVector   writeClient;
  SClientVector   jobClient;

  /* using pointer */
  SClientHashtable      sourceHashtable;
  SClientHashtable      targetHashtable;
  /* using id */
  SClientHashtable      clientHashtable;

  void    remakeReadFd ();
  void    remakeWriteFd ();

  void*           delegate;

  int             readMax;
  int             writeMax;
  bool            up;
  unsigned int    roundRobin;
};


#endif /* SEventBSD_h */
