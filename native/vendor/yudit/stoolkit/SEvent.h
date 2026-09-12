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
 
#ifndef SEvent_h
#define SEvent_h

#include "SExcept.h"
#include "SString.h"
#include "SStringVector.h"

/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 * This library is not multi-threaded. Therefor we need event handlers.
 */
class SEventSource
{
public:
  enum Type {FILE, SOCKET, SERVER, JOB, TIMER, PIPE};
  SEventSource (Type t, long systemId);
  SEventSource (void);
  SEventSource (const SEventSource& s);
  virtual ~SEventSource();
  SEventSource& operator = (const SEventSource& s);

  Type getType ();
  long getId ();
  void setOK(bool ok);
  bool isOK();
  bool close();

private:
  void *shared;
};

/**
 * These are the specific event sources that the implementation of
 * the event menager should handle
 */

typedef SEventSource SInputStream;
typedef SEventSource SOutputStream;
typedef SEventSource SServerStream;

class SJob : public SEventSource
{
public:
  SJob(long priority);
  SJob(void);
  virtual ~SJob();
  virtual int run ();// Normally it returns 0, on fisnich it returns -1.
                     // On urgent data it returns 1
};

/**
 *  The event target can receive all kinds of events from the 
 *  event sources above.
 */
class SEventTarget 
{
public:
  SEventTarget ();
  virtual ~SEventTarget ();

  /* Any Event Source */
  virtual void error (const SEventSource* s);

  /* Output Stream  Done*/
  virtual bool write (const SEventSource* s);

  /* Timer Done */
  virtual bool timeout (const SEventSource* s);

  /* Job Done */
  virtual bool done (const SEventSource* s);

  /* Input Stream Read */
  virtual bool read (const SEventSource* s, const SString& m);

  /* Input Stream Is Readable */
  virtual int readable (const SEventSource* s);

  /* Server Stream Accept */
  virtual bool serve (const SEventSource* s);

};

class STimer : public SEventSource
{
public:
  STimer (long timeout);
  virtual ~STimer ();
  /* This one automatically add it to the event manager */
  static STimer* newTimer (long timeout, SEventTarget* t);

};

/**
 * If you have an external event handler reimplement this
 */
class SEventHandlerImpl
{
public:
  SEventHandlerImpl();
  virtual ~SEventHandlerImpl();
  /* One source can have only one target! Same target
   * can be used for more source */
  virtual void addJob (SJob* s, SEventTarget* t);
  virtual void addServer (SServerStream* s, SEventTarget* t);
  virtual void addTimer (STimer* s, SEventTarget* t);
  virtual void addInput (SInputStream* s, SEventTarget* t);
  // Send async write back to SEventSource
  virtual void addOutput (SOutputStream* s, SEventTarget* t, const SString& m);

  virtual void remove (SEventTarget* t); 
  virtual void remove (SEventSource* s); 


  virtual void start();
  virtual void exit();
  virtual bool next();
};

/**
 * There can be only one event handler.
 * So this is pretty much a static class.
 */
class SEventHandler
{
public:
  SEventHandler (void);
  ~SEventHandler (); // Destroys the implementation.
  static bool setImpl (SEventHandlerImpl* impl);
  static bool implemented();

  static void addJob (SJob* s, SEventTarget* t);
  static void addServer (SServerStream* s, SEventTarget* t);
  static void addTimer (STimer* s, SEventTarget* t);
  static void addInput (SInputStream* s, SEventTarget* t);
  static void addOutput (SOutputStream* s, SEventTarget* t, const SString& m);

  static void remove (SEventTarget* t); 
  static void remove (SEventSource* s); 

  static void start();
  static void exit();
  static bool next();

private:
  static SEventHandlerImpl* delegate;
};

#endif /* SEvent_h */
