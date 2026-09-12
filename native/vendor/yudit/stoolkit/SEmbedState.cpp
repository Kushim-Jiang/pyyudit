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

#include "stoolkit/SEmbedState.h"
#include "stoolkit/SCharClass.h"
#include "stoolkit/STypes.h" 

static SS_UCS4 stateMap[4] = 
{
   SD_CD_LRO, SD_CD_LRE,
   SD_CD_RLO, SD_CD_RLE,
};

/**
 * This is a state class that stores the embedding history.
 * Currently it supports 32+32/4 = 16 embedding levels.
 * To support more you need to add exstatexx.
 */


/**
 * Check if state is equal
 */
bool
SEmbedState::operator==(const SEmbedState& es) const
{
  return (explevel == es.explevel 
      && memcmp (states, es.states, sizeof (states)) == 0);
}

/**
 * Check if state is not equal
 */
bool
SEmbedState::operator!=(const SEmbedState& es) const
{
  return (explevel != es.explevel 
      || memcmp (states, es.states, sizeof (states)) != 0);
}

/**
 * exstate0 and exstate1 are operated with this.
 @ @param level is the current (real) level
 * @return an index into stateMap
 */
unsigned int
SEmbedState::getMark(unsigned int level) const
{
  unsigned int line = level/8;
  unsigned int index = (level%8)*4;
  return ((states[line] >> index) & 0x3);
}

/**
 * exstate0 and exstate1 are operated with this.
 @ @param level is the current (real) level
 * @mark is an index into stateMap
 */
void
SEmbedState::setMark (unsigned int level, unsigned int _mark)
{
  unsigned int line = level/8;
  unsigned int index = (level%8)*4;
  unsigned int mask = ((0x3 << index) ^ 0xffffffff);
  states[line] = states[line] & mask;
  states[line] = states[line] | (_mark << index);
}

/* Return embedding marks */
SV_UCS4
SEmbedState::getEmbeddingMarks (const SEmbedState* from) const
{
  SV_UCS4 ret;
  unsigned int depfrom = (from==0) ? 0 : from->getDepth();
  unsigned int depto = getDepth(); 
  unsigned int min = (depfrom < depto) ? depfrom : depto;
  /* skip the common ones */
  unsigned int i = 0;
  for (i=0; i<min; i++)
  {
     if (from->getMark(i)!=getMark(i)) break;
  }
  /* fill in the space with PDF */
  for (unsigned int j=i; j<depfrom; j++)
  {
    ret.append (SD_CD_PDF);
  }
  while (i<depto)
  {
    ret.append (stateMap[getMark(i)]);
    i++;
  }
  return SV_UCS4(ret);
}

/* 
 * Get the *real* embedding depth, not the Unicode algorithm crap:
 * For that you need: getExplicitLevel() 
 */
unsigned int
SEmbedState::getDepth () const
{
  unsigned ret = 0;
  unsigned int to = getExplicitLevel();
  unsigned int mark;
  unsigned int i=0;
  unsigned int emb = 0;
  while ( emb<to )
  {
    ret++;
    mark = getMark(i);
    i++;
    /* inc twice needed for RL and 1 and LR and 0 */
    if (mark>=2 && (emb&1) == 1) emb++;
    if (mark<2 && (emb&1) == 0) emb++;
    emb++;
  }
  return ret;
}

/**
 * Set the overriding state from stack 
 * Stack can contain only glyphs that are in stateMap.
 */
void
SEmbedState::setEmbeddingMarks (const SV_UCS4& stack)
{
  memset (states, 0, sizeof (states));
  explevel = 0;

  /* our max embedding is 16 */
  unsigned int level = 0;
  for (unsigned int i=0; i<stack.size() && i <= SD_BIDI_MAX_EMBED; i++)
  {
    unsigned int mark = 0;
    bool override = false;
    /* find our mark */
    switch (stack[i])
    { 
    /* These are in this order in stateMap */
    case SD_CD_LRO:
      mark = 0;
      override = true;
      break;
    case SD_CD_LRE:
      mark = 1;
      break;
    case SD_CD_RLO:
      mark = 2;
      override = true;
      break;
    case SD_CD_RLE:
      mark = 3;
       break;
    default:
      continue;
    }
    // RLO rnd RLE is at 2 and 3
    if ((mark==2 || mark == 3) && (level&1) == 1)
    {
      if (level+2 > SD_BIDI_MAX_EMBED) break;
      level++;
      level++;
    }
    else if ((mark==0 || mark == 1) && (level&1) == 0)
    {
      if (level+2 > SD_BIDI_MAX_EMBED) break;
      level++; 
      level++;
    }
    else
    {
      if (level+1 > SD_BIDI_MAX_EMBED) break;
      level++;
    }
    /* RL is 2 and 3 */
    setExplicitLevel (level, override);
    setMark (i, mark);
  }
  return;
}
/**
 * Used for debug.
 */
SString
SEmbedState::toString() const
{
  SString ret;
  ret.append ("[");
  char s[12];
  SV_UCS4 v = getEmbeddingMarks(0);
  const char* c = "";
  for (unsigned int i=0; i<v.size(); i++)
  {
   
   snprintf (s, 12, "%s%04X", c, (unsigned int)v[i]); 
   c = " ";
   ret.append (s);
  }
  ret.append ("]");
  return SString(ret);
}
