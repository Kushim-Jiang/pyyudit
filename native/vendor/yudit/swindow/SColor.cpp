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
 
#include "swindow/SColor.h"
#include "stoolkit/SBinHashtable.h"

#include "swindow/SColorDefs.i"
#include <stdlib.h>

/**
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-04-23
 */

SColor::SColor (void) :
    red (0xff),
    green (0xff),
    blue (0xff), 
    alpha (0xff)
{
}

/**
 * This is out color class
 */
SColor::SColor (SS_WORD32 value) :
    red ((value>>16)&0xff),
    green ((value>>8)&0xff),
    blue (value&0xff), 
    alpha ((value>>24)&0xff)
{
}
SColor::SColor (unsigned char _red, unsigned char _green, unsigned char _blue, unsigned char _alpha)
 : red(_red), green (_green), blue (_blue), alpha (_alpha)
{
}

SColor::SColor (double _red, double _green, double _blue, double _alpha)
 : red((unsigned char)(_red*255.9)),
   green ((unsigned char)(_green*255.9)),
   blue ((unsigned char) (_blue*255.9)), 
   alpha ((unsigned char)(_alpha*255.9))
{
}

SColor::SColor (const SColor& color)
  : red(color.red), green (color.green), blue (color.blue), 
    alpha (color.alpha)
{
}

SColor
SColor::operator= (const SColor& color)
{
  if (&color == this) return *this;
  red = color.red;
  green = color.green;
  blue = color.blue;
  alpha = color.alpha;
  return *this;
}

SColor::~SColor()
{
}

SS_WORD32
SColor::getValue() const
{
  return ((SS_WORD32) blue)
        + (((SS_WORD32) green) << 8) 
        + (((SS_WORD32) red) << 16) 
        + (((SS_WORD32) alpha) << 24); 
}

SBinHashtable<SS_WORD32> colorHashtable;
bool colorHashtableInited = false;

SS_WORD32
SColor::getNamedColor(const SString& _name)
{
  if (!colorHashtableInited)
  {
    colorHashtable.put ("white", 0xffffff);
    colorHashtable.put ("black", 0x000000);
    colorHashtable.put ("darkgray", 0xa9a9a9);
    colorHashtable.put ("gray", 0xbebebe);
    colorHashtable.put ("lightgray", 0xd3d3d3);
    colorHashtable.put ("red", 0xff0000);
    colorHashtable.put ("green", 0x00ff00);
    colorHashtable.put ("blue", 0x0000ff);
    colorHashtable.put ("cyan", 0x00ffff);
    colorHashtable.put ("magenta", 0xff00ff);
    colorHashtable.put ("yellow", 0xffff00);
    colorHashtable.put ("darkred", 0x8b0000);
    colorHashtable.put ("darkgreen", 0x006400);
    colorHashtable.put ("darkblue", 0x00008b);
    colorHashtable.put ("darkcyan", 0x008b8b);
    colorHashtable.put ("darkmagenta", 0x8b008b);
    colorHashtable.put ("darkyellow", 0x8b8b00);
    colorHashtable.put ("metal", 0xafb49f);
    for (unsigned int i=0; rgb_colors[i].name != 0; i++)
    {
       unsigned char r = ((unsigned char)(rgb_colors[i].red));
       unsigned char g = ((unsigned char)(rgb_colors[i].green));
       unsigned char b = ((unsigned char) (rgb_colors[i].blue)); 
       unsigned char a = 0xff;
       SS_WORD32 v =  ((SS_WORD32) b) + (((SS_WORD32) g) << 8) 
        + (((SS_WORD32) r) << 16) + (((SS_WORD32) a) << 24); 
       SString colorName (rgb_colors[i].name);
       colorName.lower();
       colorHashtable.put (colorName, v);
    }
    colorHashtableInited = 1;
  }
  SS_WORD32 ret = 0;
  SString name (_name);
  if (_name.size()==7 && _name[0] == '#')
  {
    name.append ((char)0);
    char * next;
    ret = (SS_WORD32)  strtoul (&(name.array()[1]),  &next, 16);
    ret |= 0xff000000;
  }
  else if (_name.size()==9 && _name[0] == '#')
  {
    name.append ((char)0);
    char * next;
    ret = (SS_WORD32)  strtoul (&(name.array()[1]),  &next, 16);
    SS_WORD32 a = (ret & 0xff) << 24;
    ret = ret >> 8;
    ret = ret & 0x00ffffff;
    ret |= a;
//fprintf (stderr, "%08x\n", ret);
  }
  else
  {
    name.lower ();
    ret =  colorHashtable.get (name);
  }
  return ret;
}
bool
SColor::operator==(const SColor& col) const
{
  return (red==col.red && green == col.green && blue == col.blue && 
         alpha == col.alpha);
}

bool
SColor::operator!=(const SColor& col) const
{
  return (red!=col.red || green != col.green || blue != col.blue || 
         alpha != col.alpha);
}

bool
SColor::isDark() 
{
    double luma = 0.2126 * (double) red + 0.7152 * (double) green + 0.0722 * (double) blue;
    return luma < 128.0;
}

/**
 * Blend the other color into this color, using alpha values
 * This function is virtual because you may need to reimplement this.
 */
void
SColor::blend (const SColor& color)
{
  int rA = (int) color.red;
  int gA = (int) color.green;
  int bA = (int) color.blue;
  int aA = (int) color.alpha;

  int rB = (int) red;
  int gB = (int) green;
  int bB = (int) blue;
  int aB = (int) alpha;

  // Alpha compositing a over b
  int a = aA + aB * (255-aA) / 255;
  // For alpha == 0, it does not matter what colors we have.
  if (a > 0) {
    red = (char) ((rA * aA + rB * aB * (255 - aA) / 255) / a);
    green = (char) ((gA * aA + gB * aB * (255 - aA) / 255) / a);
    blue = (char) ((bA * aA + bB * aB * (255 - aA) / 255) / a);
  }
  alpha = a;
}

SColor
SColor::lighter(double a) const
{
  SColor orig(*this);
  SColor ret(1.0, 1.0, 1.0, a);
  orig.blend (ret);
  return (SColor(orig));
}

SColor
SColor::darker(double a) const
{
  SColor orig(*this);
  SColor ret(0.0, 0.0, 0.0, a);
  orig.blend (ret);
  return (SColor((orig)));
}

/**
 * This is from SObject
 */
SObject*
SColor::clone () const
{
  SColor * c = new SColor(*this);
  CHECK_NEW (c);
  return c;
}

SColor::SColor (const SString& name, double _alpha)
{
  SS_WORD32 vle = getNamedColor (name);
  alpha = (unsigned char) (_alpha * 255.9);
  blue = (vle)&0xff;;
  green = (vle>>8)&0xff;
  red = (vle>>16)&0xff;
}

SColor::SColor (const SString& name)
{
  SS_WORD32 vle = getNamedColor (name);
  blue = (vle)&0xff;;
  green = (vle>>8)&0xff;
  red = (vle>>16)&0xff;
  alpha = (vle>>24)&0xff;
}

