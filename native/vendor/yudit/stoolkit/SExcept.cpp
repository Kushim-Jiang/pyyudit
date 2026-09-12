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
 
#include "SExcept.h"
#include <string.h>

SExcept::SExcept (const char* mes)
{
  if (mes != 0 && message != mes)
  {
    message = new char[strlen(mes) + 1];
    if (message) strcpy (message, mes);
  }
}

SExcept::SExcept (const SExcept& e)
{
  if (e.message != 0 && &e != this)
  {
    message = new char[strlen(e.message) + 1];
    if (message) strcpy (message, e.message);
  }
}

SExcept::~SExcept ()
{
 if (message) delete message;
}

SExcept&
SExcept::operator= (const SExcept& e)
{
  if (e.message != 0 && &e != this)
  {
    message = new char[strlen(e.message) + 1];
    if (message) strcpy (message, e.message);
  }
  return *this;
}

const char*
SExcept::toString() const
{
  if (message) return message;
  return "Out Of Memory";
}
