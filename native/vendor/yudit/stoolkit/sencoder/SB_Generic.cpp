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
 
#include "stoolkit/sencoder/SB_Generic.h"
#include "stoolkit/SString.h"
#include "stoolkit/SStringVector.h"
#include "stoolkit/SEncoder.h"
#include "stoolkit/SCluster.h"

/**
 * This is a sample (base) implementation of the core encoding class
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-05-12
 * E2 80 A8 E2 80 A9 are paragraph and line seps in utf-8 (U+20A8, U+20A9) 
 */
SB_Generic::SB_Generic(const SString& n) : SBEncoder ("\n,\r\n,\r"), map (n)
{
  ok = map.isOK();
  clustered = map.isClustered();
}

SB_Generic::~SB_Generic ()
{
}

/**
 * return false if this generic encoder does not exist.
 */
bool
SB_Generic::isOK() const
{
  return ok;
}

/**
 * This is encoding a unicode string into a bytestring
 * @param input is a unicode string.
 */
const SString&
SB_Generic::encode (const SV_UCS4& input)
{
  return map.encode(input);
}

/**
 * Decode an input string into a unicode string.
 * @param input is a string.
 *   he output can be null, in this case a line is not
 *   read fully. If input size is zero output will be flushed.
 */
const SV_UCS4&
SB_Generic::decode (const SString& input)
{
  /* if clustered, we let output out only if a cluster is finished */
  if (!clustered) return map.decode(input);

  ucs4string.clear();
  const SV_UCS4& ret = map.decode(input); 
  if (ret.size()) remaining.append (ret);

  /* clusterize it */
  bool zeorinput = (input.size()==0);
  while(remaining.size())
  {
    int finished; SV_UCS4  out;
    unsigned int next = getCluster (remaining, 0, &out, &finished);
    /* it *is* finished */
    if (finished==0 && zeorinput) finished = 1;
    if (!finished) break;
    if (next==0) next = 1;
    while (next)
    {
      ucs4string.append (remaining[0]);
      remaining.remove(0);
      next--;
    }
  }
  if (zeorinput) map.reset(false);
  return (ucs4string);
}

/**
 * These methods guess the line delimiters for the input
 * The one without arguments is giving the 'first approximation'
 * It returns an inclusive list of all possibilities.
 */
const SStringVector&
SB_Generic::delimiters ()
{
  return realDelimiters;
}

/**
 * These methods guess the line delimiters for the input
 * The one without arguments is giving the 'first approximation'
 * It returns an exact list
 */
const SStringVector&
SB_Generic::delimiters (const SString& sample)
{
  return sampleDelimiters;
}

#if 0
/**
 * @return the text in the decode buffer
 */
SString
SB_Generic::remainder() const
{ 
  if (!clustered)
  {
    return SString(map.remainder());
  }
  /* Make a string from UCS4 remainder, by appliying reverse map. */
  SUniMap m = map;
  SString ret = m.encode (remaining);
  SV_UCS4 empty;
  ret.append (m.encode(empty));
  SString rem = map.remainder();
  //fprintf (stderr, "rem.size%u ucs4.size=%u string.size=%u\n", 
  //    remaining.size(), ret.size(), rem.size());
  ret.append (rem);
  return SString(ret);
}
#endif

/* for non-clustering it is remainder */
SString
SB_Generic::preEditBuffer() const
{
  if (!clustered)
  {
    return SString(map.remainder());
  }
  return SString(map.remainder()); // for a change :)
}

/* for clustering */
SV_UCS4
SB_Generic::postEditBuffer () const
{
  if (!clustered)
  {
     return SV_UCS4();
  }
  return SV_UCS4(remaining);
}

/**
 * return key value map to see what decodes to what
 * @param key will contain the keys
 * @param value will contain the values
 * @param _size is the maximum size of returned arrays
 * @return the real size of the arrays.
 */
unsigned int
SB_Generic::getDecoderMap (SStringVector* key, SStringVector* value,
        unsigned int _size)
{
  key->clear();
  value->clear();
  if (!map.isOK()) return 0;
  return (map.getDecoderMap (key, value, _size));
}

/**
 * Clear the map
 */
void
SB_Generic::clear()
{
  remaining.clear();
  map.reset();
}
