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
 
#include "SEvent.h"
#include "SEventBSD.h"
#include "SExcept.h"

#ifndef USE_WINAPI
#include <unistd.h>
#else
#include <winsock.h>
#include <fcntl.h>
#include <io.h>
#endif

class SEvtBuffer
{
public:
  SEvtBuffer(SEventSource::Type t, long i);
  ~SEvtBuffer ();
  bool close();
  SEventSource::Type type;
  long id;
  int count;
  bool ok;
  bool closed;
};

SEvtBuffer::SEvtBuffer(SEventSource::Type t, long i)
{
  count = 1;
  closed = false;
  id = i;
  type = t;
  ok = false;
}

SEvtBuffer::~SEvtBuffer()
{
  close ();
}

bool
SEvtBuffer::close()
{
  int ret =0;
  if (!closed)
  {
    closed = true;
    switch (type)
    {
    case SEventSource::PIPE:
#ifdef USE_WINAPI
         ret = (id < 0) ? 0 : CloseHandle ((char*)id);
         if(!ret) ok=false;
         id = -1;
#else
         ret = (id < 0) ? 0 : ::close (id);
//fprintf (stderr, "SEvtBuffer::close closing pipe=%d\n", (int)id);
         if(ret!=0) ok=false;
         id = -1;
#endif
         break;
    case SEventSource::FILE:
         ret = (id < 0) ? 0 : ::close (id);
         if(ret!=0) ok=false;
         id = -1;
         break;
    case SEventSource::SOCKET:
    case SEventSource::SERVER:
#ifdef USE_WINAPI
         ret = (id < 0) ? 0 : ::closesocket (id);
#else
         ret = (id < 0) ? 0 : ::close (id);
#endif
         if(ret!=0) ok=false;
         id = -1;
         break;
    case SEventSource::JOB:
    case SEventSource::TIMER:
         break;
    }
  }
  return (ok);
}


/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 * This library is not multi-threaded. Therefor we need event handlers.
 */

STimer::STimer (long timeout) : SEventSource (TIMER, timeout)
{
  setOK(true);
}

STimer::~STimer()
{
}

STimer*
STimer::newTimer(long timeout, SEventTarget* target)
{
  STimer* t = new STimer(timeout);
  CHECK_NEW(t);
  SEventHandler::addTimer (t, target);
  return t;
}

SJob::SJob (long priority) : SEventSource (JOB, priority)
{
  setOK(true);
}

SJob::SJob (void) : SEventSource (JOB, 0)
{
}

SJob::~SJob()
{
}

/**
 *  if false is returned we want to remove this guys.
 */
int
SJob::run ()
{
  return -1;
}

SEventSource::SEventSource (Type t, long sid)
{
  SEvtBuffer* buf = new SEvtBuffer(t, sid);
  CHECK_NEW (buf);
  shared = buf;
}

SEventSource::SEventSource (void)
{
  SEvtBuffer* buf = new SEvtBuffer(FILE, -1);
  CHECK_NEW (buf);
  shared = buf;
}

bool
SEventSource::close()
{
  return ((SEvtBuffer*) shared)->close();
}

SEventSource::SEventSource(const SEventSource& s)
{
  SEvtBuffer* buf = (SEvtBuffer*) s.shared;
  buf->count++;
  shared = buf;
}

SEventSource&
SEventSource::operator = (const SEventSource& s)
{
  if (&s != this)
  {
    SEvtBuffer* buf = (SEvtBuffer*) shared;
    buf->count--;
    if (buf->count == 0) delete buf;
    buf = (SEvtBuffer*) s.shared;
    buf->count++;
    shared = buf;
  }
  return *this;
}

/**
 * When an event source dies, we need to remove it from the event 
 * distributor's list.
 */
SEventSource::~SEventSource ()
{
  SEventHandler::remove (this);
  SEvtBuffer* buf = (SEvtBuffer*) shared;
  buf->count--;
  if (buf->count == 0) delete buf;
}

SEventSource::Type
SEventSource::getType ()
{
  return ((SEvtBuffer*) shared)->type;
}

void
SEventSource::setOK (bool ok)
{
  ((SEvtBuffer*) shared)->ok=ok;
}

bool
SEventSource::isOK()
{
  return ((SEvtBuffer*) shared)->ok;
} 

long
SEventSource::getId ()
{
  return ((SEvtBuffer*) shared)->id;
}

/**
 * When an event target dies, we need to remove it from the event 
 * distributor's list.
 */

SEventTarget::SEventTarget (void)
{
}

SEventTarget::~SEventTarget ()
{
  SEventHandler::remove (this);
}

/**
 * When re-implementing return true if you want repeated timeouts.
 * @param s is the source where the event came from. 
 */
/*ARGSUSED*/
bool
SEventTarget::timeout (const SEventSource* s)
{
  return false;
}

/**
 * caused by a jinished job...
 */
/*ARGSUSED*/
bool
SEventTarget::done (const SEventSource* s)
{
  return false;
}

/**
 * When re-implementing return true if you want repeated timeouts.
 * @param s is the source where the event came from. 
 */
/*ARGSUSED*/
void
SEventTarget::error (const SEventSource* s)
{
  return;
}

/**
 * A server socket accepts...
 * @param s is the source where the event came from. 
 * return false if the server socket should be closed.
 */
/*ARGSUSED*/
bool
SEventTarget::serve (const SEventSource* s)
{
  return false;
}

/**
 * You need to reimplement this to read.
 * @param s is the source.
 * @param m is the string read. it will be a SStringNull in case of fail.
 * @return true if you want to read more.
 */
/*ARGSUSED*/
bool
SEventTarget::write (const SEventSource* s)
{
  return false;
}

/**
 * You need to reimplement this to read.
 * @param s is the source.
 * @param m is the string read. it will be a SStringNull in case of fail.
 * @return true if you want to read more.
 */
/*ARGSUSED*/
bool
SEventTarget::read (const SEventSource* s, const SString& m)
{
  return false;
}

/**
 * return 0 for no read
 */
/*ARGSUSED*/
int
SEventTarget::readable (const SEventSource* s)
{
  return 1;
}

/**
 *  THIS IS THE STATIC EVENT HANDLER.
 */
SEventHandlerImpl* SEventHandler::delegate=0;
SEventHandler h;

/**
 * Set the implementation if not set.
 */
SEventHandler::SEventHandler()
{
  if (delegate==0)
  {
    delegate = new SEventBSD();
    CHECK_NEW(delegate);
  }
}

SEventHandler::~SEventHandler()
{
  if (delegate) delete delegate;
}

/**
 * Set the new Implementation. DELETE the old one.
 */
bool
SEventHandler::setImpl (SEventHandlerImpl* impl)
{
  bool old = implemented();
  if (old) delete delegate;
  delegate = impl;
  return old;
}

bool
SEventHandler::implemented()
{
  return (delegate != 0);
}

void
SEventHandler::addTimer (STimer* s, SEventTarget* t)
{
  delegate->addTimer (s, t);
}

void
SEventHandler::addServer (SServerStream* s, SEventTarget* t)
{
  delegate->addServer (s, t);
}

void
SEventHandler::addInput (SInputStream* s, SEventTarget* t)
{
  delegate->addInput (s, t);
}

void
SEventHandler::addJob (SJob* s, SEventTarget* t)
{
  delegate->addJob (s, t);
}

void
SEventHandler::addOutput(SOutputStream* s, SEventTarget* t, const SString& m)
{
  delegate->addOutput (s, t, m);
}

void
SEventHandler::remove (SEventTarget* t)
{
  delegate->remove (t);
}

void 
SEventHandler::remove (SEventSource* s)
{
  delegate->remove (s);
}

void
SEventHandler::start()
{
  delegate->start ();
}

void
SEventHandler::exit()
{
  delegate->exit ();
}

/**
 * Handle the next single event. Return false if 
 * event handler exit was called.
 */
bool
SEventHandler::next()
{
  return delegate->next ();
}

/**
 * This is a single implementation of the event handler
 */
SEventHandlerImpl::SEventHandlerImpl()
{
}

/**
 * The tables will clean up.
 */
SEventHandlerImpl::~SEventHandlerImpl()
{
}

void
SEventHandlerImpl::addServer (SServerStream* s, SEventTarget* t)
{
}

void
SEventHandlerImpl::addTimer (STimer* s, SEventTarget* t)
{
}

void
SEventHandlerImpl::addJob (SJob* s, SEventTarget* t)
{
}

void
SEventHandlerImpl::addInput (SInputStream* s, SEventTarget* t)
{
}

void
SEventHandlerImpl::addOutput (SOutputStream* s, SEventTarget* t, const SString& m)
{
}

void
SEventHandlerImpl::remove (SEventTarget* s)
{
}

void 
SEventHandlerImpl::remove (SEventSource* s)
{
}

void
SEventHandlerImpl::start()
{
}

void
SEventHandlerImpl::exit()
{
}

/**
 * Process a next event. Return false if exit was called.
 */
bool
SEventHandlerImpl::next()
{
  return false;
}
