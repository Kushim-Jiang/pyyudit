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
 
#include "stoolkit/SUniMap.h"
#include "stoolkit/SBMap.h"
#include "stoolkit/SBinHashtable.h"
#include "stoolkit/SIO.h"
#include "stoolkit/SUtil.h"
#include "stoolkit/STypes.h" 
#include <stdio.h>

static SS_UCS4 getMaxBytes (unsigned int inp);

/**
 * A unicode Map. It maps SV_UCS2 to SV_UCS4 or SString<->SV_UCS4
 * It is generally useful when mapping UCS4 to Font encodings and
 * Keyboard trasliterations to unicode strings. 
 *
 * This is a tamed version of SBMap.
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-05-12
 */

class SUniMapDelegate
{
public:
  SUniMapDelegate(const SString& name, SBMap* _map);
  ~SUniMapDelegate();
  /* these routines supposed to speed it up */
  SString name;
  int    count;
  SBMap* map;
};


/**
 * We use the delegate to hide the ugly SBMap and
 * maintain one reference in a cache
 */
SUniMapDelegate::SUniMapDelegate(const SString& n, SBMap* _map)
{
  name = n;
  map = _map;
  count = 1;
}
SUniMapDelegate::~SUniMapDelegate()
{
  delete map;
}

typedef SBinHashtable<SUniMapDelegate*> SUniMapHashtable;
/*
 * This canche is consulted when a new map is requested.
 */
static SUniMapHashtable mapCache;

static SStringVector searchPath;
/*
("/,maps,../maps,/etc/maps,../mytool/uni,../mytool/kmap,../mytool/mys,mytool/uni,mytool/kmap,mytool/mys,uni,kmap,mys");
*/
static void _uniAppend (SString* s, SS_UCS4 u);

void
SUniMap::setPath (const SStringVector& l)
{
  searchPath = l;
}

/**
 * search files for property in order and set the path to the 
 * property. Always add YUDIT_DATA/data
 */
void
SUniMap::guessPath ()
{
  SStringVector unipath = getUserPath("yudit.datapath", "data");
//  fprintf (stderr, "uni is %*.*s\n", SSARGS(unipath.join(",")));
  setPath (unipath);
}

const SStringVector&
SUniMap::getPath ()
{
  return searchPath;
}


/**
 * Vector available maps
 */
SStringVector 
SUniMap::list()
{
  SStringVector ret;
  for (unsigned int i=0; i<searchPath.size(); i++)
  {
    SDir d(searchPath[i]);
    SStringVector l = d.list("*.my");
    for (unsigned int j=0; j<l.size(); j++)
    {
      SString s = l[j];
      s.truncate(s.size()-3);
      ret.append (s);
    }
  }
  return SStringVector (ret);
}

/**
 * This is a straight map.
 */
SUniMap::SUniMap (void)
{
  delegate = 0;
  dmodel = 0;
  emodel = 0;
  dmodel4 = 0;
  emodel4 = 0;
  eindex = -1;
  dindex = -1;
  bumap = 0;
  ok = true;
}

/**
 * Try to load the delegate with the name.
 * The suffix ".my" will be added to the name here in this routine.
 */
SUniMap::SUniMap (const SString& name)
{
  bumap = 0;
  load (name);
}

/**
 * Try to load the map. The name is the name without file extension .my
 */
void
SUniMap::load (const SString& name)
{
  delegate = 0;
  dmodel = 0;
  emodel = 0;
  dmodel4 = 0;
  emodel4 = 0;
  eindex = -1;
  dindex = -1;
  ok = false;

  if (mapCache.get (name))
  {
    SUniMapDelegate* d = mapCache[name];
    d->count++;
    delegate = d;
    ok = true;
    setModel(1, 2, 2, 1);
    return;
  }
  SString n (name);
  n.append (".my");
  SFile f(n, searchPath);
  if (f.size()  <= 0) return ;
  SFileImage i = f.getFileImage();
  if ( i.size() <= 0) return;
  SBMap* b = new SBMap();
  CHECK_NEW (b);
  if (!b->setFileImage(i))
  {
    delete b;
    return;
  }
  SUniMapDelegate* ud = new SUniMapDelegate(name, b);
  CHECK_NEW (ud);
  delegate = ud;
  mapCache.put (name, ud);
  setModel(1, 2, 2, 1);
  bumap = (ud->map->isUMap()) ? ud->map : 0;
  ok = true;
}

/**
 * Allocates new models
 * @param din is the decode input size 0=SS_WORD8 1=SS_WORD16 3=SS_WORD32
 * @param dout is the decode output size
 * @param ein is the encode input size
 * @param eout is the encode output size
 */
void
SUniMap::setModel(int din, int dout, int ein, int eout)
{
  SUniMapDelegate* d = (SUniMapDelegate*) delegate;
  SBMap *map = d->map;

  eindex = indexOf(true);
  dindex = indexOf(false);

  emodel = 0; dmodel = 0;
  if (eindex>=0)
  {
      /* From 4 byte  to 2 byte */
      emodel = new SStateModel (ein, map->getInWordSize (eindex),
            map->getOutWordSize (eindex), eout);
      emodel4 = new SStateModel (2, map->getInWordSize (eindex),
            map->getOutWordSize (eindex), 2);
  }

  if (dindex>=0)
  {
      /* From 2byte  to 4 byte */
      dmodel = new SStateModel (din, map->getInWordSize (dindex),
            map->getOutWordSize (dindex), dout);
      dmodel4 = new SStateModel (2, map->getInWordSize (dindex),
            map->getOutWordSize (dindex), 2);
  } 
}

/**
 * Copy an existing map
 * @param m is an existing map
 */
SUniMap::SUniMap (const SUniMap &m)
{
  dmodel = 0;
  emodel = 0;
  dmodel4 = 0;
  emodel4 = 0;
  delegate = 0;
  dindex = -1;
  eindex = -1;
  bumap = m.bumap;
  if (!m.isOK())
  {
    ok = false;
  }
  else
  {
    SUniMapDelegate* d = (SUniMapDelegate*) m.delegate;
    if (d)
    {
      d->count++;
      delegate = d;
      dindex = m.dindex;
      eindex = m.eindex;
      setModel(1, 2, 2, 1);
    }
    ok = true;
  }
}

/**
 * Assign a map
 */
SUniMap&
SUniMap::operator = (const SUniMap &m)
{
  if (&m == this) return *this;
  derefer();
  dmodel = 0;
  emodel = 0;
  dmodel4 = 0;
  emodel4 = 0;
  delegate = 0;
  dindex = -1;
  eindex = -1;
  bumap = m.bumap;
  reset();
  if (!m.isOK())
  {
    ok = false;
  }
  else
  {
    SUniMapDelegate* d = (SUniMapDelegate*) m.delegate;
    if (d)
    {
      d->count++;
      delegate = d;
      dindex = m.dindex;
      eindex = m.eindex;
      setModel(1, 2, 2, 1);
    }
    ok = true;
  }
  return *this;
}

SUniMap::~SUniMap()
{
  if (delegate !=0) derefer();
}

/**
 * dereference the current map
 */
void
SUniMap::derefer()
{
  if (isOK())
  {
    SUniMapDelegate* d = (SUniMapDelegate*) delegate;
    if (d!=0)
    {
      d->count--;
      if (d->count==0)
      { 
        // FIXME: if I delete it it gets deleted twice !
        //mapCache.remove (d->name);
        //fprintf (stderr, "FIXME: STRANGE DELETE %*.*s\n", SSARGS(d->name));
        //delete d;
      }
      if (dmodel) delete ((SStateModel*) dmodel);
      if (emodel) delete ((SStateModel*) emodel);
      if (dmodel4) delete ((SStateModel*) dmodel4);
      if (emodel4) delete ((SStateModel*) emodel4);
    }
  }
}

/**
 * A simplistic encoder. 
 * @return the UCS2 value of the first decoded value or zero
 */
SS_UCS2
SUniMap::encode (SS_UCS4 in)
{
  if (bumap)
  {
    if (in > 0xffff) return 0;
    return ((SBMap*) bumap)->encode (in);
  }
  if (!ok) return 0;
  if (in > 0xffff) return 0;
  if (delegate==0) return (SS_UCS2) in;
  SUniMapDelegate* d = (SUniMapDelegate*) delegate;
  SBMap *map = d->map;

  SStateModel* enc = (SStateModel*) emodel;
  if (enc == 0) return 0;
  SS_UCS2 maxBytes = getMaxBytes (map->getInWordSize (eindex));
  if (in > maxBytes || maxBytes==0) return 0;

  int len = map->encode (eindex, &in, 1, enc, 0);
  if (len < 0)
  {
    enc->reset(); return 0;
  }
  if (enc->out.length != 1)
  {
    enc->reset(); return 0;
  }
  SS_UCS2 retVle = enc->out.u.u16[0];
  enc->reset();
  return retVle;
}

/**
 * A simplistic decoder. 
 * @return the UCS4 value of the first decoded value or zero
 */
SS_UCS4
SUniMap::decode (SS_UCS2 in)
{
  if (bumap)
  {
    return ((SBMap*) bumap)->decode (in);
  }
  if (!ok) return 0;
  if (delegate==0) return (SS_UCS4) in;
  SUniMapDelegate* d = (SUniMapDelegate*) delegate;
  SBMap *map = d->map;

  SStateModel* dec = (SStateModel*) dmodel;
  if (dec == 0) return 0;

  SS_UCS4 maxBytes = getMaxBytes (map->getInWordSize (dindex));
  if (in > maxBytes && maxBytes == 0) return 0;
  int len = map->encode (dindex, &in, 1, dec, 0);
  if (len < 0)
  {
    dec->reset(); return 0;
  }
  if (dec->out.length != 1)
  {
    dec->reset(); return 0;
  }
  SS_UCS4 retVle = dec->out.u.u32[0];
  dec->reset();
  return retVle;
}

SObject*
SUniMap::clone() const
{
  SObject* n = new SUniMap(*this);
  CHECK_NEW (n);
  return n;
}

/**
 * @param encode is true if we are looking at the encoding map
 * return 0 if map was designed to accept SS_WORD8
 *        1 if map was designed to accept SS_WORD16
 *        2 if map was designed to accept SS_WORD32
 *        3 if map was designed to accept SS_WORD64
 *        -1 if map is not initialized
 */
int
SUniMap::getInWordSize (bool encode)
{
  if (!ok) return -1;
  if (delegate ==0) return 1;
  int index = indexOf(encode);
  if (index < 0) return -1;
  SUniMapDelegate* d = (SUniMapDelegate*) delegate;
  return d->map->getInWordSize(index);
}

/**
 * @param encode is true if we are looking at the encoding map
 * return 0 if map was designed to produce SS_WORD8
 *        1 if map was designed to produce SS_WORD16
 *        2 if map was designed to produce SS_WORD32
 *        3 if map was designed to produce SS_WORD64
 *        -1 if map is not initialized
 */
int
SUniMap::getOutWordSize (bool encode)
{
  if (!ok) return -1;
  if (delegate ==0) return 1;
  int index = indexOf(encode);
  if (index < 0) return -1;
  SUniMapDelegate* d = (SUniMapDelegate*) delegate;
  return d->map->getOutWordSize(index);
}

/**
 * Get the index in the map
 * @param encode is true if we are looking at the encoding map
 * @return the index in the SBMap or -1
 */
int
SUniMap::indexOf(bool encode) const
{
  SUniMapDelegate* d = (SUniMapDelegate*) delegate;
  if (d==0) return -1;
  SBMap *map = d->map;
  for (int i=0; i<map->getSize(); i++)
  {
    if (map->getType(i)==(encode ? SBMap::SBMap_ENCODE : SBMap::SBMap_DECODE)) 
    {
      return i;
    }
  }
  return -1;
}

/**
 * return true if the map is 'usable'
 * a useless map is a map that could not be found.
 */
bool
SUniMap::isOK() const
{
  return ok;
}

void
SUniMap::reset ()
{
  reset (false);
  reset (true);
}

void
SUniMap::reset(bool en)
{
  if (en)
  {
    ucs4vIn.clear();
    sstringOut.clear();
  }
  else
  {
    sstringIn.clear();
    ucs4vOut.clear();
  }
}

void
SUniMap::undo (bool encode)
{
  if (encode)
  {
    if (ucs4vIn.size() > 0) ucs4vIn.truncate(ucs4vIn.size()-1);
  }
  else
  {
    if (sstringIn.size() > 0) sstringIn.truncate(sstringIn.size()-1);
  }
}

/**
 * Decode the input string
 * if in.size() is zero flush it.
 * It can return a zero sized array, in this case more input is needed.
 */
const SV_UCS4&
SUniMap::decode (const SString& in)
{
  ucs4vOut.clear ();
  sstringIn.append(in);
  unsigned int i;
  if (dmodel==0)
  {
    for (i=0; i<sstringIn.size(); i++)
    {
      ucs4vOut.append (sstringIn[i]);
    }
    sstringIn.clear();
    return ucs4vOut;
  }
  SV_UCS2 buffer;
  for (i=0; i<sstringIn.size(); i++)
  {
    buffer.append (SS_UCS2((unsigned char)sstringIn[i]));
  }
  SStateModel* model = (SStateModel*) dmodel;
  model->reset();
  SUniMapDelegate* delg = (SUniMapDelegate*) delegate;
  SBMap* map = (delg==0 || model==0) ? 0 : delg->map;

  // Now go in circles and append till matches.
  int st;
  unsigned int fullsize = buffer.size();
  i=0;
  while (i<fullsize)
  {
    if (map ==0)
    {
      st = SS_REJECT; 
    }
    else
    {
      model->reset ();
      int more = (in.size()>0)?1:0;
      /* imposing a limit of 10 because map->circle sucks. */
      /* FIXME we really need a better map and encoding  */
      unsigned int bsz = ((int)(buffer.size()-i) > 10) 
          ? 10 : buffer.size()-i; 
      st = map->circle (SBMap::SBMap_DECODE, &buffer.array()[i],
          (int)(bsz), model, more);
    }

    if (st == SS_ACCEPT)
    {
       model->reset ();
       int more = (in.size()>0)?1:0;
       st = map->circle (SBMap::SBMap_DECODE, &buffer.array()[i],
          (int)(buffer.size()-i), model, more);
       if (st == SS_ACCEPT)
       {
         break;
       }
    }

    // Append stuff as it is and crunch buffer.
    if (st == SS_REJECT)
    {
      ucs4vOut.append ((SS_UCS4)buffer[i]);
      i++;
    }
    // There is a match here. append the output and
    // Shift the input.
    else if (st>0)
    {
      ucs4vOut.append ((SS_UCS4*)model->out.u.u32, model->out.length);
      while (st--)
      {
        i++;
      }
    }
    // Provision for the bad maps.
    else 
    {
      //fprintf (stderr, "SUniMap:: decode bad map zero matched!\n");
      ucs4vOut.append ((SS_UCS4)buffer[i]);
      i++;
    }
  }
  SString vrest;
  while (i<fullsize)
  {
    vrest.append (sstringIn[i]);
    i++;
  }
  sstringIn = vrest;
  return ucs4vOut;
}

const SString&
SUniMap::remainder() const
{
  return sstringIn;
}

const SString&
SUniMap::encode (const SV_UCS4& in)
{
  sstringOut.clear ();
  ucs4vIn.append(in);
  unsigned int i;
  if (emodel==0)
  {
    for (i=0; i<ucs4vIn.size(); i++)
    {
      _uniAppend (&sstringOut, ucs4vIn[i]);
    }
    ucs4vIn.clear();
    return sstringOut;
  }
  SStateModel* model = (SStateModel*) emodel;
  model->reset();
  SUniMapDelegate* delg = (SUniMapDelegate*) delegate;
  SBMap* map = (delg==0 || model==0) ? 0 : delg->map;

  // Now go in circles and append till matches.
  int st;
  unsigned int fullsize = ucs4vIn.size();
  i=0;
  while (i<fullsize)
  {
    if (map ==0)
    {
      st = SS_REJECT; 
    }
    else
    {
      model->reset ();
      int more = (in.size()>0)?1:0;
      /* check if map can possible handle our input */
      SS_UCS4 maxBytes = getMaxBytes (map->getInWordSize (eindex));
      unsigned int k;
      for (k=i; k<ucs4vIn.size(); k++)
      {
        if (ucs4vIn[k] > maxBytes) break;
        if ((k-i) >= 10) break; /* FIXME self-imposed limit - map->circle sucks */
      }
      if (k==i)
      {
        st = SS_REJECT;
      }
      else
      {
        st = map->circle (SBMap::SBMap_ENCODE, 
              &ucs4vIn.array()[i], (int)(k-i), model, more);
      }
    }

    if (st == SS_ACCEPT)
    {
       /* remove self-imposed limit */
       model->reset ();
       int more = (in.size()>0)?1:0;
       /* check if map can possible handle our input */
       SS_UCS4 maxBytes = getMaxBytes (map->getInWordSize (eindex));
       unsigned int k;
       for (k=i; k<ucs4vIn.size(); k++)
       {
          if (ucs4vIn[k] > maxBytes) break;
       }
       /* k==i was checked before */
       st = map->circle (SBMap::SBMap_ENCODE, 
            &ucs4vIn.array()[i], (int)(k-i), model, more);
       if (st == SS_ACCEPT)
       {
           break;
       }
    }

    // Append stuff as it is and crunch buffer.
    if (st == SS_REJECT)
    {
      _uniAppend (&sstringOut, ucs4vIn[i]);
      i++;
    }
    // There is a match here. append the output and
    // Shift the input.
    else if (st>0)
    {
      // Model is not char !
      for (int j = 0; j<model->out.length; j++)
      {
        _uniAppend (&sstringOut, (SS_UCS4) model->out.u.u16[j]);
      }
      while (st--)
      {
        i++;
      }
    }
    // Provision for the bad maps.
    else 
    {
      fprintf (stderr, "SUniMap:: decode bad map zero matched!\n");
      _uniAppend (&sstringOut, ucs4vIn[i]);
      i++;
    }
  }
  SV_UCS4 vrest;
  while (i<fullsize)
  {
    vrest.append (ucs4vIn[i]);
    i++;
  }
  ucs4vIn = vrest;
  return sstringOut;
}

const SString&
SUniMap::encodeBuffer()
{
  return sstringIn;
}

const SV_UCS4&
SUniMap::decodeBuffer()
{
  return ucs4vIn;
}

static void
_uniAppend (SString* s, SS_UCS4 u)
{
  if (u < 0x100)
  {
    char u8= (char) u;
    s->append (&u8, 1);
  }
  else if (u < 0x10000)
  {
    char u16[32];
    snprintf (u16, 32, "\\u%04x", (unsigned int) u);
    s->append (u16);
  }
  else 
  {
    char u32[32];
    snprintf (u32, 32, "\\U%04x", (unsigned int) u);
    s->append (u32);
  }
}

/**
 * Lift off whetever can be decoded/encoded
 * This version does not work with circular maps.
 * @param in is the input
 * @param inindex is the starting point to process input.
 * @param out is the output. It can be null.
 * @praram decode is true if we are decoding.
 * @return the new index after liftoff.
 */
unsigned int 
SUniMap::lift (const SV_UCS4& in, unsigned int inindex, 
    bool isdecode, SV_UCS4* out)
{
  if (!ok) return inindex;
  if (delegate==0) return inindex;

  /* make sure we don't have too big values */
  SBMap *map = ((SUniMapDelegate*) delegate)->map;

  int mindex = isdecode ? dindex : eindex;
  SS_UCS4 maxBytes = getMaxBytes (map->getInWordSize (mindex));
  if (in[inindex] > maxBytes || maxBytes == 0) return inindex;

  if (mindex < 0) return inindex;

  SStateModel* model = isdecode ? 
       (SStateModel*) dmodel4 : (SStateModel*) emodel4;

  model->reset();
  
  unsigned int realend = in.size();
  int more = 1;
  int status = SS_ACCEPT;
  unsigned int i;
  for (i=inindex; status == SS_ACCEPT && i < realend; i++)
  {
    /* more == 0 would return - correctly, the whole string */
    if (i+1>=realend || in[i+1] > maxBytes)
    {
      more = 0;
    }
    status = map->encode (mindex, &in.array()[i], 1, model, more);
  }

  if (status <= 0) return inindex;
  if (out == 0) return inindex + (unsigned int) status;

  for (i=0; (int)i< model->out.length; i++)
  {
    out->append ((SS_UCS4)(model->out.u.u32[i]));
  }
  return inindex + (unsigned int) status;
}

/**
 * return key value map to see what decodes to what
 * @param key will contain the keys
 * @param value will contain the values
 * @param _size is the maximum size of returned arrays
 * @return the real size of the arrays.
 */
unsigned int
SUniMap::getDecoderMap (SStringVector* key, SStringVector* value,
        unsigned int _size)
{
  if (delegate==0) return 0;
  /* make sure we don't have too big values */
  SBMap *map = ((SUniMapDelegate*) delegate)->map;
  return map->getDecoderMap (key, value, _size);
}

/**
 * return true if this is an UMap.
 * umaps are generally faster.
 */
bool
SUniMap::isUMap() const
{
  if (delegate ==0) return true;
  return ((((SUniMapDelegate*) delegate)->map)->isUMap() !=0);
}

/**
 * return true if this is a type 4 bumap
 */
bool
SUniMap::isClustered() const
{
  if (delegate ==0) return true;
  return ((((SUniMapDelegate*) delegate)->map)->mapType==4);
}

  /* For maps with holes */
unsigned int
SUniMap::getDecodePosition (SS_UCS4 key)
{
  if (dindex < 0) return 0;
  SUniMapDelegate* d = (SUniMapDelegate*) delegate;
  SBMap *map = d->map;
  return map->getLinearPosition((unsigned int)dindex, key);
}
SS_UCS4
SUniMap::getDecodeKey (unsigned int position)
{
  if (dindex < 0) return 0;
  SUniMapDelegate* d = (SUniMapDelegate*) delegate;
  SBMap *map = d->map;
  return map->getLinearKey((unsigned int)dindex, position);
}
SS_UCS4
SUniMap::getDecodeValue (unsigned int position)
{
  if (dindex < 0) return 0;
  SUniMapDelegate* d = (SUniMapDelegate*) delegate;
  SBMap *map = d->map;
  return map->getLinearValue((unsigned int)dindex, position);
}

unsigned int
SUniMap::getEncodePosition (SS_UCS4 key)
{
  if (eindex < 0) return 0;
  SUniMapDelegate* d = (SUniMapDelegate*) delegate;
  SBMap *map = d->map;
  return map->getLinearPosition((unsigned int)eindex, key);
}
SS_UCS4
SUniMap::getEncodeKey (unsigned int position)
{
  if (eindex < 0) return 0;
  SUniMapDelegate* d = (SUniMapDelegate*) delegate;
  SBMap *map = d->map;
  return map->getLinearKey((unsigned int)eindex, position);
}
SS_UCS4
SUniMap::getEncodeValue (unsigned int position)
{
  if (eindex < 0) return 0;
  SUniMapDelegate* d = (SUniMapDelegate*) delegate;
  SBMap *map = d->map;
  return map->getLinearValue((unsigned int)eindex, position);
}

/**
 * Convert mys length to max value.
 * return 0 on fail.
 */
static SS_UCS4
getMaxBytes (unsigned int inp)
{
  /* FIXME: maxBytes algorithm is fixed here. please fix it
   * in decode and encode - look for maxBytes.
   */
  switch (inp)
  {
  case 0: return (0xff);
  case 1: return (0xffff);
  case 2: return (0xffffffff);
  }
  /* don't support 64 bit */
  return 0;
}
