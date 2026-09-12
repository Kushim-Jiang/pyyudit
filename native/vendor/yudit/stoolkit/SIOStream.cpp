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
 
#include "SIOStream.h"
#include <stdio.h>
#ifdef USE_WINAPI
#include <io.h>
#include <windows.h>
#endif

SReader::SReader (const SInputStream& i, SEventTarget* t) : in (i)
{
  errFlag = false;
  finishFlag = false;
  more = true;
  SEventHandler::addInput (&in, this);
  trg = t;
  if (trg) SEventHandler::addJob (this, trg);
}

SReader::SReader (const SInputStream& i, const SStringVector& sep, SEventTarget* t) : sepVector (sep), in(i)
{
  errFlag = false;
  finishFlag = false;
  more = true;
  trg = t;
  SEventHandler::addInput (&in, this);
  if (trg) SEventHandler::addJob (this, trg);
}

bool
SReader::isOK()
{
  return !errFlag;
}

bool
SReader::close()
{
  return in.close();
}

SReader::~SReader ()
{
}

bool
SReader::read (SString* s)
{
 // This is how we read things unblocked.
#ifdef USE_WINAPI
  if (in.getType()==SInputStream::PIPE)
  {
    DWORD n;
    char* buff = new char[4096];
    CHECK_NEW (buff);
    do
    {
      if (!ReadFile ((char*)in.getId(), buff, 4096, &n, 0) ||  n<=0)
      {
        break;
        errFlag=true;
      }
      SString ns(buff, n);
      read (&in, ns);
    } while (n>0);
    delete buff;
    read (&in, SStringNull);
  }
  if (in.getType()==SInputStream::FILE)
  {
    int n;
    char* buff = new char[4096];
    CHECK_NEW (buff);
    
    do
    {
      n  = ::read (in.getId(), buff, 4096); 
      SString ns(buff, n);
      read (&in, ns);
    } while (n>0);
    delete buff;
    read (&in, SStringNull);
  }
#endif
 if (trg==0)
 {
   bool n = true;
   while (n==true && finishFlag==false)
   {
     n = SEventHandler::next();
   }
 }
 if (errFlag || !finishFlag) return false;

 if (sepVector.size() != 0)
 {
   if (line.size() ==0)
   {
     if (!more)
     {
        *s = buffer;
        buffer.clear();
     }
     else
     {
       *s = SStringNull;
     }
   }
   else
   {
     *s = line[0];
     line.remove(0);
   }
   if (more && line.size() == 0)
   {
      if (in.getId() <0)
      {
        finishFlag=true; 
        errFlag=true; 
        more = false;
        return false;
      }
      SEventHandler::addInput (&in, this);
      if (trg) SEventHandler::addJob (this, trg);
      finishFlag = 0;
   }
   return true;
 }
 *s = buffer;
 buffer.clear();
 return true;
}

int
SReader::run ()
{
  if (finishFlag) return -1;
  return 0;
}

/*a*
 * This is for event target if t == 0
 */
bool
SReader::read (const SEventSource* s, const SString& m)
{
  buffer.append (m);
  // End of stream...
  if (m.size() == 0)
  {
    finishFlag = true;
    more = false;
    return false;
  }
  if (sepVector.size() != 0) 
  {
     while (true)
     {
        int found =-1;
        unsigned int i;
        for (i=0; i<sepVector.size(); i++) 
        {
           found = buffer.find (sepVector[i]);
           if (found>=0) break;
        }
        if (found < 0) break;
        unsigned int ind = found+sepVector[i].size();
        SString a(buffer.array(), ind);
        line.append (a);
        buffer.remove (0, ind);
     }
     // Job is done!
     if (line.size())
     {
        finishFlag = true;
        return false;
     }
  }
  return true;
}

/*a*
 * This is for event target fi t == 0
 */
void
SReader::error (const SEventSource* s)
{
  errFlag = true;
}

SWriter::SWriter (const SOutputStream& i, SEventTarget* t) : out(i)
{
  errFlag = false;
  finishFlag = true;
  writeCount = 0;
  trg = t;
  if (t)
  {
    SEventHandler::addJob (this, t);
  }
}

SWriter::~SWriter ()
{
}

bool
SWriter::write (const SString& s)
{
  if (errFlag) return false;
#ifdef USE_WINAPI
  if (out.getType()==SOutputStream::PIPE)
  {
    DWORD n;
    SString rem=s;
    while (rem.size()) {
      if (rem.size()==0) break;
      if (!WriteFile ((char*)out.getId(), rem.array(), rem.size(), &n, 0) 
          || n <= 0)
      {
        errFlag=true;
        break;
      }
      SString ns(rem.array(), n, rem.size()-n);
      rem = ns;
    }
    finishFlag = true;
    if (n<0) errFlag = true;
    return !errFlag;
  }
  else if (out.getType()==SOutputStream::FILE)
  {
    int n;
    SString rem=s;
    while (rem.size()) {
      if (rem.size()==0) break;
      n  = ::write ((int)out.getId(), rem.array(), rem.size()); 
      if (n<0) break;
      SString ns(rem.array(), n, rem.size()-n);
      rem = ns;
    }
    finishFlag = true;
    if (n<0) errFlag = true;
    return !errFlag;
  }
#endif
  if (finishFlag)
  {
    finishFlag = false;
    if (trg) SEventHandler::addJob (this, trg);
  }
  if (out.getId() <0)
  {
     finishFlag=true; 
     errFlag=true; 
     return false;
  }
  SEventHandler::addOutput (&out, this, s);
  writeCount++;
  finishFlag = false;
  // SGC finish
  if (trg) {
    return true;
  }
  if (trg==0)
  {
    bool n = true;
    do
    {
     n = SEventHandler::next();
     if (out.getId() <0)
     {
         finishFlag=true; 
         errFlag=true; 
         return false;
     }
    } while (n==true && finishFlag==false);
  }
  return !errFlag;
}

/**
 * This is as a job. Eent manager use 
 */
int
SWriter::run ()
{
  if (finishFlag) return -1;
  return 0;
}

/**
 * This is for event target fi t == 0
 */
bool
SWriter::write (const SEventSource* s)
{
  writeCount--;
  if (writeCount == 0) finishFlag = true;
  if (finishFlag) return false;
  return true;
}

/*a*
 * This is for event target fi t == 0
 */
void
SWriter::error (const SEventSource* s)
{
  errFlag = true; finishFlag = true;
}

bool
SWriter::isOK()
{
  return !errFlag;
}

bool
SWriter::close()
{
  return out.close();
}
