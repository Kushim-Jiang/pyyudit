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
 
#ifndef SExcept_h
#define SExcept_h


class SExcept
{
public:
  SExcept (const char* mes);
  SExcept (const SExcept& e);
  ~SExcept ();
  SExcept& operator= (const SExcept& e);
  const char* toString() const;
private:
  char* message;
};

#define CHECK_NEW(_a) if(_a==0) throw SExcept("Not Enough Memory")

#endif /* SExcept_h */
