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

#define DEBUG_ALL 0

/* 
* Overhang is not supposed to move cursor in pothana2000.ttf 
* With this defined as 1 we hack it. It happens mainly with
* Mbelow.
*/
#define DO_FIX_BROKEN_BELOW 1


/*!
 * \file SScriptProcessor.cpp
 * \brief Script Processing routines.
 * \author: Gaspar Sinai <gaspar@yudit.org>
 * \version: 2000-04-23
 */

#include "swindow/SScriptProcessor.h"
#include "stoolkit/SCluster.h"

#include <string.h>

static int l_isSupported = -1;
static int l_isInit = 1;

#define SD_NONE 0x7ffffffe

#if DEBUG_ALL
static void debugArray (const char* message,
  const SS_UCS4* arr, unsigned int size);
#endif

static SRendClass::RType  l_devanagari_gujarati_reorder_guide[] =
{
  SRendClass::Mpre,
  SRendClass::Cpre,
  SRendClass::Cbase,
  SRendClass::VO,  
  SRendClass::Cbelow,
  SRendClass::Cpost,
  SRendClass::Mbelow,
  SRendClass::VMbelow,
  SRendClass::SMbelow,
  SRendClass::Creph,
  SRendClass::Mabove,
  SRendClass::Mpost,
  SRendClass::CMreph,
  SRendClass::VMabove,
  SRendClass::SMabove,
  SRendClass::VMpost,
  SRendClass::None
};

static SRendClass::RType  l_bengali_oriya_reorder_guide[] =
{
  SRendClass::Mpre,
  SRendClass::Cpre,
  SRendClass::Cbase,
  SRendClass::Cra,
  SRendClass::VO,  
  SRendClass::Creph,
  SRendClass::Cbelow,
  SRendClass::Mbelow,
  SRendClass::Mabove,
  SRendClass::CMreph,
  SRendClass::VMabove,
  SRendClass::Cpost,
  SRendClass::Mpost,
  SRendClass::VMpost,
  SRendClass::None
};

static SRendClass::RType  l_gurmukhi_reorder_guide[] =
{
  SRendClass::Mpre,
  SRendClass::Cpre,
  SRendClass::Cbase,
  SRendClass::VO,  
  SRendClass::Cra,
  SRendClass::Cbelow,
  SRendClass::Mbelow,
  SRendClass::SMbelow,
  SRendClass::Mabove,
  SRendClass::Cpost,
  SRendClass::Mpost,
  SRendClass::CMra,
  SRendClass::VMabove,
  SRendClass::SMabove,
  SRendClass::None
};

static SRendClass::RType  l_telugu_reorder_guide[] =
{
  SRendClass::Cpre, 
  SRendClass::Cbase, 
  SRendClass::VO, 
  SRendClass::Cra, 
  SRendClass::CMra, 
  SRendClass::Mabove, 
  SRendClass::Mbelow, 
  SRendClass::Mpost, 
  SRendClass::Cbelow, 
  SRendClass::Cpost, 
  SRendClass::VMpost, 
  SRendClass::None,
};

static SRendClass::RType  l_kannada_reorder_guide[] =
{
  SRendClass::Cpre,
  SRendClass::Cbase,
  SRendClass::VO,  
  SRendClass::Mabove,
  SRendClass::Mpost,
  SRendClass::Cbelow,
  SRendClass::Cpost,
  SRendClass::LMpost, // For Kannada 0CD5 and 0CD6 is not in indic.txt
  SRendClass::Creph,
  SRendClass::CMreph,
  SRendClass::VMpost,
  SRendClass::None
};

static SRendClass::RType  l_malayalam_reorder_guide[] =
{
  SRendClass::Mpre, // put before Cbase in gsubclean
  SRendClass::Cra,
  SRendClass::CMra,
  SRendClass::Cpre,
  SRendClass::Cbase,
  SRendClass::VO,  
  SRendClass::Cbelow,
  SRendClass::Mbelow,
  SRendClass::Creph, // This makes it traditional. It will be
                     // automatically re-ordered if not found int the font.
  SRendClass::Cpost,
  SRendClass::Mpost,
  SRendClass::CMreph, // This makes it traditional. It will be
                      // automatically re-ordered if not found int the font.
  SRendClass::VMpost,
  SRendClass::None
};

static SRendClass::RType  l_tamil_reorder_guide[] =
{
  SRendClass::Mpre, // put before base in gsubclean
  SRendClass::Cpre,
  SRendClass::Cbase,
  SRendClass::Cra,
  SRendClass::CMra,
  SRendClass::VO,  
  SRendClass::Cbelow,
  SRendClass::Mabove,
  SRendClass::Mpost,
  SRendClass::VMpost,
  SRendClass::None
};

/* FIXME */
static SRendClass::RType  l_sinhala_reorder_guide[] =
{
  SRendClass::Cpre,
  SRendClass::Cbase,
  SRendClass::VO,  
  SRendClass::Mabove,
  SRendClass::Mpost,
  SRendClass::Cbelow,
  SRendClass::Cpost,
  SRendClass::LMpost, // For Kannada 0CD5 and 0CD6 is not in indic.txt
  SRendClass::Creph,
  SRendClass::CMreph,
  SRendClass::VMpost,
  SRendClass::None
};

/*!
 * \brief Find out if the script is supported by this 
 * shaping engine.
 * \param c is the first unicode character.
 */
bool
SScriptProcessor::isSupported (SS_UCS4 c) const
{
  static bool warned = false;

  /* Issue a warning for incompatible yudit installation */
  if (SRendClass::get(0x915)!=SRendClass::Cbase)
  {
    if (!warned)
    {
       fprintf (stderr, "WARNING: new indic.my map is not installed. (SScriptProcessor will turn off now.)\n");
       warned = true;
    }
    return false;
  }
  if (c==0) return false;
  if (l_isSupported==0) return false;
  /* Devanagari */
  if (c >= 0x0900 && c<= 0x097f) return true;
  /* Bengali */
  if (c >= 0x0980 && c<= 0x09ff) return true;
  /* Gurmukhi */
  if (c >= 0x0a00 && c<= 0x0a7f) return true;
  /* Gujarati */
  if (c >= 0x0a80 && c<= 0x0aff) return true;
  /* Oriya */
  if (c >= 0x0b00 && c<= 0x0b7f) return true;
  /* Tamil - supported with fixed clusters */
  if (c >= 0x0b80 && c<= 0x0bff) return false; //<<<<
  /* Telugu */
  if (c >= 0x0c00 && c<= 0x0c7f) return true;
  /* Kannada */
  if (c >= 0x0c80 && c<= 0x0cff) return true;
  /* Malayalam */
  if (c >= 0x0d00 && c<= 0x0d7f) return true;
  /* Sinhala - not supported, need to get info */
  if (c >= 0x0d80 && c<= 0x0dff) return true; //<<<<
  /* Thai */
  if (c >= 0x0e00 && c<= 0x0e7f) return true;
  /* Lao */
  if (c >= 0x0e80 && c<= 0x0eff) return true;
  /* Tibetan */
  if (c >= 0x0f00 && c<= 0x0fff) return true;
  /* Jamo */
  if (c >= 0x1100 && c<= 0x11ff) return true;
  return false;
}

/*!
 * \brief Turn the engine on and off.
 * \param on is true if the engine turns on.
 */
void
SScriptProcessor::support (bool on)
{
  l_isSupported = on ? 1 : 0;
}

/*!
 * \brief Turn the engine on and off.
 * \param on is true if the engine turns on.
 */
void
SScriptProcessor::doInit (bool on)
{
  l_isInit = on ? 1 : 0;
}

/*!
 * \brief Create a new font engine. 
 * \font is an abstract font-dependent interface.
 */
SScriptProcessor::SScriptProcessor (SFontLookup* font)
{
  m_script = SC_NONE;
  m_otfScript = "latn";
  m_font = font;
}

SScriptProcessor::~SScriptProcessor (void)
{
}

/*!
 * \put a string into the buffer for processing.
 * \param in is the input unicode array.
 * \param len is the size of the input unicode array.
 * \param is_begin is true if this is at the beginning of the word.
 * \return the number of character that will be processed.
 */
unsigned int
SScriptProcessor::put (const SS_UCS4* in, unsigned int len, bool is_begin)
{
  m_script = SC_NONE;
  if (len == 0) return 0;
  m_is_begin = is_begin;

  unsigned int reallen = 0; // Real length.
  unsigned int min = 0;     // minimum range (inclusive)
  unsigned int max = 0;     // maximum range (non-inclusive)
  m_script_has_reph = false;
  m_ra = 0;
  const SRendClass::RType* order = 0; 

  SS_UCS4 sample = ((in[0] == 0x200D || in[0] == 0x25cc) && len > 1) 
           ? in[1] : in[0];

  // Devanagari
  if (sample >= 0x0900 && sample < 0x0980)
  {
    m_script = SC_DEVANAGARI;
    m_otfScript = "deva";
    min = 0x0900; max = 0x0980;
    m_script_has_reph = true;
    order = l_devanagari_gujarati_reorder_guide;
  }
  // Bengali
  else if (sample >= 0x0980 && sample < 0x0a00)
  {
    m_script = SC_BENGALI;
    m_otfScript = "beng";
    min = 0x0980; max = 0x0a00;
    m_script_has_reph = true;
    order = l_bengali_oriya_reorder_guide;
  }
  // Gurmukhi
  else if (sample >= 0x0a00 && sample < 0x0a80)
  {
    m_script = SC_GURMUKHI;
    m_otfScript = "guru";
    min = 0x0a00; max = 0x0a80;
    m_script_has_reph = false;
    order = l_gurmukhi_reorder_guide;
  }
  // Gujarati
  else if (sample >= 0x0a80 && sample < 0x0b00)
  {
    m_script = SC_GUJARATI;
    m_otfScript = "gujr";
    min = 0x0a80; max = 0x0b00;
    m_script_has_reph = true;
    order = l_devanagari_gujarati_reorder_guide;
  }
  // Oriya
  else if (sample >= 0x0b00 && sample < 0x0b7f)
  {
    m_script = SC_ORIYA;
    m_otfScript = "orya";
    min = 0x0b00; max = 0x0b80;
    m_script_has_reph = true;
    order = l_bengali_oriya_reorder_guide;
  }
  // Tamil
  else if (sample >= 0x0b80 && sample < 0x0c00)
  {
    m_script = SC_TAMIL;
    m_otfScript = "taml";
    min = 0x0b80; max = 0x0c00;
    order = l_tamil_reorder_guide;
  }
  // Telugu
  else if (sample >= 0x0c00 && sample < 0x0c80)
  {
    m_script = SC_TELUGU;
    m_otfScript = "telu";
    min = 0x0c00; max = 0x0c80;
    order = l_telugu_reorder_guide;
  }
  // Kannada
  else if (sample >= 0x0c80 && sample < 0x0d00)
  {
    m_script = SC_KANNADA;
    m_otfScript = "knda";
    min = 0x0c80; max = 0x0d00;
    order = l_kannada_reorder_guide;
    m_script_has_reph = true;
  }
  // Malayalam
  else if (sample >= 0x0d00 && sample < 0x0d80)
  {
    m_script = SC_MALAYALAM;
    m_otfScript = "mlym";
    min = 0x0d00; max = 0x0d80;
    order = l_malayalam_reorder_guide;
    // Test reph with the font.
    m_out.clear();
    m_out_type.clear();

    m_out.append (0x0d30);
    m_out_type.append (SRendClass::Creph); 

    m_out.append (0x0d4d);
    m_out_type.append (SRendClass::Halant); 

    gsub("rphf");
    gsub("abvs");
    if (m_out.size()<2)
    {  
       m_script_has_reph = true;
    }
    m_out.clear();
    m_out_type.clear();
  }
  // Sinhala - not supported yet - have not seen any fonts.
  else if (sample >= 0x0d80 && sample < 0x0e00)
  {
    m_script = SC_SINHALA;
    m_otfScript = "sinh";
    min = 0x0d80; max = 0x0e00;
    order = l_sinhala_reorder_guide;
    m_script_has_reph = true;
  }
  // Thai
  else if (sample >= 0x0e00 && sample < 0x0e80)
  {
    m_script = SC_THAI;
    m_otfScript = "thai";
    min = 0x0e00; max = 0x0e80;
    order = 0;
  }
  // Lao
  else if (sample >= 0x0e80 && sample < 0x0f00)
  {
    m_script = SC_LAO;
    m_otfScript = "lao ";
    min = 0x0e80; max = 0x0f00;
    order = 0;
  }
  // Tibetan
  else if (sample >= 0x0f00 && sample < 0x1000)
  {
    m_script = SC_TIBETAN;
    m_otfScript = "tibt";
    min = 0x0f00; max = 0x1000;
    order = 0;
  }
  // Hangul Jamo
  else if (sample >= 0x1100 && sample < 0x1200)
  {
    m_script = SC_JAMO;
    m_otfScript = "jamo";
    min = 0x1100; max = 0x1200;
    order = 0;
  }
  else 
  {
    return 0;
  }
  // For some scripts it is irrelevant.
  m_ra = min + 0x30;

  // Stop at first non-telugu.
  for (reallen=1; reallen<len; reallen++)
  {
    // ignore ZWJ, ZWNJ
    if (in[reallen] == 0x200D) continue; 
    if (in[reallen] == 0x200C) continue; 
    if (in[reallen] == 0x25CC) continue; 
    if (in[reallen]<min) break;
    if (in[reallen]>=max) break;
  }

  unsigned int i=0;

  /* initialize m_order */
  m_reorder_guide.clear();
  while (order!=0 && order[i]!=SRendClass::None)
  {
    m_reorder_guide.append (order[i]);
    i++;
  }
  /* 
   * Our cluster is ready-made so we dont need to check
   * if each character is telugu or ZWJ,ZWNJ.
   */
  for (i=0; i<reallen; i++)
  {
    m_orig.append (in[i]);
  }
  m_in = m_orig;
  m_out.clear();
  m_out_type.clear();

  m_positions.clear();
  if (!gindex()) return 0;
  return reallen;
}

/*!
 * \return the glyph results of the apply() method. 
 */
const SV_GlyphIndex&
SScriptProcessor::getGlyphs () const
{
  return m_out;
}
/*!
 * \return the positioning results of the apply() method. 
 */
const SV_INT&
SScriptProcessor::getPositions () const
{
  return m_positions;
}

/*!
 * \brief Decompose all glyphs in the buffer.
 *  vowel marks will also get decomposed into left and right part.
 *  This manipulates the m_in array.
 */
void
SScriptProcessor::decompose ()
{
  for (unsigned int i=0; i<m_in.size(); i++)
  {
again:
    SS_UCS4 u = m_in[i];
    SS_UCS4 left;
    SS_UCS4 right;
    if (SRendClass::split (u, &left, &right))
    {
#if DEBUG_ALL
      if (left==0||right==0)
      {
        fprintf (stderr, 
          "SScriptProcessor::decompose: does not work on %04X\n", u);
      }
#endif
      m_in.replace (i, left);
      m_in.insert (i+1, right);
      goto again; // may need further split.
    }
  }
}

/*!
 * \brief apply an otf ligature substitution OTF feature.
 *  This manipulates the m_out array.
 */
void
SScriptProcessor::gsub (const char* feature)
{
  SS_GlyphIndex* lig = new SS_GlyphIndex[m_out.size()];
  CHECK_NEW(lig);

  if (m_out.size() != m_out_type.size())
  {
#if DEBUG_ALL
     fprintf (stderr, "gsub: fatal error: m_out.size=%u m_out_type_size=%u\n",
       m_out.size(), m_out_type.size());
#endif 
     return;
  }
  const char* ft = feature;
  if (strcmp(feature, "null")==0) ft=0;

  const SS_GlyphIndex* gi;
  unsigned int cutsize;
  unsigned int start;
  unsigned int out_size;
  unsigned int j;
  bool is_init =  strcmp (feature, "init")==0; 
  bool is_blwf =  strcmp (feature, "blwf")==0; 

  for (unsigned int i=0; i+1 < m_out.size(); i++)
  {
    bool is_contextual = false;
    if (is_init && (i > 0 || !m_is_begin)) break; 

    gi = m_out.array();


    // In confirmance with the ISCII standard, the half-consonant form
    // RRAh is represented as eyelash-RA. This form of RA is 
    // commonly used in writing Marathi.
    if (i==0 && m_out.size()>2 && strcmp (feature, "half")==0
       && m_out_type[1] == SRendClass::Halant 
       && gi[0] == m_font->gindex (0x0931))
    {
      SS_GlyphIndex eye[2];
      eye[0] = m_font->gindex (m_ra); 
      eye[1] = gi[1]; 
      cutsize = m_font->gsub (m_otfScript, ft, eye,  2, 
          &start, lig, &out_size, &is_contextual);
#if DEBUG_ALL
     if (cutsize)
     {
        fprintf (stderr, "gsub: fixed 0x0931 + halant + zwj as eyelash.\n");
     }
#endif      
      // Fallback to original.
      if (cutsize==0)
      {
        cutsize = m_font->gsub (m_otfScript, ft, &gi[i],  m_out.size()-i, 
            &start, lig, &out_size, &is_contextual);
      }
    }
    // RA+Halant+ZWJ+Consonant
    else if (is_blwf
      && i+3< m_out.size() 
      && m_out_type[i] == SRendClass::Cfirst
      && m_out_type[i+1] == SRendClass::Halant
      && m_out_type[i+2] == SRendClass::ZWJ
      && m_out[i] == m_font->gindex (0x0cb0))
    {
      SS_GlyphIndex bl[2];
      bl[0] = m_out[i+3];
      bl[1] = m_out[i+1];
      cutsize = m_font->gsub (m_otfScript, ft, bl,  2, 
          &start, lig, &out_size, &is_contextual);
      if (cutsize==2 && out_size==1)
      {
#if DEBUG_ALL
        fprintf (stderr, "gsub: fixed Kannada ra+halant+zwj.\n");
#endif      
        out_size = 2;
        cutsize = 4;
        start = 0;
        lig[1] = lig[0];
        lig[0] = m_out[i];
        is_contextual = true; // to be sure it passes.
      }
      else
      {
        cutsize = m_font->gsub (m_otfScript, ft, &gi[i],  m_out.size()-i, 
            &start, lig, &out_size, &is_contextual);
      }
    }
    // Malayalam post 0D31 should render same a 0D30
    else if (allowed(feature, m_out_type[i]) 
      && i+1< m_out.size() 
      && (m_out_type[i] == SRendClass::Cpost
          || m_out_type[i] == SRendClass::Clast
          || (i+2 < m_out.size() && m_out_type[i+2] == SRendClass::ZWJ))
      && m_out_type[i+1] == SRendClass::Halant
      && m_out[i] == m_font->gindex (0x0D31))
    {
      cutsize = m_font->gsub (m_otfScript, ft, &gi[i],  m_out.size()-i, 
          &start, lig, &out_size, &is_contextual);
      if (cutsize==0)
      {
        SS_GlyphIndex bl[2];
        bl[0] = m_font->gindex (m_ra);
        bl[1] = m_out[i+1];
        cutsize = m_font->gsub (m_otfScript, ft, bl,  2, 
            &start, lig, &out_size, &is_contextual);
        if (cutsize==2 && out_size==1)
        {
#if DEBUG_ALL
          fprintf (stderr, "gsub: fixed Malayalam ..ta+halant+zwj as ra+halant+zwj.\n");
#endif      
          m_out.replace (i, bl[0]);
          start = 0;
        }
        else
        {
          cutsize = 0;
        }
      }
    }
    // Malayalam post 0D31 should render same a 0D30
    else if (allowed(feature, m_out_type[i]) 
      && i+1< m_out.size() 
      && (m_out_type[i] == SRendClass::Cpost
          || m_out_type[i] == SRendClass::Clast
          || (i+2 < m_out.size() && m_out_type[i+2] == SRendClass::ZWJ))
      && m_out_type[i+1] == SRendClass::Halant
      && m_out[i] == m_font->gindex (m_ra))
    {
      cutsize = m_font->gsub (m_otfScript, ft, &gi[i],  m_out.size()-i, 
          &start, lig, &out_size, &is_contextual);
      if (cutsize==0)
      {
        SS_GlyphIndex bl[2];
        bl[0] = m_font->gindex (0x0d31);
        bl[1] = m_out[i+1];
        cutsize = m_font->gsub (m_otfScript, ft, bl,  2, 
            &start, lig, &out_size, &is_contextual);
        if (cutsize==2 && out_size==1)
        {
#if DEBUG_ALL
          fprintf (stderr, "gsub: fixed Malayalam ra+halant+zwj as ..ta+halant+zwj.\n");
#endif      
          start = 0;
        }
        else
        {
          cutsize = 0;
        }
      }
    }
    // Some malayalam fonts have ill-defined conjuncts - fix them here.
    // Fix la conunct.
    else if (m_script == SC_MALAYALAM 
      && i+2< m_out.size() 
      && m_out_type[i] == SRendClass::Cbase
      && (m_out_type[i+1] == SRendClass::Clast
          || m_out_type[i+1] == SRendClass::Cbelow
          || m_out_type[i+1] == SRendClass::Mbelow
          || m_out_type[i+1] == SRendClass::Cpost)
      && m_out_type[i+2] == SRendClass::Halant
      && m_out[i] == m_font->gindex (0x0d32) // or more if needed.
      && (i+3 >= m_out.size() ||  m_out_type[i+3] != SRendClass::ZWJ))
    {
      cutsize = m_font->gsub (m_otfScript, ft, &gi[i],  m_out.size()-i, 
          &start, lig, &out_size, &is_contextual);
      if (cutsize==0)
      {
        SS_GlyphIndex bl[3];
        // SWAP Halant.
        bl[0] = m_out[i];
        bl[1] = m_out[i+2];
        bl[2] = m_out[i+1];
        cutsize = m_font->gsub (m_otfScript, ft, bl,  3, 
            &start, lig, &out_size, &is_contextual);
        if (cutsize==3 && out_size==1)
        {
          // make it happen.
#if DEBUG_ALL
          fprintf (stderr, "gsub: fixed Malayalam lla.\n");
#endif      
          is_contextual = true;
          start = 0;
        }
        else
        {
          cutsize = 0;
        }
      }
    }
    else
    {
      cutsize = m_font->gsub (m_otfScript, ft, &gi[i],  m_out.size()-i, 
          &start, lig, &out_size, &is_contextual);
    }

    // Check if the substitution is allowed.
    if (cutsize > 0)
    {
      unsigned int end = start+cutsize;
      bool allowed_once = false;
      if (is_contextual)
      {
         allowed_once = true;
      }
      /*
       * In scripts like Malayalam, the halant form of certain consonants
       * is represented by 'chillaksharams'. 
       * Chillakasharams must form if C+H+ZWJ is present.
       * Chillakasharams should bot form if C+H+ZWNJ is present.
       * Conjuncts may form otherwise.
       */
      else if (m_script==SC_MALAYALAM && strcmp (feature, "haln")==0)
      {
        allowed_once = (i+2 < m_out_type.size()
           && m_out_type[i+1] == SRendClass::Halant
           && m_out_type[i+2] == SRendClass::ZWJ);
      }
      else for (j=start; j<end; j++)
      {
        SRendClass::RType type = m_out_type[j+i];
        if (type != SRendClass::Halant 
          && type != SRendClass::Nukta
          && type != SRendClass::ZWJ
          && type != SRendClass::ZWNJ
          && allowed(feature, type)) 
        {
          allowed_once = true;
        }
      }
      // This *must be* haln */
      if (m_script==SC_MALAYALAM 
          && strcmp (feature, "haln")!=0
          && i+2 < m_out_type.size() 
          && m_out_type[i+1] == SRendClass::Halant
          && ( m_out_type[i+2] == SRendClass::ZWJ
              ||  m_out_type[i+2] == SRendClass::ZWNJ))
      {
        allowed_once = false;
      }
      if (!allowed_once)
      {
#if DEBUG_ALL
        fprintf (stderr, "%u gsub %s is not allowed.\n", i, feature);
#endif
        cutsize=0;
      }
    }

    if (cutsize > 0)
    {
      // find out till when we need to cut.
      unsigned int end = start+cutsize;

      // what type should it be?
      SRendClass::RType type = m_out_type[start+i]; // if base was involved move it.
      for (j=start+1; j<end; j++)
      {
        // anything is better than these.
        if (type == SRendClass::Halant || type == SRendClass::Nukta 
          || type == SRendClass::None) 
        {
           type = m_out_type[j+i];
        }
        // we must have a base. maybe more than one :)
        if (m_out_type[i+j]==SRendClass::Chbase 
           || m_out_type[i+j] == SRendClass::Cbase)
        {
           type = m_out_type[j+i];
        } 
        if (type == SRendClass::Cbelow && m_out_type[j+1] == SRendClass::Clast) 
        {
           type = SRendClass::Clast;
        }
        if (type == SRendClass::Cpost && m_out_type[j+1] == SRendClass::Clast) 
        {
           type = SRendClass::Clast;
        }
      }
      if (type == SRendClass::Creph || type==SRendClass::Cra
        || type == SRendClass::CMreph || type==SRendClass::CMra)
      {
        // Mark it as below or above.
        type = (strcmp (feature, "rphf")==0 ||
             strcmp (feature, "abvs")==0)
          ? SRendClass::Mabove : SRendClass::Mbelow;
      }
#if DO_FIX_BROKEN_BELOW
      if (is_blwf && cutsize > 0 
         && type != SRendClass::Cbase 
         && type != SRendClass::Chbase)
      {
        type = SRendClass::Mbelow;
      }
#endif

      // This is from the old program.
      // Post-consonant Malayalam ra has to be reordered to syllable start
      if (out_size ==1 && cutsize ==2 && m_script == SC_MALAYALAM
       && strcmp (feature, "haln")!=0
       &&  m_out_type[i+1] == SRendClass::Halant
       &&  m_out[i] == m_font->gindex (m_ra))
      {
        // out_type is adjusted here. 
        for (j=start; j<end; j++) m_out_type.remove (i+start); // yes
        for (j=start; j<end; j++) m_out.remove (i+start); // yes
        if (m_out_type.size()>0 && m_out_type[0] == SRendClass::Mpre)
        {
          m_out_type.insert (1, type);
          m_out.insert (1, lig[0]);
        }
        else
        {
          m_out_type.insert (0, type);
          m_out.insert (0, lig[0]);
        }
      }
      else
      {
        // out_type is adjusted here. 
        for (j=start; j<end; j++) m_out_type.remove (i+start); // yes
        for (j=0; j<out_size; j++) m_out_type.insert (i+start, type);

        // out is adjusted here. 
        replaceOut (start+i, end+i, lig, out_size);
      }
    }
  }
  delete[] lig;
}

/*!
 * \brief clean the m_out and m_type_out so that it will
 * not have any joiners.
 */
void
SScriptProcessor::gsubclean()
{
  unsigned int i=m_out.size();
  if (i==0) return;
  while (i--)
  {
    if (m_out_type[i] == SRendClass::ZWJ)
    {
      m_out_type.remove (i);
      m_out.remove (i);
    }
    else if (m_out_type[i] == SRendClass::ZWNJ)
    {
      m_out_type.remove (i);
      m_out.remove (i);
    }
    else if (m_out[i] == 0)
    {
      m_out_type.remove (i);
      m_out.remove (i);
    }
  }
  /* 
   * If reph or ra is not found in the font, at least we
   * should make the cluster legible.
   */
  for (i=0; i+1<m_out.size(); i++)
  {
     SRendClass::RType t = m_out_type[i];
     if ((t == SRendClass::Creph 
        || t == SRendClass::Cra
        || t == SRendClass::CMreph 
        || t == SRendClass::CMra
       ) && m_out_type[i+1]==SRendClass::Halant)
     {
        SS_GlyphIndex gi0 = m_out[i];
        SS_GlyphIndex gi1 = m_out[i+1];
        m_out.remove (i);
        m_out.remove (i);
        m_out_type.remove (i);
        m_out_type.remove (i);

        if (m_out_type.size() > 0 && m_out_type[0] == SRendClass::Mpre)
        {
          m_out.insert (1, gi0);
          m_out.insert (2, gi1);
          m_out_type.insert (1, t);
          m_out_type.insert (2, SRendClass::Halant);
        }
        else
        {
          m_out.insert (0, gi0);
          m_out.insert (1, gi1);
          m_out_type.insert (0, t);
          m_out_type.insert (1, SRendClass::Halant);
        }
        // There can be at most one Cra,CMra,CMreph or Creph 
        break;
     }
  }
   // Correcting the reorder-guide. In case of Tamil and
   // Malayalam it is impossible to predict conjuncts.
   // Move pre to its proper place after gsub - here.
  if (m_script == SC_MALAYALAM || m_script == SC_TAMIL)
  {
    SS_GlyphIndex pre = 0;
    for (i=0; i<m_out_type.size(); i++)
    {
      if (m_out_type[i] == SRendClass::Mpre)
      {
         pre = m_out[i];
         m_out.remove (i);
         m_out_type.remove (i);
         break;
      }
    }
    if (pre!=0)
    {
      for (i=0; i<m_out_type.size(); i++)
      {
        if (m_out_type[i] == SRendClass::Cbase
          || m_out_type[i] == SRendClass::Chbase)
        {
           // RA might have been reordered.
           if (m_out_type[0] == SRendClass::Cpost
             || m_out_type[0] == SRendClass::Clast)
           {
             m_out.insert (0, pre);
             m_out_type.insert (0, SRendClass::Mpre);
           }
           else
           {
             m_out.insert (i, pre);
             m_out_type.insert (i, SRendClass::Mpre);
           }
           pre = 0;
           break;
        }
      }
    }
    // It can happen only if we lost Cbase or Chbase.
    // - never happens. Still we dont want to lose this.
    if (pre!=0)
    {
      m_out.insert (0, pre);
      m_out_type.insert (0, SRendClass::Mpre);
    }
  }
  // Move halant from last syllable back to base if it does not 
  // have any.
  unsigned int baseIndex = 0;
  unsigned int halantIndex = 0;
  for (i=0; i<m_out_type.size(); i++)
  {
    if (m_out_type[i] == SRendClass::Cbase
      || m_out_type[i] == SRendClass::Chbase)
    {
       i++;
       baseIndex = i;
    }
    else if (m_out_type[i] == SRendClass::Halant)
    {
       halantIndex = i;
    }
    else if (m_out_type[i] == SRendClass::Cbelow
       || m_out_type[i] == SRendClass::Cpost
       || m_out_type[i] == SRendClass::Clast)
    {
       halantIndex = 0;
    }
  }
  // Move halant back. No substitutions happened.
  if (baseIndex > 0 && halantIndex > baseIndex)
  {
#if DEBUG_ALL
    fprintf (stderr, "gsubclean: moving back unprocessed halant.\n");
#endif 
    SS_GlyphIndex h = m_out[halantIndex];
    SRendClass::RType t = m_out_type[halantIndex];
    m_out.remove (halantIndex);
    m_out_type.remove (halantIndex);
    m_out.insert (baseIndex, h);
    m_out_type.insert (baseIndex, t);
    gsub ("haln");
  }
}

/*!
 * \brief Initialize positioning array.
 */
void
SScriptProcessor::gposInit ()
{
  m_xpos.clear();
  m_ypos.clear();
  m_pos_base_index.clear();
  for (unsigned int i=0; i<m_out.size(); i++)
  {
    m_xpos.append (0);
    m_ypos.append (0);
    unsigned int ref = (i>0)? (i-1) : 0;
    m_pos_base_index.append (ref);
  }
}

/*!
 * \brief Finalize positioning. Store values at m_positions.
 *  This manipulates the m_out array.
 */
void
SScriptProcessor::gposFinal ()
{
  m_positions.clear();
  m_width = 0;
  if (m_out.size()==0) return;

  /* store these for first run */
  m_positions.append(0);
  m_xpos.replace (0,0);
  m_ypos.replace (0,0);

  int width = m_font->gwidth(m_out[0]);
  if (width<0) width = -width;
  for (unsigned int i=1; i<m_out.size(); i++)
  {
    int x = 0;
    int y = 0;
    if (m_xpos[i]!=0 || m_ypos[i]!=0) // We have a position.
    {
      /* we have a relative mark-to-base or mark-to-mark position */
      x = m_xpos[i] + m_xpos[m_pos_base_index[i]];
      y = m_ypos[i] + m_ypos[m_pos_base_index[i]];
      /* if it sticks out update our width */
      int w = m_font->gwidth (m_out[i]); 
      if (x+w > width) width = x+w;
    }
    else // We don't have position. Calculate the position.
    {
      // this negative thing is just a hack. gwidth never actually works.
      int w = m_font->gwidth (m_out[i]); 
      if (w < 0) w = -w;
      if (w > 0) // left aligned.
      {
        x = width;
        y = 0;
        width += w; // move cursor.
      }
      else // right aligned, composing, zero width.
      {
        x = width - w;
        y = 0;
      }
    }
    /* store it for next run */
    m_xpos.replace (i, x);
    m_ypos.replace (i, y);
    int xy = (y << 16) & 0xffff0000;
    xy = xy | (x & 0xffff);
    // store packed position
    m_positions.append(xy);
  }
  m_width = width;
}

/*!
 * \return the advance width of this cluster.
 */
int
SScriptProcessor::getWidth() const
{
  return m_width;
}

/*!
 * \brief Fill m_out from m_in by getting glyphs indeces from 
 *      the font.
 * \return false if at least one glyph is not found.
 *   in all circumstances, the m_out will contain
 *   a glyph for each element in m_in.
 */
bool
SScriptProcessor::gindex()
{
  m_out.clear();
  bool ret = true;
  SS_GlyphIndex index;
  for (unsigned i=0; i<m_in.size(); i++)
  {
    index = m_font->gindex (m_in[i]);
    //
    // allow ZWJ ZWNJ even though they are missing from font.
    //
    if (index == 0)
    {
      if (m_in[i] != 0x200c && m_in[i] != 0x200d)
      {
        ret = false;
      }
    }
    m_out.append (index);
  }
  return ret;
}
/*!
 * \brief mark to base positioning OTF feature. 
 *  gposInit must be called before this.
 * When all gpos sequences are done gposFinal must
 * be called.
 *  This manipulates the m_out array.
 */
void
SScriptProcessor::gpos (const char* feature)
{
  int x;
  int y;
  SS_GlyphIndex  g[2];
  for (unsigned int i=0; i+1<m_out.size(); i++)
  {
    unsigned int pos_base_index = i;
    // nothing can be relative to a halant
    if (i>0 && m_out_type[i] == SRendClass::Halant)
    {
      if (i>1 && m_out_type[i-1] == SRendClass::Nukta)
      {
         pos_base_index = i-2; // should be a consonant.
      }
      else
      {
         pos_base_index = i-1; // should be a consonant.
      }
    }
    const SS_GlyphIndex* gi = m_out.array();
    g[0] = gi[pos_base_index];
    g[1] = gi[i+1];
    if (m_font->gpos (m_otfScript, feature, g, &x, &y))
    {
      m_xpos.replace (i+1, x);
      m_ypos.replace (i+1, y);
      m_pos_base_index.replace (i+1, pos_base_index) ;
      continue;
    }

    if (strcmp (feature, "abvm")==0)
    {
      // Try again to attach it to base.
      while (pos_base_index > 0 
         && m_out_type[pos_base_index] == SRendClass::Mbelow 
         && m_out_type[pos_base_index] != SRendClass::Cbase 
         && m_out_type[pos_base_index] != SRendClass::Chbase
         && m_out[pos_base_index] != m_out[i+1]) 
      {
         pos_base_index--;
      }
      if (m_out_type[pos_base_index] != SRendClass::Cbase 
         && m_out_type[pos_base_index] != SRendClass::Chbase) 
      {
         continue;
      }
      g[0] = gi[pos_base_index];
      if (m_font->gpos (m_otfScript, feature, g, &x, &y))
      {
        m_xpos.replace (i+1, x);
        m_ypos.replace (i+1, y);
        m_pos_base_index.replace (i+1, pos_base_index) ;
        continue;
      }
    }

#if DO_FIX_BROKEN_BELOW

    if (strcmp (feature, "blwm")!=0) continue;

    // Try again to attach it to base.
    while (pos_base_index > 0 
       && m_out_type[pos_base_index] != SRendClass::Cbase 
       && m_out_type[pos_base_index] != SRendClass::Chbase
       && m_out[pos_base_index] != m_out[i+1]) 
    {
       pos_base_index--;
    }

    if (m_out_type[pos_base_index] != SRendClass::Cbase 
       && m_out_type[pos_base_index] != SRendClass::Chbase) 
    {
       continue;
    }
    g[0] = gi[pos_base_index];
    if (m_font->gpos (m_otfScript, feature, g, &x, &y))
    {
      m_xpos.replace (i+1, x);
      m_ypos.replace (i+1, y);
      m_pos_base_index.replace (i+1, pos_base_index) ;
      continue;
    }

    if (m_font->getGlyphClass (m_out[i+1]) != 3) continue;

    // Select the ones that for sure should be below
    if (m_out[i+1] != m_font->gindex (0x0c56)) continue;

    if (m_out_type[i+1] == SRendClass::Mbelow
        && m_xpos[i+1] == 0 && m_ypos[i+1] == 0)
    {
      int w = m_font->gwidth (m_out[i+1]); 
      // Should be below - well, it is not.
      if (w > 0)
      {
        m_xpos.replace (i+1, w);
        m_ypos.replace (i+1, 0);
        m_pos_base_index.replace (i+1, pos_base_index);
        debugDeltaPos ("Manual hack SRendClass::Mbelow.");
      }
    }
#endif
  }
}

/*!
 * \return the position after the base consonant.
 * Move backwards and find the first consonant that does not
 * have a below-base or post base form, or arrive at first consonant.
 *
 * Initialize m_out_type and add addtionional marks for reordering:
 *
 *   Creph   Non-base consonant clas in case m_script_has_reph 
 *           is set and it starts with m_ra and syllable does not have 
 *           an Mpost.
 *
 *   CMreph   Non-base consonant clas in case m_script_has_reph 
 *           is set and it starts with m_ra and syllable has an 
 *           Mpost.
 *
 *   Cra     Non-base consonant class in case m_script_has_reph 
 *           is not set and it starts with m_ra and syllable does 
*            not have an Mpost.
 *
 *   CMra     Non-base consonant class in case m_script_has_reph 
 *           is not set and it starts with m_ra and syllable has 
 *           an Mpost.
 *
 *   Cpost   After the first Cpost, all Cbelow consonants
 *           should be marked Cpost after base.
 *
 *   Cpre    Consonants that are not at the beginning of the
 *           syllable and they are before the base consonant
 *           should be marked Cpre.
 *
 *   Cbase   The base consonant if it can not be in half-form.
 *
 *   At this moment, in addition to the inherent properties, 
 *   these are the only ones that can appear in re-ordering-guides.
 */
unsigned int
SScriptProcessor::findBaseConsonant()
{
  if (m_in.size()==0) return 0;
  unsigned int lastConsonant = 0;

  /*
   * Get first type and  add 
   *    << Creph Cra 
   */
  SRendClass::RType typeFirst; 
  if (m_in[0] == m_ra)
  {
    typeFirst = (m_script_has_reph) ? SRendClass::Creph : SRendClass::Cra;
    if (m_in.size() > 1 && SRendClass::get(m_in[1]) != SRendClass::Halant)
    {
      typeFirst = SRendClass::get (m_in[0]);
    }
    lastConsonant = 1;
  }
  else
  {
    typeFirst = SRendClass::get (m_in[0]);
    if (typeFirst == SRendClass::Cbase 
      || typeFirst == SRendClass::Cpost
      || typeFirst == SRendClass::Cdead
      || typeFirst == SRendClass::Cbelow)
    {
      lastConsonant = 1;
    }
  }

  // Khanda Ta Treat it just like Ta
  if (typeFirst == SRendClass::Cdead) 
  {
    typeFirst = SRendClass::Cbase;
  }
  m_out_type.clear();
  m_out_type.append (typeFirst);

  /*
   * Test VO 
   * In Bengali if there is VMabove after Mpost change it to 
   *      VMpost.
   *   << Cra
   */
  bool has_vo = false;
  bool has_mpost = false;
  unsigned int i;
  // Initialize m_out_type.
  for (i=1; i<m_in.size(); i++)
  {
    SRendClass::RType type = SRendClass::get (m_in[i]);
    // Khanda Ta Treat it just like Ta
    if (type == SRendClass::Cdead) 
    {
      type = SRendClass::Cbase;
    }

    // Creph and Cra can be only the first consonant 
    // This cycle starts with 1.
    if (type == SRendClass::Cbase 
       || type == SRendClass::Cpost 
       || type == SRendClass::Cbelow
       )
    {
      lastConsonant  = i+1;
    }
    if (type == SRendClass::VO)
    {
      has_vo = true;
    }
    if (type == SRendClass::Mpost)
    {
      has_mpost = true;
    }
    // This change of attribute is needed for proper rendering of
    // candrabindu in Bengali.
    if (type == SRendClass::VMabove && has_mpost 
       && m_script == SC_BENGALI)
    {
      type = SRendClass::VMpost;
    }
    m_out_type.append (type);
  }

  if (lastConsonant==0) return 0;

  //
  // At this point we can have Cbase, Cpost, Cbelow, Cra.
  // Try to figure where the Cbase or Chbase will be. 
  // 
  i= m_in.size();
  unsigned int count=0;
  unsigned int base = 0;
  unsigned int last = 0;

  // Is this a ZWJ base?
  while (i-- > base)
  {
    SRendClass::RType ct = m_out_type[i];
    switch (ct)
    {
    case SRendClass::Cbase:
    case SRendClass::Cra:
      // This formation is not allowed to be base, unless last.
      if (last>i && m_out_type[i+1] == SRendClass::Halant 
         && m_out_type[i+2] == SRendClass::ZWJ)
      {
         base = last;
         break;
      }
      base = i;
      break;
    case SRendClass::Creph:
      if (last == 0)
      {
        base = i;
      }
      else
      {
        base = last;
      }
      break;
    case SRendClass::Cpost:
    case SRendClass::Cbelow:
      // This formation is not allowed to be base, unless last.
      if (last>i && m_out_type[i+1] == SRendClass::Halant 
         && m_out_type[i+2] == SRendClass::ZWJ)
      {
         base = last;
         break;
      }
      count++;
      if (count==3 && (m_script == SC_TELUGU || m_script == SC_KANNADA))
      {
        base = i;
      }
      last = i;
      break;
    default: break;
    }
  }
#if DEBUG_ALL
  fprintf (stderr, "find-base: base=%u last=%u\n", 
      base+1, lastConsonant); 
#endif 

  /*
   * Modify first type if necassary (Creph)
   */
  // SGC IOS
  if (typeFirst == SRendClass::Cra || typeFirst == SRendClass::Creph)
  {
    if (has_vo && m_script_has_reph)
    {
      m_out_type.replace (0, SRendClass::Creph);
     // preventing engine taking reph away from me.
      if (base == 0) base = m_out_type.size()-1;
    }
    if (m_out_type.size() > 2 
       && m_out_type[1] ==  SRendClass::Halant
       && m_out_type[2] ==  SRendClass::ZWJ)
    {
      m_out_type.replace (0, SRendClass::Cpre);
    }
  }

  // Makes sure everything is ordered properly.
  bool post = false; // everything will be ppst after a post.

  for (i=0; i<m_out_type.size(); i++)
  {
    SRendClass::RType type = m_out_type[i];
    switch (type)
    {
    case SRendClass::Mpre:
      break;
    case SRendClass::Creph:
    case SRendClass::Cra:
      if (i==base)
      {
        m_out_type.replace (i, SRendClass::Cbase);
      }
      break;
    case SRendClass::Cbase:
    case SRendClass::Cbelow:
    case SRendClass::Cpost:
    case SRendClass::Cpre:
      if (i==base)
      {
        m_out_type.replace (i, SRendClass::Cbase);
      }
      if (i<base)
      {
        m_out_type.replace (i, SRendClass::Cpre);
      }
      if (i>base)
      {
        m_out_type.replace (i, SRendClass::Cpre);
        post = (post || (type != SRendClass::Cbelow));
        if (post)
        {
          m_out_type.replace (i,SRendClass::Cpost);
        }
        else
        {
          m_out_type.replace (i, type);
        }
      }
      break;
    default: break;
    }
  }
  // << CMra, CMreph
  if (has_mpost && m_out_type[0] == SRendClass::Cra)
  {
    m_out_type.replace (0, SRendClass::CMra);
  }
  if (has_mpost && m_out_type[0] == SRendClass::Creph)
  {
    m_out_type.replace (0, SRendClass::CMreph);
  }
  // Yaphala should use ZWNJ.
#if 0
  if (m_script == SC_BENGALI &&  m_is_begin
    &&  m_in.size() > 2
    &&  m_in[0] == 0x09b0
    &&  m_in[1] == 0x09cd
    &&  m_in[2] == 0x09af
    )
  {
    if (m_out_type[2] == SRendClass::Cbase)
    {
      m_out_type.replace (0, SRendClass::Cbase);
      m_out_type.replace (2, SRendClass::Cbelow);
      base = 0;
    }
    else
    {
      m_out_type.replace (0, SRendClass::Cpre);
    }
    debugInOut ("reorder-rya-init");
  }
#endif
  return base+1;
}

/*!
 * \return the position after the last consonant.
 *  This manipulates the m_in array.
 */
unsigned int
SScriptProcessor::findLastConsonant()
{
  if (m_in.size()==0) return 0;

  unsigned int i=m_in.size();
  while (i-- > 0)
  {
    SRendClass::RType ct = SRendClass::get (m_in[i]);
    if (ct==SRendClass::Cbase || ct==SRendClass::Cpost || ct==SRendClass::Cbelow)
    {
      return i+1;
    }
  }
  return 0;
}

/*!
 * \brief Manipulate the m_in array.
 * \param from is the first index that will get removed.
 * \param until is the index before last that will get removed.
 * \param with is the replacement array (it can overlap)
 * \param size is the array size.
 *  This manipulates the m_in array.
 */
void
SScriptProcessor::replaceIn (unsigned int from, unsigned int until, 
         const SS_UCS4* with, unsigned int size)
{
  unsigned int i=0;

  /* remove */
  for (i=from; i<until; i++) m_in.remove (from);
  /* append */
  for (i=0; i<size; i++) m_in.insert (from+i, with[i]);
}

/*!
 * \brief Manipulate the m_out array.
 * \param from is the first index that will get removed.
 * \param until is the index before last that will get removed.
 * \param with are the replacement glyphs.
 * \param size is the replacement size.
 *  This manipulates the m_out array.
 */
void
SScriptProcessor::replaceOut (unsigned int from, unsigned int until, 
         SS_GlyphIndex* with, unsigned int size)
{
  unsigned int i=0;

  /* remove */
  for (i=from; i<until; i++) m_out.remove (from);
  /* append */
  for (i=0; i<size; i++) m_out.insert (from+i, with[i]);
}

/*!
 * Reorder the m_in array and build m_out_type array.
 */
void
SScriptProcessor::reorder ()
{
  switch (m_script)
  {
  case SC_DEVANAGARI:
  case SC_BENGALI:
  case SC_GURMUKHI:
  case SC_GUJARATI:
  case SC_ORIYA:
  case SC_TAMIL:
  case SC_TELUGU:
  case SC_KANNADA:
  case SC_MALAYALAM:
  case SC_SINHALA:
     reorderIndic();
     break;
  case SC_THAI:
  case SC_LAO:
  case SC_TIBETAN:
  case SC_JAMO:
  default:
     reorderStraight();
  }
}

/*!
 * \brief create a straight ordering in m_in, mark all
 *   characters as SRendClass::Any in m_out_type.
 */
void
SScriptProcessor::reorderStraight()
{
  m_out_type.clear();
  for (unsigned int i=0; i<m_in.size(); i++)
  {
    SRendClass::RType type = (m_script == SC_TIBETAN
      || m_script == SC_LAO || m_script == SC_THAI) 
      ? SRendClass::Any : SRendClass::get (m_in[i]);
    m_out_type.append (type);
  }
}

/*!
 * \brief reorder the glyphs in the buffer.
 *
 * Before re-ordering, makes sure SScriptProcessor::decompose
 * has been called and the Vowels has been decomposed into their
 * parts. 
 *
 * What this routine does is the following:
 *
 * 1. Find the last consonant.
 *
 * 2. Find the base consonant. This step also creates reordering
 *    properties.
 *
 * 3. If base consonant is not the last one, move the halant from
 *    the base consonant to the last one.
 *
 * 4. Use the guide to reorder the codepoints.
 *
 * Add these additional properties for rendering:
 *
 *   Cfirst  The first consonant before base if it is not m_ra.
 *
 *   Clast   The last consonant after base
 *
 *   Chbase  The base consonant if it can be in half-form.
 *
 * In all cases m_out_type should mirror the glyphs in m_in
 * Vector and filled in with sane values for rendering, before 
 * the method returns, 
 *
 * This manipulates the m_in array.
 */
void
SScriptProcessor::reorderIndic ()
{
  // 1. Find last consonant. Return value is last+1
  unsigned int last_c = findLastConsonant();
  unsigned int i;
  
  // 2. Find base consonant. Return value is base+1
  unsigned int base = findBaseConsonant();

  debugInOut ("reorder-init");


  // 3. Move halant from base to last_c manually,
  if (base < last_c  && m_out_type[base]==SRendClass::Halant)
  {
    bool goahead = true;
    // If it has a ZWJ forget it.
    for (i=last_c; i<m_out_type.size(); i++)
    {
      if (m_out_type[i] == SRendClass::ZWJ) goahead = false;
    }
    if (goahead) 
    {
      m_in.insert (last_c, m_in[base]);
      m_in.remove (base);

      m_out_type.insert (last_c, SRendClass::Halant);
      m_out_type.remove (base);
      debugInOut ("reorder-moved-halant");
    }
  } 

#if DEBUG_ALL
  if (m_reorder_guide.size()==0)
  {
    fprintf (stderr, "ERROR: m_reorder_guide is not initialized!");
  }
  else
  {
   // fprintf (stderr, "rereorder-guide: %d items.\n", m_reorder_guide.size());
  }
#endif
  
  // we will use pp and m_out_type. copy out_type in in_type.
  SBinVector<SRendClass::RType> in_type = m_out_type;
  m_out_type.clear();
  SV_UCS4 pp;

  // 4. Use the m_reorder_guide to reorder the codepoints.
  // Update:
  //    Chbase 
  //    Cfirst 
  unsigned int chpos=0;
  unsigned int cpos;
  bool first = true;
  unsigned int c_last_pos=0; 
  for (unsigned int gindex=0; gindex < m_reorder_guide.size(); gindex++)
  {
    SRendClass::RType gclass = m_reorder_guide[gindex];

    for (i=0;i<m_in.size(); i++)
    {
      if (gclass != in_type[i])
      {
         continue;
      }

      if (m_in[i] == 0)
      {
         fprintf (stderr, "reorder: attempt to process %s twice!\n",
                  SRendClass::string(gclass));
         continue;
      }
      pp.append (m_in[i]);
      m_out_type.append (in_type[i]);
      cpos = m_out_type.size()-1;


      if (m_out_type[cpos] == SRendClass::Cbelow
         || m_out_type[cpos] == SRendClass::Cpost)
      {
         if (i>c_last_pos) c_last_pos = cpos;
      }

      if (m_out_type[cpos] == SRendClass::Cbase)
      {
        chpos = m_out_type.size();
      }

      if (first && m_out_type[cpos] == SRendClass::Cpre)
      {
        m_out_type.replace (cpos, SRendClass::Cfirst);
        first = false;
      }

      // Eat up Yaphala - halant comes before ya.
      if (i>0 && m_in[i] == 0x09af && m_in[i-1] == 0x09cd
        && (i+1==m_in.size() || m_in[i+1] != 0x09af))
      {
        if (i!=2 || m_in[i-2] != m_ra)
        {
          pp.append (m_in[i-1]);
          m_out_type.append (in_type[i-1]);
          m_in.replace (i-1, 0);
        }
      }
      m_in.replace (i, 0);
      while (i+1 < m_in.size() &&
           ( in_type[i+1] == SRendClass::Halant
          || in_type[i+1] == SRendClass::Nukta
          || in_type[i+1] == SRendClass::ZWJ))
      {
        i++;
        pp.append (m_in[i]);
        m_out_type.append (in_type[i]);
        m_in.replace (i, 0);
        if (in_type[i] == SRendClass::ZWJ)
        {
          if (chpos)
          {
            m_out_type.replace (chpos-1, SRendClass::Chbase);
          }
        }
      }
#if DEBUG_ALL
      SString str ("reorder-add-");
      str.append (SRendClass::string(gclass));
      str.append ((char)0);
      debugArray (str.array(), pp.array(), pp.size());
#endif
    }
  }

  // Malayalam needs this.
  if (c_last_pos != 0)
  {
      m_out_type.replace (c_last_pos, SRendClass::Clast);
  }

  // Add additional properties for rendering.

  unsigned int ucount =0;
  for (i=0; i< m_in.size(); i++)
  {
    if (SRendClass::get (m_in[i]) == SRendClass::ZWNJ)
    {
      m_in.replace (i, 0);
    }
    if (m_in[i] != 0)
    {
#if DEBUG_ALL
      fprintf (stderr, "[ERROR] reorder-unprocesed: %s-U+%04X.\n", 
         SRendClass::string (SRendClass::get(m_in[i])), m_in[i]);
#endif
      ucount++;
      pp.append (m_in[i]);
      m_out_type.append (in_type[i]);
    }
  }
#if DEBUG_ALL
  if (ucount)
  {
    debugArray ("reorder-unprocessed", m_in.array(), m_in.size());
  }
#endif

  // Copy back out pp, and  list.
  m_in = pp;

  /* make sure m_in and m_out is in sync */
  if (m_in.size() != m_out_type.size()) 
  {
#if DEBUG_ALL
     fprintf (stderr, "[ERROR] reorder-error: m_in.size=%u m_out_type_size=%u\n",
       m_in.size(), m_out_type.size());
#endif
      m_out_type.clear();
      m_in.clear();
  }
}


/*!
 * \brief Apply positioning and reordering features on a
 *       buffer that was accepted with 'put'.
 * This also stores the key.
 */
void
SScriptProcessor::apply ()
{
  unsigned int i;
  SStringVector gsub_guide;
  SStringVector gpos_guide;

  m_in = m_orig;
  m_out.clear();
  m_out_type.clear();

  switch (m_script)
  {
  case SC_NONE:
    break;
  case SC_DEVANAGARI:
    gsub_guide = SStringVector("nukt,akhn,rphf,blwf,half,vatu,pres,abvs,blws,psts,haln");
    gpos_guide = SStringVector("abvm,blwm,dist");
    break;
  case SC_BENGALI:
    // FIXME: If I put 'init' in from ani.ttf substitutes ka, ra with 
    // something the font can not deal with in subsequent gsub operations.
    if (l_isInit==1)
    {
      gsub_guide = SStringVector("init,nukt,akhn,rphf,blwf,half,pstf,vatu,pres,abvs,blws,psts");
    }
    else
    {
      gsub_guide = SStringVector("nukt,akhn,rphf,blwf,half,pstf,vatu,pres,abvs,blws,psts");
    }
    gpos_guide = SStringVector("abvm,blwm,dist");
    break;
  case SC_GURMUKHI:
    gsub_guide = SStringVector("nukt,blwf,pstf,vatu,pres,abvs,blws,psts,haln");
    gpos_guide = SStringVector("abvm,blwm,dist");
    break;
  case SC_GUJARATI:
    gsub_guide = SStringVector("nukt,akhn,rphf,blwf,half,pstf,vatu,pres,abvs,blws,psts,haln");
    gpos_guide = SStringVector("abvm,blwm,dist");
    break;
  // Go with the generic recipe
  case SC_ORIYA:
  case SC_KANNADA:
  case SC_MALAYALAM:
    gsub_guide = SStringVector("nukt,akhn,rphf,blwf,half,pstf,vatu,pres,blws,abvs,psts,haln");
    gpos_guide = SStringVector("abvm,blwm,dist");
    break;
  case SC_TAMIL:
    gsub_guide = SStringVector("akhn,half,pres,abvs,blws,psts,haln");
    gpos_guide = SStringVector("abvm,blwm,dist");
    break;
  case SC_TELUGU:
    gsub_guide = SStringVector("akhn,blwf,abvs,blws,psts,haln");
    gpos_guide = SStringVector("abvm,blwm,dist");
    break;
  case SC_THAI:
    gsub_guide = SStringVector("ccmp");
    gpos_guide = SStringVector("kern,mark,mkmk");
    break;
  case SC_LAO:
    gsub_guide = SStringVector("ccmp,blws,abvs");
    gpos_guide = SStringVector("kern,mark,mkmk");
    break;
  case SC_TIBETAN:
    gsub_guide = SStringVector("ccmp,blws,abvs");
    gpos_guide = SStringVector("blwm,abvm,kern");
    break;
  case SC_JAMO:
    gsub_guide = SStringVector("ccmp,ljmo,vjmo,tjmo");
    gpos_guide = SStringVector("");
  default:
  case SC_MAX:
    break;
  }

  debugIn("in");

  // Decompose into parts
  decompose ();
  debugIn("decompose");

  /*
   * reorder m_in.
   */
  reorder (); //  m_in, m_out_type   
  debugInOut("reorder");

  /*
   * make raw m_out from m_in. Look up all glyph indeces.
   */
  gindex ();
  debugOut("gindex");

  /*
   * Do all gsubs
   */
  for (i=0; i<gsub_guide.size(); i++)
  {
    SString s = gsub_guide[i];
    if (s.size()!=4)
    {
#if DEBUG_ALL 
       fprintf (stderr, "BAD GSUB: %*.*s\n", SSARGS(s));
#endif
       continue;
    }
    s.append ((char)0);
    gsub (s.array());
    debugOut(s.array());
  }

  gsubclean ();
  debugOut("clean");

  /*
   * Do all gpos
   */
  gposInit(); 
  debugDeltaPos("init");

  for (i=0; i<gpos_guide.size(); i++)
  {
    SString s = gpos_guide[i];
    if (s.size()!=4)
    {
#if DEBUG_ALL 
       fprintf (stderr, "BAD GUIDE: %*.*s\n", SSARGS(s));
#endif
       continue;
    }
    s.append ((char)0);
    gpos (s.array());
    debugDeltaPos(s.array());
  }

  gposFinal(); 
  debugPos("final");
}

/*!
 * \brief Check if a substitution can be performed.
 * \param sub is the substitution type \copydoc GSubType.
 * \param feature is the otf feature.
 * \param len returns the minimum length.
 */
bool
SScriptProcessor::allowed (const char* feature, SRendClass::RType sub)
{

  switch (sub)
  {
  case SRendClass::Cfirst:
   if (strcmp (feature, "abvs")==0) return false; 
   if (strcmp (feature, "blwf")==0) return false; 
   if (strcmp (feature, "rphf")==0) return false; 
   if (strcmp (feature, "psts")==0) return false; 
   if (strcmp (feature, "pstf")==0) return false; 
   return true;

  case SRendClass::Cpre:
   if (strcmp (feature, "rphf")==0) return false; 
   if (strcmp (feature, "psts")==0) return false; 
   if (strcmp (feature, "pstf")==0) return false; 
   return true;

  case SRendClass::Chbase: 
   if (strcmp (feature, "vatu")==0) return false; 
   if (strcmp (feature, "blwf")==0) return false; 
   if (strcmp (feature, "rphf")==0) return false; 
   if (strcmp (feature, "abvs")==0) return false; 
   if (strcmp (feature, "pstf")==0) return false; 
   if (strcmp (feature, "psts")==0) return false; 
   if (strcmp (feature, "pres")==0) return false; 
   return true;

  case SRendClass::Cbase: 
   if (strcmp (feature, "vatu")==0) return false; 
   if (strcmp (feature, "half")==0) return false; 
   if (strcmp (feature, "blwf")==0) return false; 
   if (strcmp (feature, "rphf")==0) return false; 
   if (strcmp (feature, "abvs")==0) return false; 
   if (strcmp (feature, "pstf")==0) return false; 
   if (strcmp (feature, "psts")==0) return false; 
   if (strcmp (feature, "pres")==0) return false; 
   return true;

  case SRendClass::Cpost:
   if (strcmp (feature, "half")==0) return false; 
   if (strcmp (feature, "blwf")==0) return false; 
   if (strcmp (feature, "rphf")==0) return false; 
   if (strcmp (feature, "pres")==0) return false; 
   return true;

  case SRendClass::Clast:
   if (strcmp (feature, "half")==0) return false; 
   if (strcmp (feature, "rphf")==0) return false; 
   if (strcmp (feature, "pres")==0) return false; 
   return true;

  case SRendClass::Cbelow:
   if (strcmp (feature, "pres")==0) return false; 
   if (strcmp (feature, "abvs")==0) return false; 
   if (strcmp (feature, "rphf")==0) return false; 
   return true;

  case SRendClass::Cra:
  case SRendClass::CMra:
   if (strcmp (feature, "pres")==0) return false; 
   if (strcmp (feature, "rphf")==0) return false; 
   if (strcmp (feature, "haln")==0) return false; 
   if (strcmp (feature, "abvs")!=0) return false;
   return true;

  case SRendClass::Creph:
  case SRendClass::CMreph:
   if (strcmp (feature, "pres")==0) return false; 
   if (strcmp (feature, "rphf")!=0
     && strcmp (feature, "abvs")!=0) return false;
   return true;

  case SRendClass::None:
  case SRendClass::Halant:
  case SRendClass::Nukta:
  default:
    return true;
  }
  return false;
}

/*!
 * \brief show the unicode input array. 
 */
void
SScriptProcessor::debugIn (const char* message) const
{
#if DEBUG_ALL
  fprintf (stderr, "%s: ", message);
  for (unsigned int i=0; i<m_in.size(); i++)
  {
    fprintf (stderr, " %s-U+%04X", 
       SRendClass::string(SRendClass::get(m_in[i])), m_in[i]); 
  } 
  fprintf (stderr, "\n");
#endif
}


/*!
 * \brief show the glyph output array. 
 */
void
SScriptProcessor::debugOut (const char* message) const
{
#if DEBUG_ALL
  fprintf (stderr, "%s: ", message);
  for (unsigned int i=0; i<m_out.size(); i++)
  {
    fprintf (stderr, " %s-%04X", SRendClass::string(m_out_type[i]), m_out[i]); 
  } 
  fprintf (stderr, "\n");
#endif
}
/*!
 * \brief show the glyph input array with m_out_type 
 */
void
SScriptProcessor::debugInOut (const char* message) const
{
#if DEBUG_ALL
  fprintf (stderr, "%s: ", message);
  for (unsigned int i=0; i<m_in.size(); i++)
  {
    fprintf (stderr, " %s-%04X", SRendClass::string(m_out_type[i]), m_in[i]); 
  } 
  fprintf (stderr, "\n");
#endif
}
/*!
 * \brief show the glyph output array. 
 */
void
SScriptProcessor::debugDeltaPos (const char* message) const
{
#if DEBUG_ALL
  fprintf (stderr, "%s: ", message);
  for (unsigned int i=0; i<m_xpos.size(); i++)
  {
    if (i==0)
    {
      fprintf (stderr, " [base]:%d;%d", m_xpos[i], m_ypos[i]);
    }
    else
    {
      fprintf (stderr, " [%u]:%d;%d", 
         m_pos_base_index[i], m_xpos[i], m_ypos[i]);
    }
  } 
  fprintf (stderr, "\n");
#endif
}
/*!
 * \brief show the glyph output array. 
 */
void
SScriptProcessor::debugPos (const char* message) const
{
#if DEBUG_ALL
  fprintf (stderr, "%s:  width=%d", message, m_width);
  for (unsigned int i=0; i<m_positions.size(); i++)
  {
    int mxy = m_positions[i];
    int pix = mxy & 0xffff;
    if (pix > 0x7fff) pix -= 0x10000 ;

    int piy = (mxy >> 16) & 0xffff;
    if (piy > 0x7fff) piy -= 0x10000;
    fprintf (stderr, " [%d,%d]", pix, piy);
  } 
  fprintf (stderr, "\n");
#endif
}

#if DEBUG_ALL
static void
debugArray (const char* message, const SS_UCS4* arr, unsigned int size)
{
  fprintf (stderr, "%s: ", message);
  for (unsigned int i=0; i<size; i++)
  {
    fprintf (stderr, " G-%04X", arr[i]);
  } 
  fprintf (stderr, "\n");
}
#endif


