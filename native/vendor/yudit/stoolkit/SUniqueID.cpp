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
 
#include "stoolkit/SUniqueID.h"
#include "stoolkit/STypes.h" 
#include "time.h"
#include "string.h"

static unsigned int oldSequence = 0;

SUniqueID::SUniqueID(void)
{
   time_t t = time (0);
   timePart = (unsigned int) t;
   sequence = oldSequence++;
   char a[64];
   snprintf (a, 64, "%u.%u", timePart, sequence);
   append (a, strlen (a));
}

SUniqueID::~SUniqueID ()
{
}
