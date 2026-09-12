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

#ifndef SEmbedState_h
#define SEmbedState_h

#include "stoolkit/STypes.h"
#include "stoolkit/SString.h"
#include <string.h>

#define SD_BIDI_MAX_EMBED 61
#define SD_BIDI_MAX32 (SD_BIDI_MAX_EMBED/8+1) 

class SEmbedState
{
public:
  inline SEmbedState(void);
  inline SEmbedState operator=(const SEmbedState& es);
  inline SEmbedState(const SEmbedState& es);
  bool operator == (const SEmbedState& es) const;
  bool operator != (const SEmbedState& es) const;

  void setEmbeddingMarks (const SV_UCS4& stack);
  SV_UCS4 getEmbeddingMarks (const SEmbedState* from) const;

  inline unsigned int getExplicitLevel() const;
  inline bool isOverride() const;
  SString toString() const;

private:
  unsigned int getDepth () const;
  inline void setExplicitLevel (unsigned int _exlevel, bool _override);

  void setMark (unsigned int level, unsigned int _mark);
  unsigned int getMark(unsigned int level) const;

  SS_UCS4 states[SD_BIDI_MAX32];
  char    explevel;    
};

SEmbedState::SEmbedState(void)
{
  memset(states, 0, sizeof (states));
  explevel=0;
}

SEmbedState::SEmbedState(const SEmbedState& es)
{
  memcpy (states, es.states, sizeof (states));
  explevel = es.explevel;
}

SEmbedState
SEmbedState::operator=(const SEmbedState& es)
{
  memcpy (states, es.states, sizeof (states));
  explevel = es.explevel;
  return *this;
}

bool
SEmbedState::isOverride() const
{
  return (explevel & 0x40);
}

unsigned int
SEmbedState::getExplicitLevel() const
{
  return (unsigned int) (explevel & 0x3f);
}

void
SEmbedState::setExplicitLevel (unsigned int _exlevel, bool _override)
{
  explevel = ((_exlevel & 0x3f) | (_override?0x40:0));
}

#endif /* SEmbedState_h */
