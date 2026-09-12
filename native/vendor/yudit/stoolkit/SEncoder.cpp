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
 
#include "stoolkit/SString.h"
#include "stoolkit/SBinHashtable.h"
#include "stoolkit/SEncoder.h"
#include "stoolkit/sencoder/SBEncoder.h"
#include "stoolkit/sencoder/SB_UTF8.h"
#include "stoolkit/sencoder/SB_Java.h"
#include "stoolkit/sencoder/SB_NCR.h"
#include "stoolkit/sencoder/SB_UTF7.h"
#include "stoolkit/sencoder/SB_Generic.h"
#include "stoolkit/sencoder/SB_EUC_JP.h"
#include "stoolkit/sencoder/SB_S_JIS.h"
#include "stoolkit/sencoder/SB_X11_JP.h"
#include "stoolkit/sencoder/SB_ISO2022_JP.h"
#include "stoolkit/sencoder/SB_X11_HZ.h"
#include "stoolkit/sencoder/SB_GB2312_8.h"
#include "stoolkit/sencoder/SB_GB18030.h"
#include "stoolkit/sencoder/SB_HZ.h"
#include "stoolkit/sencoder/SB_X11_KSC.h"
#include "stoolkit/sencoder/SB_EUC_KR.h"
#include "stoolkit/sencoder/SB_UHC.h"
#include "stoolkit/sencoder/SB_Johab.h"
#include "stoolkit/sencoder/SB_BIG5.h"
#include "stoolkit/sencoder/SB_UCS2.h"
#include "stoolkit/sencoder/SB_UInput.h"
#include "stoolkit/sencoder/SB_DeShape.h"
#include "stoolkit/sencoder/SB_BiDi.h"
#include "stoolkit/sencoder/SB_S_JIS0213.h"
#include "stoolkit/sencoder/SB_EUC_JP0213.h"
#include "stoolkit/sencoder/SB_ISO2022_JP3.h"
#include "stoolkit/SExcept.h"
#include "stoolkit/SUniMap.h"


static SStringVector _built_in(
"utf-8,utf-8-s,utf-7,java,java-s,ncr,ucs-2,ucs-2-le,ucs-2-be,utf-16,utf-16-le,utf-16-be,euc-jp,euc-jp-3,euc-kr,big-5,hz,iso-2022-x11,ksc-5601-x11,gb-18030,gb-2312-x11,gb-2312,iso-2022-jp,iso-2022-jp-3,shift-jis,shift-jis-3,uhc,johab,unicode,bidi"
);
/**
 * Vector all the built-in encodings.
 */
const SStringVector&
SEncoder::builtin()
{
  return _built_in;
}

/**
 * return all the external maps available
 */
SStringVector
SEncoder::external()
{
  SBinHashtable<int> mentioned;
  for (unsigned int i=0; i<_built_in.size(); i++)
  {
    mentioned.put (_built_in[i], 1);
  }
  SStringVector ext = SUniMap::list();
  SStringVector ret;
  for (unsigned int j=0; j<ext.size(); j++)
  {
    if (mentioned.get(ext[j])!=0) continue;
    mentioned.put (ext[j], 1);
    ret.append (ext[j]);
  }
  return SStringVector(ret);
}

/**
 * Try to find the converter. Default is utf-8
 * New SBEncoder should  be added here.
 */
void
SEncoder::load()
{
  ok = true;
  if (name == "utf-8")
  {
    delegate = new SB_UTF8(false);
  }
  else if (name == "utf-8-s")
  {
    delegate = new SB_UTF8(true); /* surrogate will be treated as normal char */
  }
  else if (name == "java")
  {
    delegate = new SB_Java(false);
  }
  else if (name == "java-s") /* surrogate will be treated as normal char */
  {
    delegate = new SB_Java(true);
  }
  else if (name == "ncr")
  {
    delegate = new SB_NCR();
  }
  else if (name == "utf-7")
  {
    delegate = new SB_UTF7();
  }
  else if (name == "gb-18030")
  {
    SB_GB18030* gb18030 = new SB_GB18030();
    ok = gb18030->isOK();
    delegate = gb18030;
  }
  else if (name == "big-5")
  {
    SB_BIG5* big_5 = new SB_BIG5();
    ok = big_5->isOK();
    delegate = big_5;
  }
  else if (name == "euc-jp")
  {
    SB_EUC_JP* euc_jp = new SB_EUC_JP();
    ok = euc_jp->isOK();
    delegate = euc_jp;
  }
  else if (name == "euc-jp-3")
  {
    SB_EUC_JP0213* euc_jp0213 = new SB_EUC_JP0213();
    ok = euc_jp0213->isOK();
    delegate = euc_jp0213;
  }
  else if (name == "euc-kr")
  {
    SB_EUC_KR* euc_kr = new SB_EUC_KR();
    ok = euc_kr->isOK();
    delegate = euc_kr;
  }
  else if (name == "uhc")
  {
    SB_UHC* uhc = new SB_UHC();
    ok = uhc->isOK();
    delegate = uhc;
  }
  else if (name == "ucs-2")
  {
    delegate = new SB_UCS2(SB_UCS2::AUTO_END, false);
  }
  /* I don't know why, it is all mixed up. workaround - mix them up */
  else if (name == "ucs-2-be")
  {
    delegate = new SB_UCS2(SB_UCS2::LITTLE_END, false);
  }
  else if (name == "ucs-2-le")
  {
    delegate = new SB_UCS2(SB_UCS2::BIG_END, false);
  }
  else if (name == "utf-16")
  {
    delegate = new SB_UCS2(SB_UCS2::AUTO_END, true);
  }
  /* I don't know why, it is all mixed up. workaround - mix them up */
  else if (name == "utf-16-be")
  {
    delegate = new SB_UCS2(SB_UCS2::LITTLE_END, true);
  }
  else if (name == "utf-16-le")
  {
    delegate = new SB_UCS2(SB_UCS2::BIG_END, true);
  }
  else if (name == "johab")
  {
    SB_Johab* johab = new SB_Johab();
    ok = johab->isOK();
    delegate = johab;
  }
  else if (name == "iso-2022-jp")
  {
    SB_ISO2022_JP* iso2022_jp = new SB_ISO2022_JP();
    ok = iso2022_jp->isOK();
    delegate = iso2022_jp;
  }
  else if (name == "iso-2022-jp-3")
  {
    SB_ISO2022_JP3* iso2022_jp3 = new SB_ISO2022_JP3();
    ok = iso2022_jp3->isOK();
    delegate = iso2022_jp3;
  }
  else if (name == "iso-2022-x11")
  {
    SB_X11_JP* x11_jp = new SB_X11_JP();
    ok = x11_jp->isOK();
    delegate = x11_jp;
  }
  else if (name == "shift-jis")
  {
    SB_S_JIS* s_jis = new SB_S_JIS();
    ok = s_jis->isOK();
    delegate = s_jis;
  }
  else if (name == "shift-jis-3")
  {
    SB_S_JIS0213* s_jis0213 = new SB_S_JIS0213();
    ok = s_jis0213->isOK();
    delegate = s_jis0213;
  }
  else if (name == "shift-jis-0213") /* alias to hide shift-jis-0213.my */
  {
    SB_S_JIS0213* s_jis0213 = new SB_S_JIS0213();
    ok = s_jis0213->isOK();
    delegate = s_jis0213;
  }
  else if (name == "gb-2312-x11")
  {
    SB_X11_HZ* x11_hz = new SB_X11_HZ();
    ok = x11_hz->isOK();
    delegate = x11_hz;
  }
  else if (name == "gb-2312")
  {
    SB_GB2312_8* gb_2312_8 = new SB_GB2312_8();
    ok = gb_2312_8->isOK();
    delegate = gb_2312_8;
  }
  else if (name == "ksc-5601-x11")
  {
    SB_X11_KSC* gb_x11_ksc = new SB_X11_KSC();
    ok = gb_x11_ksc->isOK();
    delegate = gb_x11_ksc;
  }
  else if (name == "hz")
  {
    SB_HZ* hz = new SB_HZ();
    ok = hz->isOK();
    delegate = hz;
  }
  else if (name == "unicode")
  {
    SB_UInput* uni = new SB_UInput();
    ok = true;
    delegate = uni;
  }
  else if (name == "deshape")
  {
    SB_DeShape* deshape = new SB_DeShape();
    ok = deshape->isOK();
    delegate = deshape;
  }
  else if (name == "bidi")
  {
    SB_BiDi* bidi = new SB_BiDi();
    ok = bidi->isOK();
    delegate = bidi;
  }
  else
  {
    SB_Generic* g = new SB_Generic(name);
    ok = g->isOK();
    if (ok)
    {
      delegate = g;
    }
    else
    {
      delete g;
      delegate = new SB_UTF8(false);
    }
  }
}

/**
 * Create a utf-8 converter
 */
SEncoder::SEncoder (void)
{
  name = "utf-8";
  ok = true;
  load();
}

/**
 * return false if something is wrong with the map:
 *  The map not found or similar
 */
bool
SEncoder::isOK () const
{
  return ok;
}

/**
 * Create a converter with a name
 * @param name is either a valid name 
 * or a map
 */
SEncoder::SEncoder (const SString& _name)
{
  name = _name;
  ok = true;
  load ();
}

SEncoder::SEncoder (const SEncoder& c)
{
  name = c.getName();
  load ();
}

SEncoder&
SEncoder::operator = (const SEncoder& c)
{
  if (this != &c)
  {
    delete ((SBEncoder*) delegate);
    name = c.getName();
    load ();
    clear();
  }
  return *this;
}

SEncoder::~SEncoder ()
{
  delete ((SBEncoder*) delegate);
}

const SString&
SEncoder::getName() const
{
  return name;
}

/**
 * This is encoding a unicode string into a bytestring
 * @param input is a unicode string.
 */
const SString&
SEncoder::encode (const SV_UCS4& input)
{
  return ((SBEncoder*) delegate)->encode (input);
}

void
SEncoder::clear()
{
  buffer.clear();
  delim.clear();
  remaining.clear();
  ((SBEncoder*) delegate)->clear();
}
/**
 * Decode an input string into a unicode string.
 * @param input is a string.
 *   he output can be null, in this case a line is not
 *   read fully. If input size is zero output will be flushed.
 */
const SV_UCS4&
SEncoder::decode (const SString& input, bool more)
{
  if (delim.size() == 0 && input.size()!=0)
  {
    ((SBEncoder*) delegate)->delimiters(input);
  }
  buffer.append (input);
  /**
   * We need more input for the delimiter?
   */
  if (delim.size() != 0 && more)
  {
    unsigned int i;
    /* there is a potential bug here - r n should be specified 
       in front of r or n  */
    for (i=0; i<delim.size(); i++)
    {
      if (buffer.find (delim[i]) >= 0) break;
    }
    retUCS4.clear();
    if (i==delim.size()) return retUCS4;
  }
  retUCS4 =  ((SBEncoder*) delegate)->decode (buffer);
  SV_UCS4 additional;
  if (!more)
  {
    additional = ((SBEncoder*) delegate)->decode("");
    retUCS4.append (additional);
    
  }
  buffer.clear();
  return retUCS4;
}

/**
 * return key value map to see what decodes to what
 * @param key will contain the keys
 * @param value will contain the values
 * @param _size is the maximum size of returned arrays
 * @return the real size of the arrays.
 */
unsigned int
SEncoder::getDecoderMap (SStringVector* key, SStringVector* value,
        unsigned int _size)
{
   return ((SBEncoder*) delegate)->getDecoderMap (key, value, _size);
}

/* for non-clustering it is remainder */
SString
SEncoder::preEditBuffer() const
{
  SString rm = ((SBEncoder*) delegate)->preEditBuffer();
  rm.append (buffer);
  return SString(rm);
}
 /* for clustering */
SV_UCS4
SEncoder::postEditBuffer () const
{
   return ((SBEncoder*) delegate)->postEditBuffer();
}

/**
 * These methods guess the line delimiters for the input
 * The one without arguments is giving the 'first approximation'
 * It returns an inclusive list of all possibilities.
 */
const SStringVector&
SEncoder::delimiters ()
{
  return ((SBEncoder*) delegate)->delimiters();
}

SObject*
SEncoder::clone() const
{
  SEncoder* n = new SEncoder(name);
  CHECK_NEW (n);
  return n;
}
