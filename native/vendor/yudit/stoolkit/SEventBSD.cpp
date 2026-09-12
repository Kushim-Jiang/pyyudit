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
#define SHOW_BUGS 0 

#include "SEvent.h"
#include "SEventBSD.h"
#include "SExcept.h"

#include <errno.h>
#include <string.h>
#include <stdio.h>

#ifndef USE_WINAPI
#include <signal.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#endif /* ! USE_WINAPI */

#include <sys/types.h>
#ifndef USE_WINAPI
#include <unistd.h>
#include <sys/time.h>
#else
#include <winsock.h>
#include <time.h>
#endif

#ifdef __sun__
#ifndef __svr4__
extern "C"
{
void bzero(void *s, size_t n);
int gettimeofday(struct timeval *, void *);
int select (int width, fd_set* readfds, fd_set* writefds,
            fd_set* exceptfds, struct timeval* timeout);
}
#endif
#endif

#ifdef USE_WINAPI
WSADATA* wsaData=0;
WSADATA  wsaDataStruct;
#endif



static int sEventBSDSignal =0;
static void initSignals (bool init);

static long substract (const struct timeval& from, const struct timeval& what);

/*
 * Make these part of the class if multi-instance is needed
 */
class SEventDelegate
{
public:
  SEventDelegate(void) {}
  ~SEventDelegate() {}
  struct timeval  baseTime;
  fd_set          readFD;
  fd_set          writeFD;
};

#ifdef USE_WINAPI
// Hacked for winapi
int gettimeofday (struct timeval* tv, void* tz);
int initsockets(bool init);
#define SBAD_SOCKET  0
#define EWOULDBLOCK TRY_AGAIN
#else
#define SBAD_SOCKET  -1
#endif


int selectHack (int readSize, fd_set *ro, int writeSize, fd_set *wo,
  int exceptSize, fd_set *ex, struct timeval* t);

static int nonblockHack (long id);

/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 * This is the default unix event handler implementation.
 */
SClient::SClient (SEventSource* s, SEventTarget* t, ClientType _type) : data()
{
  source = s;
  target = t;
  type = _type;
  value = s->getId();
  progress = 0;
}

SClient::ClientType
SClient::getType ()
{
  return type;
}

SClient::~SClient()
{
}

/**
 * This is a single implementation of the event handler
 */
SEventBSD::SEventBSD(void) : SEventHandlerImpl()
{
  delegate = new SEventDelegate();
  roundRobin = 0;
  up = true;
  readMax = 0;
  writeMax = 0;
  initSignals(true);
  remakeWriteFd();
  remakeReadFd();
  gettimeofday (&((SEventDelegate*)delegate)->baseTime, 0);
  seedTimer();
  CHECK_NEW (delegate);
}

/**
 * The tables will clean up.
 */
SEventBSD::~SEventBSD()
{
  initSignals(false);
  delete (((SEventDelegate*)delegate));
}

/**
 * Add a timer to the list of event sources.
 * @param tim is the timer event source
 * @param t is the target of events
 */
void
SEventBSD::addTimer (STimer* tim, SEventTarget* t)
{
  if (sourceHashtable.get (tim))
  {
// SGC BUG.
#if SHOW_BUGS
    fprintf (stderr, "attempted to add same event source (timer) twice.\n");
#endif
    return;
  }
  SEventSource* s = tim;
  addTimer (s, t);
}

void 
SEventBSD::addTimer (SEventSource* s, SEventTarget* t)
{
  SClient* c = new SClient (s, t, SClient::TIMER);
  CHECK_NEW (c);

  /* Check if there is already somebody there...*/
  targetHashtable.put (t, c, false);
  sourceHashtable.put (s, c, false);
  clientHashtable.put((const SString&)c->id, c);

  struct timeval timenow;
  struct timeval timeout;

  gettimeofday (&timenow, 0);

  // If time is set back 
  if (timenow.tv_sec < ((SEventDelegate*)delegate)->baseTime.tv_sec)
  {
    ((SEventDelegate*)delegate)->baseTime.tv_sec = timenow.tv_sec;
    ((SEventDelegate*)delegate)->baseTime.tv_usec = timenow.tv_usec;
  }
  timeout.tv_sec = timenow.tv_sec; timeout.tv_usec = timenow.tv_usec;

  timeout.tv_sec += ((unsigned long)c->value)/1000;
  timeout.tv_usec += (((unsigned long)c->value)%1000) * 1000;

  timeout.tv_sec += timeout.tv_usec / 1000000;
  timeout.tv_usec = timeout.tv_usec % 1000000;

  long to = substract (timeout, ((SEventDelegate*)delegate)->baseTime);
  unsigned int p = timeoutVector.appendSorted (to);
  if (to != timeoutVector[p] && timeoutClient.size() + 1 != timeoutVector.size())
  {
     fprintf (stderr, "SEventBSD: bad news for timers\n");
  }
  timeoutClient.insert (p, c);
  //fprintf (stderr, "SEventBSD: add timer %u\n", p);
}

/**
 * Add a server to the list of event sources.
 * @param tim is the server event source
 * @param t is the target of events
 */
void
SEventBSD::addServer (SServerStream* st, SEventTarget* t)
{
  if (sourceHashtable.get (st))
  {
#if SHOW_BUGS
    fprintf (stderr, "attempted to add save event source (server) twice.\n");
#endif
    return;
  }
  SEventSource* s = st;
  SClient* c = new SClient (s, t, SClient::SERVER);
  CHECK_NEW (c);
  nonblockHack (c->value);

  /* Check if there is already somebody there...*/
  targetHashtable.put (t, c, false);
  sourceHashtable.put (s, c, false);
  clientHashtable.put ((const SString&)c->id, c);

  unsigned int p = readVector.appendSorted (c->value);
  readClient.insert (p, c);
  remakeReadFd();
}

/**
 * Add an input stream to the list of event sources.
 * @param ins is the input event source
 * @param t is the target of events
 */
void
SEventBSD::addInput (SInputStream* ins, SEventTarget* t)
{
  if (sourceHashtable.get (ins))
  {
#if SHOW_BUGS
    fprintf (stderr, "attempted to add same event source (input) twice.\n");
#endif
    return;
  }
  SEventSource* s = ins;
  SClient* c = new SClient (s, t, SClient::READER);
  CHECK_NEW (c);
  nonblockHack (c->value);

  /* Check if there is already somebody there...*/
  targetHashtable.put (t, c, false);
  sourceHashtable.put (s, c, false);
  clientHashtable.put((const SString&)c->id, c);

  unsigned int p = readVector.appendSorted (c->value);
  readClient.insert (p, c);
  remakeReadFd();
}

/**
 * Add a job to the list of event sources.
 * @param sjob is the input event source
 * @param t is the target of events
 */
void
SEventBSD::addJob (SJob* sjob, SEventTarget* t)
{
  if (sourceHashtable.get (sjob))
  {
#if SHOW_BUGS
    fprintf (stderr, "attempted to add same event source (job) twice.\n");
#endif
    return;
  }
  SEventSource* s = sjob;
  SClient* c = new SClient (s, t, SClient::JOB);
  CHECK_NEW (c);

  targetHashtable.put (t, c, false);
  sourceHashtable.put (s, c, false);
  clientHashtable.put((const SString&)c->id, c);

  unsigned int p = jobVector.appendSorted (c->value);
  //fprintf (stderr, "SEventBSD: addjob %u - full=%u\n", p, jobClient.size());
  if (c->value != jobVector[p] && jobClient.size() + 1 
       != jobVector.size())
  {
     fprintf (stderr, "SEventBSD: bad news for jobs\n");
  }
  jobClient.insert (p, c);
}

/**
 * Add an output stream to the list of event sources.
 * @param outs is the input event source
 * @param t is the target of events
 */
void
SEventBSD::addOutput (SOutputStream* outs, SEventTarget* t, const SString& m)
{
  if (sourceHashtable.get (outs))
  {
    fprintf (stderr, "attempted to add save event source (output) twice.\n");
    return;
  }
  SEventSource* s = outs;
  SClient* c = new SClient (s, t, SClient::WRITER);
  CHECK_NEW (c);
  c->data = m;
  
  nonblockHack (c->value);
  /* Check if there is already somebody there...*/
  targetHashtable.put (t, c, false);
  sourceHashtable.put (s, c, false);
  clientHashtable.put((const SString&)c->id,c);

  unsigned int p = writeVector.appendSorted (c->value);
  writeClient.insert (p, c);
  remakeWriteFd();
}

/**
 * Remove all wires that connect the target to an event source
 * @param t is the target
 */
void
SEventBSD::remove (SEventTarget* t)
{
  while (true)
  {
    SClient* cl = targetHashtable.get (t);
    if (cl == 0)
    {
      //fprintf (stderr, "SEventBSD::remove - target already got removed.\n");
      return;
    }
    remove (cl);
  }
}

/**
 * Remove all wires that connect the target to an event source
 * @param s is the source
 */
void 
SEventBSD::remove (SEventSource* s)
{
  while (true)
  {
    SClient* cl = sourceHashtable.get (s);
    if (cl == 0)
    {
      //fprintf (stderr, "SEventBSD::remove - source already got removed.\n");
      return;
    }
    remove (cl);
  }
}

/**
 * remove the client from sources
 * This is used internally.
 */
void
SEventBSD::remove (SClient* c)
{
  if (c==0)
  {
    fprintf (stderr, "SEventBSD::remove tried to remove zero client.\n");
    return;
  }

  if (clientHashtable.get ((const SString&)c->id)==0) return;
  int cindex=-1;
  switch (c->getType())
  {
  case SClient::WRITER: 
    cindex=writeClient.find (c);
    //fprintf (stderr, "removing writer %u\n", cindex);
    if (cindex<0)
    {
       /* this should not be an error - it removed itself -*/
       //fprintf (stderr, "SEventBSD::remove can not find writer.\n");
       remakeWriteFd();
       break;
    }
    writeVector.remove (cindex);
    writeClient.remove (cindex);
    remakeWriteFd();
    break;
  case SClient::READER: 
  case SClient::SERVER: 
    cindex=readClient.find (c);
    //fprintf (stderr, "removing reader %u\n", cindex);
    if (cindex<0)
    {
       /* this should not be an error - it removed itself -*/
       //fprintf (stderr, "SEventBSD::remove can not find reader.\n");
       //fprintf (stderr, "SEventBSD::readvector:%u readClient:%u.\n", 
       //     readVector.size(), readClient.size());
       remakeReadFd();
       break;
    }
    readVector.remove (cindex);
    readClient.remove (cindex);
    remakeReadFd();
    break;
  case SClient::TIMER: 
    cindex=timeoutClient.find (c);
    //fprintf (stderr, "removing timer %u\n", cindex);
    if (cindex<0)
    {
       /* this should not be an error - it removed itself -*/
       fprintf (stderr, "SEventBSD::remove can not find timer.\n");
       break;
    }
    timeoutVector.remove (cindex);
    timeoutClient.remove (cindex);
    break;
  case SClient::JOB: 
    cindex=jobClient.find (c);
    //fprintf (stderr, "removing job %u\n", cindex);
    if (cindex<0)
    {
       /* this should not be an error - it removed itself -*/
       //fprintf (stderr, "SEventBSD::remove can not find job.\n");
       break;
    }
    jobVector.remove (cindex);
    jobClient.remove (cindex);
    break;
  }

  if (cindex < 0) return;


  /* if there are multiple matches, search the one that 
     needs to be removed. */
  SClientVector addbackTarget;
  /** targets **/
  while (targetHashtable.get (c->target))
  {
     SClient* ab = targetHashtable.get (c->target);
     if (ab == c)
     {
       targetHashtable.remove (c->target);
       break;
     }
     addbackTarget.append (ab);
     targetHashtable.remove (c->target);
  }

  unsigned int i;
  /* adding back stuff. */
  for (i=0; i<addbackTarget.size(); i++)
  {
    targetHashtable.put (addbackTarget[i]->target, 
              addbackTarget[i], false);
  }

  /** sources **/
  SClientVector addbackSource;
  while (sourceHashtable.get (c->source))
  {
    SClient* ab = sourceHashtable.get (c->source);
    if (ab == c)
    {
      sourceHashtable.remove (c->source);
      break;
    }
    addbackSource.append (ab);
    sourceHashtable.remove (ab->source);
  }
  for (i=0; i<addbackSource.size(); i++)
  {
    sourceHashtable.put (addbackSource[i]->source, 
          addbackSource[i], false);
  }

  clientHashtable.remove ((const SString&)c->id);
  delete c;
  return;
}

/**
 * This is to remake the fd's
 */
void
SEventBSD::remakeWriteFd()
{
  FD_ZERO (&((SEventDelegate*)delegate)->writeFD);
  writeMax = 0;
  unsigned int i;
  for (i = 0; i<writeVector.size(); i++)
  {
    int fd = (int)writeVector[i];
#ifdef USE_WINAPI
    if (fd<=0)
#else
    if (fd<0)
#endif
    {
       fprintf (stderr, "SEventBSD::remakeWriteFd: bad file [%d].\n", fd);
    }
    else
    {
      FD_SET (fd, &((SEventDelegate*)delegate)->writeFD);
    }
  }
  if (i>0) writeMax = (int)writeVector[i-1] +1;
}

/**
 * Remaking the read fd's
 */
void
SEventBSD::remakeReadFd()
{
  FD_ZERO (&((SEventDelegate*)delegate)->readFD);
  readMax = 0;
  unsigned int i;
  for (i = 0; i<readVector.size(); i++)
  {
    int fd = (int)readVector[i];
#ifdef USE_WINAPI
    if (fd<=0)
#else
    if (fd<0)
#endif
    {
       fprintf (stderr, "SEventBSD::remakeReadFd: bad file [%d].\n", fd);
    }
    else
    {
      FD_SET (fd, &((SEventDelegate*)delegate)->readFD);
    }
  }
  if (i>0) readMax = (int)readVector[i-1] +1;
}

void
SEventBSD::start()
{
  while (next());
}

void
SEventBSD::exit()
{
  up = false;
}

/**
 * Process a next event. Return false if exit was called.
 */
bool
SEventBSD::next()
{
  if (!up)
  {
#ifndef USE_WINAPI
    fprintf (stderr, "SEventBSD::next - down...\n");
#endif
    return false;
  }
#ifdef USE_WINAPI
  int dummy;
  dummy = initsockets (true);
  if (dummy < 0)
  {
    fprintf (stderr, "SEventBSD::next - no socket...\n");
    return false;
  }
#endif
  unsigned int i;
  struct timeval timeout;
  struct timeval timenow;
  struct timeval* tp= 0;
//fprintf (stderr, "readvector=%u writeVector=%u timeoutVector=%u jobVector=%u\n",
 //readVector.size(), writeVector.size(), timeoutVector.size(), jobVector.size());
  if (readVector.size() == 0 && writeVector.size()==0
    && timeoutVector.size() == 0 && jobVector.size()==0)
  {
    fprintf (stderr, "SEventBSD::next - nothing to do\n");
    return false;
  }

  int more = true;

  // Get the jobs done first
  if (jobClient.size() != 0)
  {
    // This is no rocket science here... Just a job
    while (more && jobVector.size())
    {
      more = false;
      roundRobin++;
      unsigned int  k = roundRobin % jobVector.size();
      SClient* c = jobClient[k];

      SJob *j= (SJob*)c->source;
      SEventTarget *t=c->target;

      remove (c);
      int status = j->run();
      t->done(j);
      if (status>0) more = true;
    }
  }
  /* jobs can affect up flag */
  if (!up)
  {
#ifndef USE_WINAPI
    fprintf (stderr, "SEventBSD::next - down...\n");
#endif
    return false;
  }
  
  // Create a timeout
  if (timeoutVector.size() != 0)
  {
    gettimeofday (&timenow, 0);

    // If time is set back 
    if (timenow.tv_sec < ((SEventDelegate*)delegate)->baseTime.tv_sec)
    {
      ((SEventDelegate*)delegate)->baseTime.tv_sec = timenow.tv_sec;
      ((SEventDelegate*)delegate)->baseTime.tv_usec = timenow.tv_usec;
    }
    long diff = timeoutVector[0] - substract (timenow, ((SEventDelegate*)delegate)->baseTime);
    unsigned long next = (unsigned long) diff;
    if (diff < 0)
    {
       next = 0;
    }
    timeout.tv_sec = next/1000;
    timeout.tv_usec = (next%1000) * 1000;
    tp = &timeout;
  }

  fd_set ro;
  fd_set wo;

  memcpy (&ro, &((SEventDelegate*)delegate)->readFD, sizeof (fd_set)); 
  memcpy (&wo, &((SEventDelegate*)delegate)->writeFD, sizeof (fd_set)); 

//Does not always get the SIGPIPE on Linux Alpha
//fprintf (stderr, "Before select %d %d\n", FD_ISSET (5, &wo), FD_ISSET (5, &ro));
  //fprintf (stderr, "s=%u %u %u %u\n", 
  //  readVector.size(), writeVector.size(), timeoutVector.size(),
  //  jobVector.size());

/*
  if (readVector.size() == 0 && writeVector.size() ==0
      && timeoutVector.size() == 0)
  {
    return (jobVector.size() != 0);
  }
*/

  int selected = selectHack (readMax, &ro, writeMax, &wo, 0, 0, tp);
//fprintf (stderr, "After select %d %d\n", FD_ISSET (4, &wo), FD_ISSET (5, &ro));

#ifdef USE_WINAPI
  /* -2 is windows event */
  if (selected == -2)
  {
    return true;
  }
#endif
  if (selected == -1)
  {
#ifdef USE_WINAPI
    int last = WSAGetLastError();
    fprintf (stderr, "SEventBSD:select ERROR %d\n", last);
#endif
    return false;
  }

  /*
   * Timer event
   */
  if (selected == 0)
  {
    if (timeoutClient.size() ==0)
    {
       fprintf (stderr, "unexpected timeout in : SEventBSD::next\n");
    }
    else
    {
       // FIXME: something is wrong with this array.
       // maybe it is the sorted nature.
       SClient* c = timeoutClient[0];
       SEventSource *s=c->source;
       SEventTarget *t=c->target;
       SString id = SString((const SString&)c->id);
       if (clientHashtable.get (id)!=0)
       {
         if (!t->timeout(s))
         {
           if (clientHashtable.get (id)!=0)
           {
             remove (c);
           }
         }
         else
         {
           if (clientHashtable.get (id)!=0)
           {
             remove (c);
             addTimer (s, t);
           }
         }
       }
       else
       {
         fprintf (stderr, "SEventBSD: miracle happened with timer...\n");
         timeoutVector.remove (0);
         timeoutClient.remove (0);
       }
       //return true;
    }
  }

  // See sockets. Let's see reading...
  for (i=0; i<readVector.size(); i++)
  {
   int fd = (int)readVector[i];
   if (FD_ISSET (fd, &ro))
   {
     SClient *c=readClient[i];
     SEventSource *s=c->source;
     SEventTarget *t=c->target;
     if (c->getType()==SClient::SERVER)
     {
        if (!t->serve (s))
        {
           remove (c);
        }
        FD_CLR (fd, &ro);
        break;
     }
     if (t->readable(s)==0)
     {
       FD_CLR (fd, &ro);
       continue;
     }
     char* buffer = new char[4096];
     CHECK_NEW (buffer);
     int progress = 0;
     int n;
#ifdef USE_WINAPI
     n = recv(fd, &buffer[progress], 4096, 0);
#else
     n = ::read(fd, &buffer[progress], 4096);
#endif

     if (n<0 && (errno == EINTR || errno==EAGAIN || errno==EWOULDBLOCK)) n = 0;
     if (n<0)
     {
        remove (c);
        t->error(s);
        delete[] buffer;
        break;
     }
     else if (!sendData(c, buffer, n))
     {
       remove (c);
       delete[] buffer;
       break;
     }
     delete[] buffer;
     FD_CLR (fd, &ro);
   }
  }

  // See sockets. Let's see writing...
  for (i=0; i<writeVector.size(); i++)
  {
   int fd = (int)writeVector[i];
   if (FD_ISSET (fd, &wo)) 
   {
     SClient *c=writeClient[i];
     SEventSource *s=c->source;
     SEventTarget *t=c->target;
     // finally...
     int n;
     sEventBSDSignal = 0;
#ifdef USE_WINAPI
     n = ::send(fd, &c->data.array()[c->progress], c->data.size()-c->progress, 0);
#else
     n = ::write(fd, &c->data.array()[c->progress], c->data.size()-c->progress);
#endif
     if (n<0 && (errno == EINTR || errno==EAGAIN || errno==EWOULDBLOCK)) n = 0;
     if (sEventBSDSignal!=0)
     {
       sEventBSDSignal=0; n=-1;
     }
     if (n<0)
     {
        remove (c);
        t->error(s);
        break;
     }
     else if ((unsigned int) n  == (c->data.size() - c->progress))
     {
       remove (c);
       t->write(s);
       break;
     }
     else
     {
       c->progress += n;
     }
     FD_CLR (fd, &wo);
   }
  }
  return true;
}

/**
 * Send the whole buffer of new lines to client...
 * @return true if client wants our data.
 * if size is 0 then we always return true
 */
bool 
SEventBSD::sendData(SClient* c, char* buffer, int size)
{

  SEventSource* s=c->source;
  SEventTarget* t=c->target;
  SString data (buffer, size);
  return (t->read (s, data));
}

/**
 * This is needed if the app is up for more than a year
 * to recalculate the timeout values in the queue.
 */
void
SEventBSD::seedTimer()
{
  struct timeval  oldTime;
  oldTime.tv_sec = ((SEventDelegate*)delegate)->baseTime.tv_sec;
  oldTime.tv_usec = ((SEventDelegate*)delegate)->baseTime.tv_usec;
  gettimeofday (&((SEventDelegate*)delegate)->baseTime, 0);
  // Base minus old
  //fprintf (stderr, "seed timer\n");
  long diff = substract (((SEventDelegate*)delegate)->baseTime, oldTime);
  for (unsigned int i=0; i<timeoutVector.size(); i++)
  {
      long t = timeoutVector[i];
      timeoutVector.replace (i, t-diff);
  }
}

/**
 * Substrat the time int what from the time in from
 * @return from - what in milliseconds.
 */
static long
substract (const struct timeval& from, const struct timeval& what)
{
  // Clock is moving back.
  if (what.tv_sec > from.tv_sec)
  {
    unsigned long what_sec =  what.tv_sec-1;
    unsigned long what_usec = what.tv_usec + 1000000;
    return -((what_sec - from.tv_sec) * 1000 + (what_usec - from.tv_usec) / 1000);
  }
  // Time is moving forward
  else if (what.tv_sec < from.tv_sec)
  {
    unsigned long from_sec =  from.tv_sec-1;
    unsigned long from_usec = from.tv_usec + 1000000;
    return ((from_sec - what.tv_sec) * 1000 + (from_usec - what.tv_usec) / 1000);
  }
  // sec is same. Time is moving back
  else if (what.tv_usec > from.tv_usec)
  {
    return -((what.tv_usec - from.tv_usec) /1000);
  }
  // Time is moving forward
  return ((from.tv_usec - what.tv_usec) /1000);
}

#ifdef USE_WINAPI

int (*_windowsSelectHookup)(int readSize, fd_set *ro, 
  int writeSize, fd_set *wo, int exceptSize, fd_set *ex,
  struct timeval* t) = 0;
#endif

/**
 * This is a hack to make select work on windows too by  Gaspar 
 */
int
selectHack (int readSize, fd_set *ro, int writeSize, fd_set *wo,
  int exceptSize, fd_set *ex, struct timeval* t)
{
  int maxFd = (readSize > writeSize) ? readSize : writeSize;
  maxFd = (exceptSize > maxFd) ? exceptSize : maxFd;

#ifdef USE_WINAPI
  if (_windowsSelectHookup)
  {
    return _windowsSelectHookup(readSize, ro, writeSize, wo, exceptSize, ex, t);
  }
  // Huh, good eh?
  if (t!=0 && t->tv_sec==0 && t->tv_usec==0) t=0;
  fd_set* my_ro = (readSize==0) ? 0 : ro;
  if (readSize == 0 && ro != 0) FD_ZERO (ro);
  fd_set* my_wo = (writeSize==0) ? 0 : wo;
  if (writeSize == 0 && wo != 0) FD_ZERO (wo);
  fd_set* my_ex = (exceptSize==0) ? 0 : ex;
  if (exceptSize == 0 && ex != 0) FD_ZERO (ex);
  if (my_ro==0 && my_wo ==0)
  {
    if (t==0) return 0;
    DWORD millisec = (DWORD) (t->tv_sec * 1000 + t->tv_usec / 1000);
    // Probably this is the only case sensitive in whole windows.
    Sleep (millisec);
    return 0;
  }
  return select (maxFd, ro, wo, ex, t);
#else
  // Works as it should on unix
  return select (maxFd, ro, wo, ex, t);
#endif
}

/**
 * This makes socket non-blocking. Really...
 */
int
nonblockHack (long id)
{
#ifdef USE_WINAPI
  u_long arg=1;
  return ioctlsocket ((int)id, FIONBIO, &arg);
#else
  int opts = fcntl((int)id,F_GETFL);
  if (opts < 0)
  {
    //fprintf (stderr, "SEventBSD:nonblockHack[%d] %s\n", errno, strerror (errno));
    return -1;
  }
  opts = (opts | O_NONBLOCK);
  if (fcntl((int)id,F_SETFL,opts) < 0)
 {
    //fprintf (stderr, "SEventBSD:nonblockHack[%d] %s\n", errno, strerror (errno));
    return -1;
 }
 return 0;
  
/*
  int ret;
  int arg = 1;
  ret = ioctl ((int)id, FIONBIO, (char*) &arg);
  if (ret!=0)
  {
    fprintf (stderr, "SEventBSD:nonblockHack[%d] %s\n", errno, strerror(errno));
  }
  return ret;
*/
#endif
}

#ifdef USE_WINAPI
/**
 * Poor man's gettime by Gaspar. Timezone is gmt. 
 * @param tz is grossly ignored.
 * @raram  tv is the returned timeval.
 * @return 0
 */
int
gettimeofday (struct timeval* tv, void* tz)
{
  time_t now;
  time (&now);
  time_t onetick = (time_t) (1.0/difftime (now+1, now));
  tv->tv_sec = now/onetick;
  tv->tv_usec = now%onetick;
  return 0;
}

/**
 * Try to initialize stuping winsock.
 * return 0 on fail.
 */
int
initsockets(bool init)
{
  static int state = 0;
  if (!init)
  {
    if (state = 1)
    {
       closesocket (state);
       WSACleanup();
       wsaData = 0;
       return 1;
    }
    return 1;
  }
  if (state == 0)
  {
    WORD version = MAKEWORD (1,1);
    if (WSAStartup (version, &wsaDataStruct)!=0)
    {
       fprintf (stderr, "SEventBSD.cpp:initsockets can not initialize[%d].\n",
         WSAGetLastError());
       state = -1;
    }
    else
    {
       state=socket (AF_INET, SOCK_STREAM, IPPROTO_IP);
       if (state==0)
       {
          fprintf (stderr, "SEventBSD.cpp: failed to open socket[%d].%s\n", errno, strerror (errno));
          state = -1;
       }
       else
       {
          u_long arg=1;
          //ioctlsocket (state, FIONBIO, &arg);
          wsaData = &wsaDataStruct;
          state = 1;
      }
    }
  }
  return state;
}

#endif

#ifdef USE_WINAPI
static void
initSignals(bool init)
{
}
#else

extern "C"
{
  typedef void(*signalHandler_t)(int);
  static  void signalHandler (int signum);
};

static void
initSignals(bool init)
{
  static bool inited = false;
  if (inited && init) return;
  static struct sigaction   oact;
  struct sigaction          act;

  if (init)
  {
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    /* Hack: on some systems signalHandler does not take an int argument */
    *((signalHandler_t*) &act.sa_handler) = (signalHandler_t) signalHandler;
#ifdef SA_INTERRUPT
    act.sa_flags |= SA_INTERRUPT;
#endif
    sigaction (SIGALRM, &act, &oact);

    act.sa_handler = SIG_IGN;
    sigaction (SIGPIPE, &act, &oact);
    inited = true;
  }
  else
  {
    sigaction (SIGPIPE, &oact, &act);
    sigaction (SIGALRM, &oact, &act);
    inited = false;
  }
}

/**
 * Be aware: on some systems signalHandler does not take an int argument
 */
extern "C" {
  static  void
  signalHandler (int signum)
  {
  #if 0
      switch (signum)
      {
      case SIGPIPE:
          fprintf (stderr, "SEventBSD: disconnected - SIGPIPE\n");
          break;

      case SIGALRM:
          fprintf (stderr, "SEventBSD: timeout - SIGALRM\n");
          break;
  
      default:
          fprintf (stderr, "SEventBSD: signal %d\n", signum);
      }
      sEventBSDSignal = signum;
  #endif
     sEventBSDSignal = 1;
     return;
  }
}

#endif

