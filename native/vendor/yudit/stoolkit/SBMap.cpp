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
 
//
// SBMap.cpp
// Tokyo 1999-12-04 Gaspar Sinai
// Map N bytes to N bytes

#include "stoolkit/SExcept.h"
#include "stoolkit/SString.h"
#include "stoolkit/STypes.h"

#include "stoolkit/SBMap.h"
#include "stoolkit/SEncoder.h"
#include <string.h>
#include <stdio.h>

#define  TO16(c) (((SS_WORD16)c[0]<<8) + (SS_WORD16)c[1])
#define  TO32(c) (((SS_WORD32)c[0]<<24) + ((SS_WORD32)c[1]<<16) + ((SS_WORD32)c[2]<<8) + (SS_WORD32)c[3])
#define  TO64(c) (((SS_WORD64)c[0]<<56) + ((SS_WORD64)c[1]<<48) + ((SS_WORD64)c[2]<<40) + ((SS_WORD64)c[3]<<32)  + ((SS_WORD64)c[4]<<24)  + ((SS_WORD64)c[5]<<16) + ((SS_WORD64)c[6]<<8) + (SS_WORD64)c[7])

#if defined LONG_LONG
#define VALID_WORD(a) ((a)==3 || (a)==2 || (a)==1 || (a)==0)
#else
#define VALID_WORD(a) ((a)==2 || (a)==1 || (a)==0)
#endif

/**
 * This should be exactly 16 bytes.
 */
static char magicNN[]="YUDIT-NtoN  1.0";
static char magicBM[]="YUDIT-UMAP  1.0";
static char dummyName[32]=
//01234567890123456789012345678901
 "Yudit umap file format         ";

/**
 * The last two values are bitmap OR ed to first few values
 */
typedef enum {
  FT_MAGIC=0, 
  FT_NAME,
  FT_SECTION,
  FT_SIZE_FROM,
  FT_SIZE_TO, 
  FT_LENGTH_FROM,
  FT_LENGTH_TO, 
  FT_IS_ENCODE,
  FT_MAP_TYPE,
  FT_COMMENT,
  FT_NATIVE_DATA,
  FT_UNIC_DATA,
  FT_IGNORE, 
  FT_MAX
}  SS_FieldType;


const char* textMapPrefix[FT_MAX+1]={
  "# ", 
  "#\n#-----------------------------------------\n#### THE NAME OF THIS MAP ###.\nNAME=", 
  "\n#--------------- NEW SECTION -------------\nSECTION=", 
  "\n# 0=8 bit, 1=16 bit, 2=32 bit and 3=64 bit - width of a key word\nKEY_WIDTH=", 
  "\n# 0=8 bit, 1=16 bit, 2=32 bit and 3=64 bit - width of a value word\nVALUE_WIDTH=", 
  "\n# 0=8 bit, 1=16 bit, 2=32 bit and 3=64 bit - this can hold the key length\nKEY_LENGTH=", 
  "\n# 0=8 bit, 1=16 bit, 2=32 bit and 3=64 bit - this can hold the value length\nVALUE_LENGTH=", 
  "\nENCODE=",
  "\n# 0=NONE 1=KEY MAP 2=FONT MAP\nTYPE=",
  "COMM=",
  "",
  "",
  "#",
  "",
};
const char* cMapPrefix[FT_MAX+1]={
  "/*--- Magic --*/\n", 
  "\n/*--- Map Name --*/\n", 
  "\n/*------------------New section---------------*/\n", 
  "\n/*---- Width 0=8 bit, 1=16 bit, 2=32 bit and 3=64 bit ---*/\n", 
  "\n/*---- Width 0=8 bit, 1=16 bit, 2=32 bit and 3=64 bit ---*/\n",
  "\n/*---- Width 0=8 bit, 1=16 bit, 2=32 bit and 3=64 bit ---*/\n", 
  "\n/*---- Width 0=8 bit, 1=16 bit, 2=32 bit and 3=64 bit ---*/\n",
  "\n/*--- ENCODE--*/\n",
  "\n/*--- TYPE ---*/\n",
  "\n/*--- COMM ---*/\n",
  "",
  "",
  "",
  "",
};

static int writeBytes(SString* fd, const unsigned char* in, int size);
static int write8 (SString* _fd, SS_WORD8 number, SFileFormat _format, const SS_FieldType ftype, const SS_LineEnd lend);
static int write16 (SString* fd, SS_WORD16 number, SFileFormat _format, const SS_FieldType ftype, const SS_LineEnd lend);
static int write32 (SString* fd, SS_WORD32 number, SFileFormat _format, const SS_FieldType ftype, const SS_LineEnd lend);
static int write1(SString* _fd, const unsigned char* out, int size, SFileFormat _format, const SS_FieldType ftype, const SS_LineEnd lend);

/**
 * SBMap can be used to map any N-word-long string into 
 * any n word long string. The supported word sizes are 1,2,4 and 8.
 * The array of words can be arbitrary long.
 */
SBMap::SBMap ()
{
  status = false;
  buffer =0;
  bufferSize = 0;
  maps = 0;
  mapSize = 0;

  name = 0;
  comment = 0;
  commentSize = 0;

  nameType = SBMap_ARRAY;
  commentType = SBMap_ARRAY;
  bufferType = SBMap_ARRAY;
}

/**
 * Set the map type, and initialize map, if not initialized.
 * @param _mapType is the type of map. 
 * <ul>
 *  <li> 0 - unknown </li>
 *  <li> 1 - character map </li>
 *  <li> 2 - key map </li>
 * </ul>
 */
void
SBMap::setType (int _mapType)
{
  mapType = _mapType;
  status = true;
}

/**
 * Set the name of the map and init.
 * @param  _name is a max 32-byte-long name
 */
void
SBMap::setName (const unsigned char* _name)
{
  nameType = SBMap_DYNAMIC;
  nameBuffer = new unsigned char [32];
  CHECK_NEW (nameBuffer);
  memset (nameBuffer, 0, 32);
  strncpy ((char*) nameBuffer, (const char*) _name, 31);
  name = nameBuffer;
  status = true;
}

/**
 * Set the comment.
 * @param _comment is a comment of arbitrary length.
 * @param _commentSize is the length of the comment.
 */
void
SBMap::setComment (const unsigned char* _comment, int _commentSize)
{

  commentType = SBMap_DYNAMIC;
  commentSize = (SS_WORD32) _commentSize;
  commentBuffer = new unsigned char [commentSize];
  CHECK_NEW (commentBuffer);
  memcpy (commentBuffer, _comment, commentSize);
  comment = commentBuffer;
  status = true;
}

SBMap::~SBMap()
{
  clear ();
}

/**
 * Add a new item to the SBMapBuffer. You can add an SBMapItem only if the 
 * SBMap has been properly initialized.
 * @param _item is the item to add. This will be owned by SBMap.
 * @param _position is the position of the SBMapItem, if it is less than 0 
 *      it will be added as a new item.
 */
bool
SBMap::add (SBMapItem* _item, int _position)
{
  if (status==false) return false;
  if (_position>=0 &&  _position<mapSize)
  {
    maps [_position] = _item;
    return true;
  }
  SBMapItem**  newMaps;

  newMaps = new SBMapItem* [mapSize+1];
  if (mapSize > 0)
  {
    memcpy (newMaps, maps, mapSize * sizeof (SBMapItem*));
    delete maps;
  }
  maps = newMaps;
  maps[mapSize++] = _item;
  return true;
}
/**
 * Create an SBMapBuffer to temporarily store data
 * @param _fromWordSize is the byte count of input data.
 * @param _toWordSize is the byte count of stored data.
 */
SBMapBuffer::SBMapBuffer (int _fromWordSize, int _toWordSize) 
  : fromWordSize (_fromWordSize), toWordSize (_toWordSize)
{
  bufferSize=2;
  u.u64 = new SS_WORD64 [1+((bufferSize<<toWordSize)>>2)];
  CHECK_NEW (u.u64);
  length = 0;
}

SBMapBuffer::~SBMapBuffer()
{
  delete  u.u64;
}

/**
 * Append an array of input and put it in the internal buffer.
 * @param buffer is a pointer to the input array of inWordSize bytes type
 * @param _length is the length of the input array of specified type
 */
void
SBMapBuffer::append(const void *buffer, int _length)
{
  int   i;
  const SS_WORD8* i8;
  const SS_WORD16* i16;
  const SS_WORD32* i32;
  const SS_WORD64* i64;

  ensureCapacity (_length + length);


  switch (fromWordSize)
  {
  case 0: 
    i8 = (SS_WORD8*) buffer;
    for (i=0; i<_length; i++)
    {
      switch (toWordSize)
      {
      case 0: u.u8[length+i] = (SS_WORD8) i8[i];
        break;
      case 1: u.u16[length+i] = (SS_WORD16) i8[i];
        break;
      case 2: u.u32[length+i] = (SS_WORD32) i8[i];
        break;
      case 3: u.u64[length+i] = (SS_WORD64) i8[i];
        break;
      }
    }
    break;
  case 1:
    i16 = (SS_WORD16*) buffer;
    for (i=0; i<_length; i++)
    {
      switch (toWordSize)
      {
      case 0: u.u8[length+i] = (SS_WORD8) i16[i];
        break;
      case 1: u.u16[length+i] = (SS_WORD16) i16[i];
        break;
      case 2: u.u32[length+i] = (SS_WORD32) i16[i];
        break;
      case 3: u.u64[length+i] = (SS_WORD64) i16[i];
        break;
      }
    }
    break;
  case 2:
    i32 = (SS_WORD32*) buffer;
    for (i=0; i<_length; i++)
    {
      switch (toWordSize)
      {
      case 0: u.u8[length+i] = (SS_WORD8) i32[i];
        break;
      case 1: u.u16[length+i] = (SS_WORD16) i32[i];
        break;
      case 2: u.u32[length+i] = (SS_WORD32) i32[i];
        break;
      case 3: u.u64[length+i] = (SS_WORD64) i32[i];
        break;
      }
    }
    break;
  case 3:
    i64 = (SS_WORD64*) buffer;
    for (i=0; i<_length; i++)
    {
      switch (toWordSize)
      {
      case 0: u.u8[length+i] = (SS_WORD8) i64[i];
        break;
      case 1: u.u16[length+i] = (SS_WORD16) i64[i];
        break;
      case 2: u.u32[length+i] = (SS_WORD32) i64[i];
        break;
      case 3: u.u64[length+i] = (SS_WORD64) i64[i];
        break;
      }
    }
    break;
  }
  length+=_length;

}

/**
 * shift array items in buffer.
 * @param _size  is the length of typed shift.
 */
void
SBMapBuffer::shift (int _size)
{
  int  i;

  switch (toWordSize)
  {
  case 0: for (i=0; i<length-_size; i++) u.u8[i] = u.u8[i+_size]; break;
  case 1: for (i=0; i<length-_size; i++) u.u16[i] = u.u16[i+_size]; break;
  case 2: for (i=0; i<length-_size; i++) u.u32[i] = u.u32[i+_size]; break;
  case 3: for (i=0; i<length-_size; i++) u.u64[i] = u.u64[i+_size]; break;
  }
  if (_size>0) length -= _size;
}


/**
 * Append an array of input and put it in the internal buffer.
 * @param buffer is a pointer to the input array of inWordSize bytes type
 *        This is in native order.
 * @param _length is the length of the input array of specified type
 */
void
SBMapBuffer::appendToNet(const void *buffer, int _length)
{
  int   i;
  const SS_WORD8* i8;
  const SS_WORD16* i16;
  const SS_WORD32* i32;
  const SS_WORD64* i64;
  int    ndx;

  ensureCapacity (_length + length);

  switch (fromWordSize)
  {
  case 0: 
    i8 = (SS_WORD8*) buffer;
    for (i=0; i<_length; i++)
    {
      ndx = (length+i) << toWordSize;
      switch (toWordSize)
      {
      case 0: u.u8[ndx] = i8[i];
        break;
      case 1: u.u8[ndx] = 0;
        u.u8[ndx+1] =  i8[i];
        break;
      case 2: u.u8[ndx] = 0;
        u.u8[ndx+1] = 0;
        u.u8[ndx+2] = 0;
        u.u8[ndx+3] =  i8[i];
        break;
      case 3: u.u8[ndx] = 0;
        u.u8[ndx+1] = 0;
        u.u8[ndx+2] = 0;
        u.u8[ndx+3] = 0;
        u.u8[ndx+4] = 0;
        u.u8[ndx+5] = 0;
        u.u8[ndx+6] = 0;
        u.u8[ndx+7] =  i8[i];
        break;
      }
    }
    break;
  case 1:
    i16 = (SS_WORD16*) buffer;
    for (i=0; i<_length; i++)
    {
      ndx = (length+i) << toWordSize;
      switch (toWordSize)
      {
      case 0: u.u8[ndx] = i16[i]&0xff;
        break;
      case 1: u.u8[ndx] = (i16[i]>>8)&0xff;
        u.u8[ndx+1] = i16[i]&0xff;
        break;
      case 2: u.u8[ndx] = 0;
        u.u8[ndx+1] = 0;
        u.u8[ndx+2] = (i16[i]>>8)&0xff;
        u.u8[ndx+3] = i16[i] &0xff;
        break;
      case 3: u.u8[ndx] = 0;
        u.u8[ndx+1] = 0;
        u.u8[ndx+2] = 0;
        u.u8[ndx+3] = 0;
        u.u8[ndx+4] = 0;
        u.u8[ndx+5] = 0;
        u.u8[ndx+6] = (i16[i]>>8)&0xff;
        u.u8[ndx+7] = i16[i] &0xff;
        break;
      }
    }
    break;
  case 2:
    i32 = (SS_WORD32*) buffer;
    for (i=0; i<_length; i++)
    {
      ndx = (length+i) << toWordSize;
      switch (toWordSize)
      {
      case 0: u.u8[ndx] = i32[i]&0xff;
        break;
      case 1: u.u8[ndx] = (i32[i]>>8) & 0xff;
        u.u8[ndx+1] = i32[i]&0xff;
        break;
      case 2: u.u8[ndx] = (i32[i]>>24) & 0xff;
        u.u8[ndx+1] = (i32[i]>>16) & 0xff;
        u.u8[ndx+2] = (i32[i]>>8) & 0xff;
        u.u8[ndx+3] = i32[i] &0xff;
        break;
      case 3: u.u8[ndx] = 0;
        u.u8[ndx+1] = 0;
        u.u8[ndx+2] = 0;
        u.u8[ndx+3] = 0;
        u.u8[ndx+4] = (i32[i]>>24) & 0xff;
        u.u8[ndx+5] = (i32[i]>>16) & 0xff;
        u.u8[ndx+6] = (i32[i]>>8) & 0xff;
        u.u8[ndx+7] = i32[i] &0xff;
        break;
      }
    }
    break;
  case 3:
    i64 = (SS_WORD64*) buffer;
    for (i=0; i<_length; i++)
    {
      ndx = (length+i) << toWordSize;
      switch (toWordSize)
      {
      case 0: u.u8[ndx] = i64[i]&0xff;
        break;
      case 1: u.u8[ndx] = (i64[i]>>8) & 0xff;
        u.u8[ndx+1] = i64[i]&0xff;
        break;
      case 2: u.u8[ndx] = (i64[i]>>24) & 0xff;
        u.u8[ndx+1] = (i64[i]>>16) & 0xff;
        u.u8[ndx+2] = (i64[i]>>8) & 0xff;
        u.u8[ndx+3] = i64[i] &0xff;
        break;
      case 3: u.u8[ndx] = (i64[i]>>56) & 0xf;
        u.u8[ndx+1] = (i64[i]>>48) & 0xf;
        u.u8[ndx+2] = (i64[i]>>40) & 0xf;
        u.u8[ndx+3] = (i64[i]>>32) & 0xf;
        u.u8[ndx+4] = (i64[i]>>24) & 0xff;
        u.u8[ndx+5] = (i64[i]>>16) & 0xff;
        u.u8[ndx+6] = (i64[i]>>8) & 0xff;
        u.u8[ndx+7] = i64[i] &0xff;
        break;
      }
    }
    break;
  }
  length+=_length;

}

/**
 * Append an array of input of net order to the internal buffer.
 * @param buffer is a pointer to the input array of inWordSize bytes type
 *     This is in Net order.
 * @param _length is the length of the input array of specified type
 */
void
SBMapBuffer::appendFromNet(const void *buff, int _length)
{
  int   i;
  const unsigned char* buffer=(const unsigned  char*)buff;
  const unsigned char *p;
  SS_WORD8  i8;
  SS_WORD16 i16;
  SS_WORD32 i32;
  SS_WORD64 i64;

  ensureCapacity (_length + length);

  switch (fromWordSize)
  {
  case 0: 
    for (i=0; i<_length; i++)
    {
      i8 = buffer[i];
      switch (toWordSize)
      {
      case 0: u.u8[length+i] = (SS_WORD8) i8;
        break;
      case 1: u.u16[length+i] = (SS_WORD16) i8;
        break;
      case 2: u.u32[length+i] = (SS_WORD32) i8;
        break;
      case 3: u.u64[length+i] = (SS_WORD64) i8;
        break;
      }
    }
    length+=_length;
    break;
  case 1:
    for (i=0; i<_length; i++)
    {
      p = &buffer[i<<1];
      i16 = (SS_WORD16) TO16(p);
      switch (toWordSize)
      {
      case 0: u.u8[length+i] = (SS_WORD8) i16;
        break;
      case 1: u.u16[length+i] = (SS_WORD16) i16;
        break;
      case 2: u.u32[length+i] = (SS_WORD32) i16;
        break;
      case 3: u.u64[length+i] = (SS_WORD64) i16;
        break;
      }
    }
    length+=_length;
    break;
  case 2:
    for (i=0; i<_length; i++)
    {
      p = &buffer[i<<2];
      i32 = (SS_WORD32) TO32(p);
      switch (toWordSize)
      {
      case 0: u.u8[length+i] = (SS_WORD8) i32;
        break;
      case 1: u.u16[length+i] = (SS_WORD16) i32;
        break;
      case 2: u.u32[length+i] = (SS_WORD32) i32;
        break;
      case 3: u.u64[length+i] = (SS_WORD64) i32;
        break;
      }
    }
    length+=_length;
    break;
  case 3:
    for (i=0; i<_length; i++)
    {
      p = &buffer[i<<3];

      i64 = (SS_WORD64) TO64(p);

      switch (toWordSize)
      {
      case 0: u.u8[length+i] = (SS_WORD8) i64;
        break;
      case 1: u.u16[length+i] = (SS_WORD16) i64;
        break;
      case 2: u.u32[length+i] = (SS_WORD32) i64;
        break;
      case 3: u.u64[length+i] = (SS_WORD64) i64;
        break;
      }
    }
    length+=_length;
    break;
  }

}

/**
 * Ensures the capacity of the internal buffer. Never shrinks.
 * @param _length the length of a specified type array.
 */
void
SBMapBuffer::ensureCapacity(int _length)
{
  if (bufferSize>_length) return ;

  SS_WORD64 *u64;
  int newSize = (_length>bufferSize) ? bufferSize + _length :
      bufferSize << 1;

  // Some arch it is 32 bit :( - what a waste of memory..
  u64 = (SS_WORD64*) new SS_WORD64[1+((newSize<<toWordSize)>>2)];
  CHECK_NEW (u64);
  memcpy (u64, u.u64, length * (1<<toWordSize));
  delete u.u64; u.u64= u64;
  bufferSize = newSize;
}

/**
 * Reset the size of the internal buffer
 */
void
SBMapBuffer::reset ()
{
  length = 0;
}

/**
 * create a new state model instance.
 * @param inFromLength the byte count of the input 
 * @param inToLength the byte count of the input stored in member <b>in<b>.
 * @param outFromLength the byte count of the output 
 * @param outToLength the byte count of the output stored in member <b>out<b>.
 */

SStateModel::SStateModel (int inFromLength, int inToLength,
  int outFromLength, int outToLength) :
  out (outFromLength, outToLength),
  in (inFromLength, inToLength)

{
  low = 0;
  high = 0;
  nextPos = 0;
  lastPos = 0;
  lastMatch = 0;
  circle = 0;
  needReset = 0;
  circleResult = 0;
  circleCount = 0;
  circleSize = 0;
}

SStateModel::~SStateModel()
{
  reset ();
}

/**
 * Reset the state machine.
 */
void
SStateModel::reset ()
{
  low = 0;
  high = 0;
  nextPos = 0;
  lastPos = 0;
  lastMatch = 0;
  circle = 0;
  needReset = 0;
  circleResult = 0;
  circleCount = 0;
  circleSize = 0;

  in.reset ();
  out.reset ();
}

/**
 * Clear the SBMap. Get rid of assigned buffer.
 * You should not use any SStateModel after this.
 */
void
SBMap::clear ()
{
  int  i;

  if (bufferType == SBMap_MMAP && buffer!=0)
  {
    SFileImage fi;
    image = fi;
  }
  if (bufferType == SBMap_DYNAMIC && buffer!=0) delete buffer;

  if (nameType == SBMap_DYNAMIC)
  {
    delete nameBuffer;
  }
  nameType = SBMap_ARRAY;

  if (commentType == SBMap_DYNAMIC)
  {
    delete commentBuffer;
  }
  commentType = SBMap_ARRAY;

  status = false;
  buffer =0;
  bufferSize = 0;
  
//fprintf (stderr, "mapSize= %d of deleting %lx \n", mapSize, (unsigned long) this);
  for (i=0; i<mapSize; i++)
  {
//fprintf (stderr, "deleting %d\n", i);
    if (maps != 0 && maps[i] !=0) delete maps[i];
  }
//fprintf (stderr, "deleting maps\n");
  if (maps!=0) delete maps;
  maps = 0;
  mapSize = 0;

  bufferType = SBMap_ARRAY;
  comment = 0;
  commentSize = 0;
}

/**
 *  Assign a file to be used for this map.
 *  @author Gaspar Sinai <gsinai@iname.com>
 *  @version 1.0
 *  @param file the name of the file. This should be on your UMAP_PATH.
 *         a ".map" extension will be attached to this name.
 *  @return false if failed to map this file.
 */
bool
SBMap::setFileImage (const SFileImage& _image)
{
  clear ();
  
  image = _image;
  
  status = false;
  if (image.size() > 0)
  {
     status = true;
     bufferSize = image.size();
     buffer = (unsigned char*) image.array();
     bufferType = SBMap_MMAP;
     processBuffer();
  }
  if (status!=true) clear();
  return status;
}

/**
 *  Assign an array to be used for this map.
 *  The array should not be changed till this map
 *  is deleted.
 *  @author Gaspar Sinai <gsinai@iname.com>
 *  @version 1.0
 *  @param buffer the buffer to be used. 
 *  @return false if failed to map this array.
 */
bool
SBMap::setArray (unsigned char* _buffer, int size)
{
  clear ();
  buffer = _buffer;
  bufferSize = size;
  bufferType = SBMap_ARRAY;
  status = true;
  return processBuffer();
}
/**
 * Using the SBMapType _type map try to code the input stream using the statemodel.
 * It circles around the given SBMapType and adds up the first array element
 * of results if array size is greater than zero. The result is stored in a
 * n bit array.
 *
 * The statemodel should match this  SBMap:
 * <pre>
 *  if (map->inWordSize != stateModel->in.toWordSize) return REJECT;
 *  if (map->outWordSize != stateModel->out.fromWordSize) return REJECT;
 * </pre>
 * The best is to create a statemodel like this:
 * <pre>
 * new SStateModel (myInWordSize, nmap.getInWordSize (index),
 *     map.getOutWordSize (index), myOutWordSize);
 * </pre>
 * 
 * @param in is the input buffer. This will be appended to the statemodel.
 * @param in_size is the size of the input buffer
 * @param more is 0 if no more input comes. If this matches, return it.
 * @param in_match is the size of the match in input buffer. It is only set if
 *        MATCH or EXACT_MATCH is returned.
 * @param *out is the size of the output buffer. This buffer belongs to SBMap
 *        and it will disappear as soon as SBMap is gone.
 * @param  *out_size the size of the output buffer is indicated here.
 * @return depending on the match the following values can be returned.
 * <ul>
 *  <li>  SS_REJECT if failed to code this string.  </li>
 *  <li>  SS_ACCEPT if string will match. This will never be returned if more is 0. </li>
 *  <li>  The input string length matched, greater or equal to 0.
 *        Note that 0 byte can be a valid match. </li>
 * </ul>
 * Statemodel will have the following parameters set:
 * <ul>
 *  <li> out.length is the size of the output buffer. This buffer belongs to
 *       SStateModel. </li>
 *  <li> out.u is the the output buffer. This buffer belongs to SStateModel. 
 *  <li> state is the position in in.u </li>
 *  <li> low is the low index matching </li>
 *  <li> high is the high index matching </li>
 *  <li> last is the last state matched </li>
 * </ul>
 */

int
SBMap::circle (SBMapType _type, const void *in, int in_size, 
  SStateModel *stateModel, int more)
{
  if (stateModel->needReset)
  {
    stateModel->reset();
  }

  /* make sure we match circle */
  int i;
  int res;
  int all = getSize();
  for (i=stateModel->circle; i<all; i++)
  {
    if (getType(i) == _type) break;
  }
  if (i==all)
  {
    stateModel->needReset = 1;
    return SS_REJECT;
  }
  stateModel->circle = i;

  res = encode (i, in, in_size, stateModel, more);
  if (res==SS_ACCEPT) return SS_ACCEPT;
  if (res==SS_REJECT)
  {
    stateModel->needReset = 1;
    return SS_REJECT;
  }
  
  stateModel->in.shift (res);
  stateModel->circleSize+=res;

  if (stateModel->out.length > 0)
  {
    switch (stateModel->out.toWordSize)
    {
    case 0: stateModel->circleResult += (SS_WORD64) stateModel->out.u.u8[0]; break;
    case 1: stateModel->circleResult += (SS_WORD64) stateModel->out.u.u16[0]; break;
    case 2: stateModel->circleResult += (SS_WORD64) stateModel->out.u.u32[0]; break;
    case 3: stateModel->circleResult += (SS_WORD64) stateModel->out.u.u64[0]; break;
    }
    stateModel->circleCount++;
  }

  // Find next.
  for (i=stateModel->circle+1; i<all; i++)
  {
    if (getType(i) == _type) break;
  }

  // There is next - or maybe...
  if (i<all)
  {
    stateModel->low = 0;
    stateModel->high = 0;
    stateModel->nextPos = 0;
    stateModel->lastPos = 0;
    stateModel->lastMatch = 0;
    stateModel->circle = i;

    if (stateModel->in.length!=0)
    {
      // in already shifted in.
      return circle (_type, in, 0, stateModel, more);
    }
    if (more==0)
    {
      stateModel->needReset = 1;
      return SS_REJECT;
    }
    return SS_ACCEPT;
  }

  // There is no next.
  stateModel->needReset = 1;

  // This has only one encoder/decoder. Don't spoil output!
  if (stateModel->circleCount==1) return res;

  switch (stateModel->out.toWordSize)
  {
  case 0: stateModel->out.u.u8[0] = (SS_WORD8) stateModel->circleResult; break;
  case 1: stateModel->out.u.u16[0] = (SS_WORD16) stateModel->circleResult; break;
  case 2: stateModel->out.u.u32[0] = (SS_WORD32) stateModel->circleResult; break;
  case 3: stateModel->out.u.u64[0] = (SS_WORD64) stateModel->circleResult; break;
  }
  stateModel->out.length=1;
  return stateModel->circleSize;
}

/**
 * Using the mapIndex map try to code the input stream using the statemodel.
 * The statemodel should match this  SBMap:
 * <pre>
 *  if (map->inWordSize != stateModel->in.toWordSize) return REJECT;
 *  if (map->outWordSize != stateModel->out.fromWordSize) return REJECT;
 * </pre>
 * The best is to create a statemodel like this:
 * <pre>
 * new SStateModel (myInWordSize, nmap.getInWordSize (index),
 *     map.getOutWordSize (index), myOutWordSize);
 * </pre>
 * 
 * @param in is the input buffer. This will be appended to the statemodel.
 * @param in_size is the size of the input buffer
 * @param more is 0 if no more input comes. If this matches, return it.
 * @param in_match is the size of the match in input buffer. It is only set if
 *        MATCH or EXACT_MATCH is returned.
 * @param *out is the size of the output buffer. This buffer belongs to SBMap
 *        and it will disappear as soon as SBMap is gone.
 * @param  *out_size the size of the output buffer is indicated here.
 * @return depending on the match the following values can be returned.
 * <ul>
 *  <li>  SS_REJECT if failed to code this string.  </li>
 *  <li>  SS_ACCEPT if string will match. This will never be returned if more is 0. </li>
 *  <li>  The input string length matched, greater or equal to 0.
 *        Note that 0 byte can be a valid match. </li>
 * </ul>
 * Statemodel will have the following parameters set:
 * <ul>
 *  <li> out.length is the size of the output buffer. This buffer belongs to
 *       SStateModel. </li>
 *  <li> out.u is the the output buffer. This buffer belongs to SStateModel. 
 *  <li> state is the position in in.u </li>
 *  <li> low is the low index matching </li>
 *  <li> high is the high index matching </li>
 *  <li> last is the last state matched </li>
 * </ul>
 */

int
SBMap::encode (int mapIndex, const void *in, int in_size, 
  SStateModel *stateModel, int more)
{
  unsigned int  i;
  unsigned int  j;
  SBMapItem::SFound  result;

  if (status!=true || maps ==0 || mapIndex >= mapSize) return SS_REJECT;

  SBMapItem* map = maps[mapIndex];

  // We go through the input and do the search byte by byte
  // This sucks. Should not do this. 
  stateModel->in.appendToNet (in, in_size);
  if (stateModel->in.length==0) return SS_ACCEPT;

  if (map->inWordSize != stateModel->in.toWordSize) return SS_REJECT;
  if (map->outWordSize != stateModel->out.fromWordSize) return SS_REJECT;

  if (stateModel->nextPos==0)
  {
    unsigned int a, b;
    stateModel->lastPos = SS_REJECT;
    stateModel->lastMatch = 0;
    if (map->itemType != SBMapItem::SBMapBumap)
    {
      map->getKey(0, &a, &b);
      if (b==0) stateModel->lastPos = 0;
    }
    stateModel->low = 0; 
    stateModel->high = map->codeSize;
  }


  result = SBMapItem::REJECT;

  const char* buf= (const char*) stateModel->in.u.u8;

  unsigned int  shift = stateModel->in.toWordSize;
  unsigned int  ilimit = (stateModel->in.length << shift);
  unsigned int  istep = (1 << shift);


  for (i=stateModel->nextPos; i<ilimit; i += istep)
  {
    // This is in net order!
    for (j=0; j<istep; j++)
    {
      result = map->find (buf[i+j], i+j, 
        &stateModel->low, &stateModel->high);
      if (result==SBMapItem::REJECT) break;
    }
    stateModel->nextPos += (1<<shift);
    switch (result)
    {
    case SBMapItem::REJECT:
      if (stateModel->lastPos>=0)
      {
        // Lastmatch is set in setOutput
        setOutput (map, stateModel);
        return stateModel->lastPos;
      }
      return SS_REJECT;
    case SBMapItem::MORE:
      break;
    case SBMapItem::MATCH_MORE:
      stateModel->lastPos = i+1;
      stateModel->lastMatch =  stateModel->low;
      break;
    case SBMapItem::MATCH:
      stateModel->lastMatch = stateModel->low;
      // setOutput updates lastPos
      setOutput (map, stateModel);
      return stateModel->lastPos;
    }
  }
  switch (result)
  {
  case SBMapItem::REJECT: return SS_REJECT;
  case SBMapItem::MORE:
    if (more==0)
    {
      return SS_REJECT;
    }
    return SS_ACCEPT;
  case SBMapItem::MATCH_MORE:
    if (more==0)
    {
      setOutput (map, stateModel);
      return stateModel->lastPos;
    }
    return SS_ACCEPT;
  case SBMapItem::MATCH:
    // We did process it.
    break;
  }
  return SS_REJECT;
}

/**
 * Set the output 
 * @param map is the map where the output can be found.
 * @param staetmodel is the state model where the last and  from can be found
 */
void
SBMap::setOutput(SBMapItem* map, SStateModel* stateModel)
{
  if (map->itemType==SBMapItem::SBMapBumap)
  {
    // The input is in from, the output is in to
    stateModel->out.reset();
    SS_WORD16  w16;
    SS_WORD8  w8;
    if (map->outWordSize==1)
    {
      w16 = (SS_WORD16) stateModel->high;
      stateModel->out.append (&w16, 1);
    }
    else
    {
      w8 = (SS_WORD8) stateModel->high;
      stateModel->out.append (&w8, 1);
    }
    stateModel->lastPos = 1;
    return;
  }
  unsigned int byteSize;
  unsigned int macthedSize;
  map->getKey(stateModel->lastMatch, &byteSize, &macthedSize);
  stateModel->lastPos = macthedSize >> stateModel->in.toWordSize;
  const unsigned char* vle = map->getValue(stateModel->lastMatch, &byteSize);
  stateModel->out.reset();
  stateModel->out.appendFromNet (vle, byteSize>>map->outWordSize);
}


/**
 * Get the number of converters
 * @return the number of converters in this map.
 */
int
SBMap::getSize (int _index)
{
  if (maps==0 || status==false) return 0;
  if (_index<0)
  {
    return mapSize;
  }
  return maps[_index]->codeSize;
}

/**
 * @return whether that converter is encoding or decoding (reverse) type.
 *  SBMap_ENCODE or SBMap_DECODE.
 */
SBMap::SBMapType
SBMap::getType (int index)
{
  return ((maps[index]->encode) ? SBMap_ENCODE : SBMap_DECODE);
}

/**
 * perform some stoolkit check on buffer and assign maps.
 */
bool 
SBMap::processBuffer ()
{

  if (strncmp ((const char*)buffer, magicNN, strlen (magicNN))==0)
  {
    return  processSBMapBuffer ();
  }
  else if (strncmp ((const char*)buffer, magicBM, strlen (magicBM))==0)
  {
    return  processBMBuffer ();
  }
  fprintf (stderr, "Error: corrupted Yudit n to n map file.\n");
  status = false;
  return status;
}

/**
 * Process buffer - this is an BM buffer for sure.
 */
bool
SBMap::processBMBuffer ()
{
  // Font map.
  mapType = 2;

  status = false;
  name = &buffer[16];
  const unsigned char* mp = &buffer[16+32];
  int  offset = TO16 (mp);
  if (offset<(16+32+2)) return false;
  commentSize = offset - (16+32+2);
  mp++; mp++;
  comment = mp;
  mp = &mp[commentSize];
  SS_WORD16 localHighMin=TO16(mp); mp++; mp++;
  SS_WORD16 localHighMax=TO16(mp); mp++; mp++;
  SS_WORD16 localLowMin=TO16(mp); mp++; mp++;
  SS_WORD16 localLowMax=TO16(mp); mp++; mp++;

  SS_WORD16 uniHighMin=TO16(mp); mp++; mp++;
  SS_WORD16 uniHighMax=TO16(mp); mp++; mp++;
  SS_WORD16 uniLowMin=TO16(mp); mp++; mp++;
  SS_WORD16 uniLowMax=TO16(mp); mp++; mp++;

  mapSize = 2;
  maps = new SBMapItem*[mapSize];
  CHECK_NEW (maps);
  int decodeSize = (localLowMax - localLowMin +1) * (localHighMax - localHighMin +1);
  //int encodeSize = (localLowMax - localLowMin +1) * (localHighMax - localHighMin +1);
  maps[0] = new SBMapItem (0, (localHighMax>0) ? 1 : 0, 
    (uniHighMax>0) ? 1 : 0,
    localHighMin, localHighMax, localLowMin, localLowMax, mp);
  // 2 byte
  mp = &mp[decodeSize<<1];
  maps[1] = new SBMapItem (1, (uniHighMax>0) ? 1: 0,
    (localHighMax>0) ? 1 : 0,
    uniHighMin, uniHighMax, uniLowMin, uniLowMax, mp);
  status = true;
  return true;
}

/**
 * Make a unicode map.
 */
bool
SBMap::makeUnicodeMap ()
{
  clear ();

  mapSize = 2;
  maps = new SBMapItem*[mapSize];
  CHECK_NEW (maps);
  maps[0] = new SBMapItem (0, 1, 1, 0, 255, 0, 255);
  maps[1] = new SBMapItem (1, 1, 1, 0, 255, 0, 255);
  status = true; 
  return true;
}

/**
 * Make a iso-8859-1 map.
 */
bool
SBMap::makeStraightMap ()
{
  clear ();

  mapSize = 2;
  maps = new SBMapItem*[mapSize];
  CHECK_NEW (maps);
  maps[0] = new SBMapItem (0, 0, 1, 32, 255, 0, 0);
  maps[1] = new SBMapItem (1, 1, 0, 0, 0, 32, 255);
  status = true; 
  return true;
}

/**
 * Process buffer - this is an NN buffer for sure.
 */
bool
SBMap::processSBMapBuffer ()
{
  const unsigned char* mp;
  name = &buffer[16];
  mp = &buffer[16+32];
  commentSize = TO32 (mp);
  mp++; mp++; mp++; mp++;
  comment = mp;
  mp = &mp[commentSize];

  mapType = TO32 (mp);
  mp++; mp++; mp++; mp++;
  mapSize = TO32 (mp);
  mp++; mp++; mp++; mp++;
  maps = new SBMapItem*[mapSize];

//fprintf (stderr, "SBMap::processSBMapBuffer %d\n", mapSize);

  base = &buffer [16+32+4+commentSize+4+4+4*(mapSize+1)];
  if (maps==0)
  {
    fprintf (stderr, "Map pretends to be size: %d , memory exhausted.\n",
        mapSize);
    status = false;
    return status;
  }
  int i;
  unsigned int offset;
  for (i=0; i<mapSize; i++)
  {
    offset = TO32 (mp);
    mp++; mp++; mp++; mp++;
    maps[i] = new SBMapItem(&base[offset]);
    if (!VALID_WORD(maps[i]->inWordSize) 
      || ! VALID_WORD(maps[i]->outWordSize) 
      || ! VALID_WORD(maps[i]->inByteLength)
      || ! VALID_WORD(maps[i]->outByteLength))
    {
      fprintf (stderr,  "error: map word size is wrong [in=%d,out=%d,inlength=%d,,outlength=%d]\n", (int) maps[i]->inWordSize, (int) maps[i]->outWordSize ,
      (int) maps[i]->inByteLength, (int) maps[i]->outByteLength); 

      while (i>=0) delete maps[i--];
      delete maps; maps=0;
      status = false;
      return status;
    }
  }
  if (mapSize ==0)
  {
    status = false;
    return status;
  }
  status = true;
  return status;
}

/**
 * create a bumap type SBMapItem
 */
SBMapItem::SBMapItem (int _encode, unsigned int _inWordSize, int _outWordSize, SS_WORD16 _highMin, SS_WORD16 _highMax, SS_WORD16 _lowMin, SS_WORD16 _lowMax, const unsigned char* _matrix)
{
  itemType = SBMapBumap;
  // Determine size. 
  //(decodeHighMax << 8) + 0xff;
  encode = _encode;
  highMin = _highMin;
  highMax = _highMax;
  lowMin = _lowMin;
  lowMax = _lowMax;
  base = _matrix;
  
  // Huh - have to go through
  inWordSize = _inWordSize;
  outWordSize = _outWordSize;
  name = (const unsigned char*) dummyName;

  inByteLength = 0; 
  outByteLength = 0;

  stateMachine = 0;
  stateMachineSize = 0;
  stateMachineType = SS_STATIC;
}

/**
 * Initialize a single map buffer
 */
SBMapItem::SBMapItem (const unsigned char* buff)
{
  itemType = SBMapNToN;
  stateMachineType = SS_STATIC;
  baseType = SS_STATIC;
  codeMapType = SS_STATIC;
  nameType = SS_STATIC;

  unsigned int  smac;
  const unsigned char* mp = buff;

  name = mp;
  // First there is a long name
  mp = &mp[32];

  commentSize = TO32 (mp);
  mp++; mp++; mp++; mp++;
  comment = mp;
  mp = &mp[commentSize];

  encode = mp[0]; mp++;

  inWordSize = mp[0]; mp++;
  outWordSize = mp[0]; mp++;

  inByteLength = mp[0]; mp++; 
  outByteLength = mp[0]; mp++;


  smac = TO32 (mp);
  mp++; mp++; mp++; mp++;
  stateMachine = 0;
  stateMachineSize = 0;
  stateMachineType = SS_STATIC;

  // Ignore spare 
  mp++; mp++; mp++; mp++;

  codeSize = TO32 (mp);
  mp++; mp++; mp++; mp++;
  codeMap = mp;
  mp = &mp[(codeSize<<2)];
  baseSize = TO32 (mp);
  mp++; mp++; mp++; mp++;
  base = mp;

  
  if (smac>0)
  {
    stateMachine = &base[smac];
    stateMachineSize = TO32 (stateMachine);
    stateMachine++; stateMachine++; stateMachine++; stateMachine++;
  }
}

/**
 * SBMapItem constructor for dynamically built maps.
 * @param _name a max 32-byte-long name.
 * @param _comment a utf8 string (recommended)
 * @param _commentSize the size of the comment.
 * @param _inWordSize the input word (key) max length - 0=8bit, 1=16bit, 2=32bit, 3=64bit
 * @param _outWordSize the output word (value~ max length - 0=8bit, 1=16bit, 2=32bit, 3=64bit
 * @param _inByteLength the input word (key) max length - the number of bytes (maximum value)
 * @param _outByteLength the output word (value max length - the number of bytes (maximum value)
 */
SBMapItem::SBMapItem (int _encode,
  const unsigned char* _name, const unsigned char* _comment, 
  unsigned int _commentSize,
  unsigned int _inWordSize, unsigned int _outWordSize, 
  unsigned int _inByteLength, unsigned int _outByteLength)
{
  itemType = SBMapNToN;

  nameBuffer = new unsigned char[32];
  CHECK_NEW (nameBuffer);
  memset (nameBuffer, 0, 32);
  strncpy ((char*)nameBuffer, (const char*) _name, 31);
  nameType = SS_DYNAMIC;
  name = nameBuffer;

  commentBufferSize = _commentSize;
  commentBuffer = new unsigned char[commentBufferSize];
  CHECK_NEW (commentBuffer);
  memcpy (commentBuffer, _comment, commentBufferSize);
  comment=commentBuffer;
  commentType = SS_DYNAMIC;
  commentSize=commentBufferSize;

  codeMap = 0;
  codeSize = 0;
  codeMapType = SS_STATIC;

  base = 0;
  baseSize = 0;
  baseType = SS_STATIC;

  inWordSize = _inWordSize;
  outWordSize = _outWordSize;

  inByteLength = _inByteLength; 
  outByteLength = _outByteLength;

  stateMachineType = SS_STATIC;;
  stateMachineSize = 0;
  stateMachine = 0;

  encode = _encode;
}

SBMapItem::~SBMapItem ()
{
//fprintf (stderr, "Deleting map dynamic=%d this=%lx name=%s\n",
// stateMachineType == SS_DYNAMIC, (unsigned long) this, nameBuffer);
  if (itemType == SBMapNToN && stateMachineType == SS_DYNAMIC)
  {
    delete stateMachineBuffer;
  }
  if (itemType == SBMapNToN && codeMapType == SS_DYNAMIC)
  {
    delete codeMapBuffer;
  }
  if (itemType == SBMapNToN && baseType == SS_DYNAMIC)
  {
    delete baseBuffer;
  }
  if (itemType == SBMapNToN && commentType == SS_DYNAMIC)
  {
    delete commentBuffer;
  }
  if (itemType == SBMapNToN && nameType == SS_DYNAMIC)
  {
    delete nameBuffer;
  }
}

/**
 * get rid of all state machines.
 */
void
SBMapItem::strip ()
{
  if (itemType!=SBMapNToN) return ;

  if (stateMachineType == SS_DYNAMIC)
  {
    delete stateMachineBuffer;
  }
  stateMachineSize = 0;
  stateMachine = 0;
  stateMachineType = SS_STATIC;
}


/**
 * return the comment.
 * @param at is the index at which the string is
 * @param size is the byte-length of string.
 */
const unsigned char*
SBMapItem::getComment (int at, unsigned int *size)
{
  if (itemType!=SBMapNToN)
  {
    *size = 0;
    return (const unsigned char*) "";
  }
  const unsigned char*  vle = &codeMap[at<<2];

  unsigned int index = TO32(vle);
  unsigned int keyHead = 1 << inByteLength;
  unsigned int vleHead = 1 << outByteLength;
  // Comment head is 1.

  unsigned int  i;

  vle = &base[index];

  index =0;
  unsigned int svle=0;
  for (i=0; i<keyHead; i++)
  {
    svle = vle[index++] + (svle<<8);
  }
  index += keyHead;
  // This is the start of key.
  index += svle;

  svle=0;
  for (i=0; i<vleHead; i++)
  {
    svle = vle[index++] + (svle<<8);
  }
  // This is the start of vle
  index += svle;

  // comment.
  svle = (unsigned int) vle[index++];
  *size = svle;
  return &vle[index];
}

/**
 * get the length of this item.
 * @param at is the index at which the string is
 * @return the length of this cell.
 */
unsigned int
SBMapItem::getLength (unsigned int at)
{
  if (itemType != SBMapNToN)
  {
    return 0;
  }

  const unsigned char*  vle = &codeMap[at<<2];

  unsigned int index = TO32(vle);
  unsigned int keyHead = 1 << inByteLength;
  unsigned int vleHead = 1 << outByteLength;
  // Comment head is 1.

  unsigned int  i;

  vle = &base[index];

  index =0;
  unsigned int svle=0;
  for (i=0; i<keyHead; i++)
  {
    svle = vle[index++] + (svle<<8);
  }
  index += keyHead;
  // This is the start of key.
  index += svle;

  svle=0;
  for (i=0; i<vleHead; i++)
  {
    svle = vle[index++] + (svle<<8);
  }
  // This is the start of vle
  index += svle;

  // comment.
  svle = (unsigned int) vle[index++];
  return index+svle;

}

/**
 * convert a bumap to a n to n  map.
 * Does some allocations.
 */
void
SBMapItem::convertFromBumap()
{
  if (itemType != SBMapBumap)
  {
    return;
  }

  // Avoid infinite loop.
  itemType = SBMapNToN;

  const unsigned char* oldBase;
  const unsigned char* mp;
  codeSize = 0;
  baseSize = 0;
  int  i;
  int  j;
  int  arrayIndex;
  SS_WORD16  input;
  SS_WORD16  output;
  char  word[64];
  char  fullLine[64];
  int  status;

  // Go through the array and map everything to everything.
  oldBase = base;
  base = 0;
  for (i=highMin; i<=highMax; i++) for (j=lowMin; j<=lowMax; j++)
  {
    arrayIndex = (i - highMin) * (lowMax - lowMin+1) + (j - lowMin);
    input = (i<<8) + j;
    if (oldBase==0)
    {
      output = input;
    }
    else
    {
      mp = &oldBase[arrayIndex<<1];
      output = TO16 (mp);
    }
    if (output == 0) continue;
    if (inWordSize==0)
    {
      snprintf (fullLine, 64, "%02X -> ", input&0xff);
    }
    else
    {
      snprintf (fullLine, 64, "%04X -> ", input&0xffff);
    }
    if (outWordSize==0)
    {
      snprintf (word, 64, "%02X", output&0xff);
    }
    else
    {
      snprintf (word, 64, "%04X", output&0xffff);
    }
    strcat (fullLine, word);
    status = addLine ((const unsigned char*) fullLine, strlen (fullLine));
    if (status < 0)
    {
      fprintf (stderr, "SBMapItem::convertFromBumap can not add '%s'\n", fullLine);
    }
  }
}

/**
 * Add a new line into the map. Clear state machine, and make map dynamic.
 * @param _key is the pointer to the bytestream key.
 * @param _keySize is the byte length of the key. 
 * @param _matchSize is the byte-length of the sub-key, the part that will be returned as matched. 
 * @param _value is the pointer to the bytestream value.
 * @param _valueSize is the byte-length of the value. 
 * @param _comment is the - preferredly- utf8 comment. 
 * @param _commentSize is the size of the comment in bytes. Its should be less than 255.
 * @return the new index or a negative number on failure.
 */
int
SBMapItem::add (const unsigned char* _key, int _keySize, int _matchSize, 
      const unsigned char* _value, int _valueSize,
      const unsigned char* _comment, int _commentSize)
{
  // Use name and comment. Then convert the rest.
  if (itemType == SBMapBumap)
  {
    convertFromBumap();
  }
  unsigned int  needBytes;
  unsigned int 	newSize;
  unsigned char*  newBuffer;

  if ((1<<(8*(1<<inByteLength))) <= _keySize || _matchSize > _keySize
    || (1<<(8*(1<<outByteLength))) <= _valueSize || _commentSize > 255)
  {
    fprintf (stderr, "error: map can not hold this key/value length.");
    return -1;
  }

  if (stateMachineType == SS_DYNAMIC)
  {
    delete stateMachineBuffer;
    stateMachineBuffer = 0;
    stateMachineType = SS_STATIC;
  }

  // We need to get rid of the state machine.
  stateMachine = 0;
  stateMachineSize = 0;

  // Make a new code map.
  if (codeMapType == SS_STATIC)
  {
    codeMapBufferSize = (codeSize == 0) ? 2 : (codeSize << 1) +2;
    codeMapBuffer = new unsigned char[4*codeMapBufferSize];
    CHECK_NEW (codeMapBuffer);
    if (codeSize>0)
    {
      memcpy (codeMapBuffer, codeMap, 4 * codeSize);
    }
    codeMap = codeMapBuffer;
    codeMapType = SS_DYNAMIC;
  }

  // Make a new buffer.
  if (baseType == SS_STATIC)
  {
    baseBufferSize = (baseSize == 0) ? 32 : (baseSize << 1) + 2;
    baseBuffer = new unsigned char[baseBufferSize];
    CHECK_NEW (baseBuffer);
    if (baseSize>0)
    {
      memcpy (baseBuffer, base, baseSize);
    }
    base = baseBuffer;
    baseType = SS_DYNAMIC;
  }

  // Ensure capacity. 
  if (codeMapBufferSize <= codeSize)
  {
    newSize = codeMapBufferSize << 1;
    newBuffer = new unsigned char[newSize*4];
    CHECK_NEW (newBuffer);
    if (newSize>0)
    {
      memcpy (newBuffer, codeMap, codeSize*4);
    }
    delete codeMapBuffer;
    codeMapBuffer = newBuffer;
    codeMapBufferSize = newSize;
    codeMap = codeMapBuffer;
    codeMapType = SS_DYNAMIC;
  }

  // Ensure capacity. Length is max 4 bytes...
  needBytes = _keySize + _valueSize + _commentSize +  2*(1 <<inByteLength) + (1<<outByteLength) + 1;
  if (baseBufferSize < baseSize + needBytes)
  {
    newSize = (needBytes > baseBufferSize) ?
      baseBufferSize + needBytes :  baseBufferSize*2;
    newBuffer = new unsigned char[newSize];
    CHECK_NEW (newBuffer);
    if (newSize>0)
    {
      memcpy (newBuffer, base, baseSize);
    }
    delete baseBuffer;
    baseBuffer = newBuffer;
    baseBufferSize = newSize;
    base = baseBuffer;
    baseType = SS_DYNAMIC;
  }

  // Let's do the work.
  int   i;
  int  from;
  int  len;
  
  unsigned char* newElement = &baseBuffer[baseSize]; 

  int  index=0;
  len = 1<< inByteLength;
  for (i=0; i<len; i++)
  {
    newElement[index++] = (_keySize >> (8 * (len-i-1))) & 0xff;
  }
  for (i=0; i<len; i++)
  {
    newElement[index++] = (_matchSize >> (8 * (len-i-1))) & 0xff;
  }
  for (i=0; i<_keySize; i++)
  {
    newElement[index++] = _key[i];
  }
  
  len = 1<< outByteLength;
  for (i=0; i<len; i++)
  {
    newElement[index++] = (_valueSize >> (8 * (len-i-1))) & 0xff;
  }
  for (i=0; i<_valueSize; i++)
  {
    newElement[index++] = _value[i];
  }

  newElement[index++] = (unsigned char) _commentSize;
  for (i=0; i<_commentSize; i++)
  {
    newElement[index++] = _comment[i];
  }

/**
 *  Do a binary search 
 */
  from = nextSorted(_key, _keySize);
  
  // The will be after that..

  // So where would we like to insert this? 

  // Move the old data.
  for (i=codeSize; i>from; i--)
  {
    codeMapBuffer [i*4] = codeMapBuffer [(i-1)*4];
    codeMapBuffer [i*4+1] = codeMapBuffer [(i-1)*4+1];
    codeMapBuffer [i*4+2] = codeMapBuffer [(i-1)*4+2];
    codeMapBuffer [i*4+3] = codeMapBuffer [(i-1)*4+3];
  }

  // And insert the new one.
  codeMapBuffer [from*4] = ((SS_WORD32)baseSize>>24) & 0xff;
  codeMapBuffer [from*4+1] = ((SS_WORD32)baseSize>>16) & 0xff;
  codeMapBuffer [from*4+2] = ((SS_WORD32)baseSize>>8) & 0xff;
  codeMapBuffer [from*4+3] = (SS_WORD32)baseSize & 0xff;
  codeSize++;

  baseSize += index;


  return from;
}


/**
 * Find the insertion point in the sorted array
 * This routine will return the insertion point where
 * The key should be inserted. If the same key is already there
 * it will be inserted after that key.
 * @param key is the key to find.
 * @param klen is the length of the key
 */
unsigned int 
SBMapItem::nextSorted (const unsigned char* key, unsigned int klen)
{
  unsigned int    top, bottom, mid;
  unsigned int keyLen1;
  top = codeSize; bottom = 0;
  const unsigned char* vle1;
  int result;

  while (top > bottom) {
    mid = (top+bottom)/2;
    vle1 = getKey (mid, &keyLen1);
    /* compare key with vle1 */
    unsigned int cmplen = (keyLen1 > klen) ? klen : keyLen1;
    result = 0;
    for (unsigned int i=0; i<cmplen; i++)
    {
      if (key[i] > vle1[i]) { result = 1; break; } 
      if (key[i] < vle1[i]) { result = -1; break; } 
    }
    if (result==0)
    {
       if (klen > keyLen1) { result = 1; }
       if (klen < keyLen1) { result = -1; }
    }
     /* if equal we want to insert after */
     if (result == 0) { top = mid+1; break; }
     if (result == -1) { top = mid; continue; }
     bottom = mid + 1;
  }
  return top;
}

/**
 * Add a new line into the map. Clear state machine, and make map dynamic.
 * @param _line is the pointer to the buffer for input
 *   the input line should look like tthis:
 *   <pre>
 *   20 23 6f 45  -> aa9c
 *   30 23 6f 45 / 5b 44 -> 98ff
 *   'A '# -> 2046 # This is a comment.
 *   </pre>
 * The first line means that 2023 6f45 will be mapped to aa 9c
 * The second line means that 3023 6f45  that is followed by 5b 44 will be mapped to 
 * aa 9c. The macthed size will be only 3023 6f45. The third line is an example for
 * using escape ascii characterrs. The comment will be also placed into the map.
 * @param _length is the size of the _line in bytes.
 * @param _reverse is nonzero to reverse the stuff.
 * @return the new index or a negative number on failure.
 */
int
SBMapItem::addLine (const unsigned char* _line, unsigned int _length, bool _reverse)
{
  if (itemType != SBMapNToN)
  {
    return -1;
  }
  unsigned int    i;
  unsigned int    start = 0;
  unsigned int	toggle = 0;
  unsigned char*   key = 0;
  unsigned char*   vle = 0;
  unsigned int     keyLen = 0;
  unsigned int    match = 0;
  unsigned int     vleLen = 0;
  int    dummy = 0;
  unsigned int    matchVle = 0;
  unsigned int    csize = 0;
  const unsigned char* comm=0;

  // Split up input.
  for (i=0; i<_length; i++)
  {
    if (_line[i] == '\n') break;
    // Key
    if (toggle==0)
    {
      if (i+1<_length && _line[i]=='\'' && _line[i+1]=='#')
      {
        i++; 
      }
      else if (i+1<_length && _line[i]=='-' && _line[i+1]=='>')
      {
        key = toHex (&_line[start], i-start, &keyLen, &match);
        i++;
        start=i+1;
        toggle = 1;
      }
    }
    // Vle
    else if (toggle==1)
    {
      if (i+1<_length && _line[i]=='\'' && _line[i+1]=='#')
      {
        i++; 
      }
      else if (_line[i]=='#')
      {
        vle = toHex (&_line[start], i-start, &vleLen, &matchVle);
        i++;
        start=i;
        toggle = 2;
      }
    }
  }
  if (toggle == 0 || key == 0)
  {
    return -1;
  }
  if (toggle == 1)
  {
    vle = toHex (&_line[start], i-start, &vleLen, &matchVle);
  }
  if (vle ==0)
  {
    delete key;
    return -1;
  }
  if (toggle == 2)
  {
    csize = i-start;
    comm = &_line[start];
  }

  int   iws = (1<<inWordSize);
  int   ows = (1<<outWordSize);

  if (_reverse)
  {
    if ((vleLen % iws) || (matchVle%iws) 
      || (keyLen%ows) || (match%ows))
    {
      dummy = -1;
    }
    else
    {
      dummy = add (vle, vleLen, matchVle, key, match, comm, csize);
    }
  }
  else
  {
    if ((vleLen % ows) || (matchVle%ows) 
      || (keyLen%iws) || (match%iws))
    {
      dummy = -1;
    }
    else
    {
      dummy = add (key, keyLen, match, vle, matchVle, comm, csize);
    }
  }
  delete key;
  delete vle;

  return dummy;
}

/**
 * convert nibbles into a real unsgined char string. This string is owned by 
 * calling function.
 * @param in is the input character buffer. 
 * @param _size is the size of the input character buffer. 
 * @param _len is the length of the resulting array.
 * @param _match is the length of the buffer till '/' 
 * @return the converted hexadecimal buffer.
 */
unsigned char*
SBMapItem::toHex (const unsigned char* in, unsigned int _size, unsigned int* _len, unsigned int* _match)
{
  unsigned int    i;
  unsigned int    count;
  unsigned char*  retVle;
  int    match;

  count = 0;
  match = -1;
  for (i=0; i<_size; i++)
  {
    if (in[i]<= ' ') continue;
    if ((i==0 && in[i]=='/') || (i>0 && in[i]=='/'&&in[i-1]!='\''))
    {
      match = count;
      continue;
    }
    count++;
  }
  if (match == -1) match = count;
  if ((count&1) || (match&1)) return 0;

  // Convert the thing into hex. (real)
  retVle = new unsigned char[count/2];
  *_len = count/2;
  *_match = match/2;
  count = 0;
  int  subH;
  int  subL;
  int  old=0;
  for (i=0; i<_size; i++)
  {
    if (in[i]<= ' ') continue;
    if ((i==0 && in[i]=='/') || (i>0 && in[i]=='/'&&in[i-1]!='\''))
    {
      continue;
    }
    count++;
    if (count&1)
    {
      old = i;
      continue;
    }
    // Escape
    if (in[i-1] == '\'')
    {
      retVle[count/2-1] = in[i];
      continue;
    }
    subH = -1;
    subL = -1;
    if (in[old] >= '0' && in[old] <= '9')
    {
      subH = '0';
    }
    if (in[old] >= 'A' && in[old] <= 'F')
    {
      subH = 'A' - 10;
    }
    if (in[old] >= 'a' && in[old] <= 'f')
    {
      subH = 'a' - 10;
    }
    if (in[i] >= '0' && in[i] <= '9')
    {
      subL = '0';
    }
    if (in[i] >= 'A' && in[i] <= 'F')
    {
      subL = 'A' - 10;
    }
    if (in[i] >= 'a' && in[i] <= 'f')
    {
      subL = 'a' - 10;
    }
    if (subL < 0 || subH < 0)
    {
      delete retVle;
      return 0;
    }
    retVle[count/2-1] = ((in[old]-subH) << 4) + in[i]-subL;
  }
  return retVle;
}

/**
 * return value byte-string from index.
 * @param at is the index at which the string is
 * @param size is the byte-length of string.
 */
const unsigned char*
SBMapItem::getValue (int at, unsigned int *size)
{
  if (itemType != SBMapNToN)
  {
    *size = 0;
    return (const unsigned char*) "";
  }
  const unsigned char*  vle = &codeMap[at<<2];

  unsigned int index = TO32(vle);
  unsigned int keyHead = 1 << inByteLength;
  unsigned int vleHead = 1 << outByteLength;
  // Comment head is 1.

  unsigned int  i;

  vle = &base[index];

  index =0;
  unsigned int svle=0;
  for (i=0; i<keyHead; i++)
  {
    svle = vle[index++] + (svle<<8);
  }
  index += keyHead;
  // This is the start of key.
  index += svle;

  svle=0;
  for (i=0; i<vleHead; i++)
  {
    svle = vle[index++] + (svle<<8);
  }
  // This is the start of vle
  *size =  svle;
  return &vle[index];
}

/**
 * return value byte-string from index.
 * @param at is the index at which the string is
 * @param size is the byte-length of string.
 */
const unsigned char*
SBMapItem::getKey (int at, unsigned int *size, unsigned int *matchedSize)
{
  if (itemType != SBMapNToN)
  {
    *size = 0;
    if (matchedSize!=0) *matchedSize = 0;
    return (const unsigned char*) "";
  }
  const unsigned char*  vle = &codeMap[at<<2];

  unsigned int index = TO32(vle);
  unsigned int keyHead = 1 << inByteLength;
  //unsigned int vleHead = 1 << outByteLength;

  // Comment head is 1.
  unsigned int  i;

  vle = &base[index];

  index =0;
  unsigned int svle=0;
  for (i=0; i<keyHead; i++)
  {
    svle = vle[index++] + (svle<<8);
  }

  *size = svle;
  if (matchedSize==0)
  {
    index += keyHead;
    // This is the start of key.
    return (&vle[index]);
  }
  svle=0;
  for (i=0; i<keyHead; i++)
  {
    svle = vle[index++] + (svle<<8);
  }
  *matchedSize = svle;
  return (&vle[index]);
}

/**
 * find the character at position. This function should be called in sequence
 * - the inner loop that goes through the string has been moved out.
 * It takes care of multiple keys.
 * @param in the input normalized string
 * @param pos the position of the string more to be copied from - from[0]
 * @from the item from which we search - inclusive
 * @to the item till what which we search - exclusive
 * @returns 
 *  <ul>
 *   <li> MATCH - the input matches exactly no more </li>
 *   <li> MATCH_MORE - the input matches exactly with more </li>
 *   <li> MORE - the input matches with more </li>
 *   <li> REJECT - the input won't match </li>
 *  </ul>
 * In case itemType is SBMapBumap, from will be mapped into 'to'.
 * For straight maps, only bounds are checked.
 */
SBMapItem::SFound
SBMapItem::find(const unsigned char in, unsigned int pos, int* from, int* to)
{
  int arrayIndex;
  unsigned char high;
  const unsigned char* mp;

  if (itemType == SBMapBumap)
  {
    if (pos>1) return REJECT;
    if (inWordSize == 0 && pos > 0) return REJECT;
    if (pos == 0)
    {
      if (inWordSize!=0)
      {
        *from = in;
        if (in < highMin || in > highMax) return REJECT;
        return MORE;
      }
      if (highMin>0) return REJECT;
      *from=0;
    }
    if (in < lowMin || in > lowMax) return REJECT;
    // Straight map
    high = (unsigned char) *from;
    *from = ((int)high<<8) + in;
    if (base == 0)
    {
      *to = *from;
      return MATCH;
    }
    arrayIndex = ((int)high - highMin)
      * (lowMax - lowMin+1) + ((int)in - lowMin);
    // Array of 16 bit integers
    mp = &base[arrayIndex<<1];
    *to = TO16 (mp);
    if (*to==0) return REJECT;
    return MATCH;
  }
  if (itemType != SBMapNToN)
  {
    return REJECT;
  }
  //
  // We have a fast map!!!
  //
  if (stateMachine != 0 )
  {
    unsigned int next;
    const unsigned char* sp;
    unsigned int flags;

    // First nibble  * 4
    sp = &stateMachine[((in>>2) & 0x3c) + (*from << 6)];
    next = TO32 (sp);
    flags = (next >> 30) & 0x3;
    *from = next & 0x3fffffff;

    if (flags != MORE && flags != MATCH_MORE)
    {
       return (SFound)flags;
    }

    // Second nibble  * 4
    sp = &stateMachine[((in & 0xf)<<2) + (*from << 6)];
    next = TO32 (sp);
    flags = (next >> 30) & 0x3;
    *from = next & 0x3fffffff;

    return ((SFound) flags);
  }

  //
  // Only binary-search map is available
  //
        unsigned int    top;
        unsigned int    bottom;
        unsigned int    mid;

  const unsigned char*  vle1=0;
  unsigned int    keyLen1;

  const unsigned char*  vle2=0;
  unsigned int    keyLen2;

  top = *to;
  bottom = *from;

  // Search for low position = minimum match all...
  while (top > bottom)
  {
    mid = (top+bottom)>>1;
    vle1 = getKey(mid, &keyLen1);
    // bottom has to be greater or equal
    if (pos > keyLen1 || in > vle1[pos])
    {
      bottom = mid + 1; continue;
    }
    // == or less
    if (mid==bottom)
    {
      break;
    }
    // Let top slip through
    top = mid;
  }

  *from = bottom;
  top = *to;
  // Search for high position
  while (top > bottom)
  {
    mid = (top+bottom)>>1;
    vle2 = getKey(mid, &keyLen2);
    // bottom has to be greater
    if (pos >= keyLen2 || in >= vle2[pos])
    {
      bottom = mid+1; continue;
    }
    // == or less
    if (mid==bottom)
    {
      break;
    }
    // Let top slip through
    top = mid;
  }
  *to=bottom;
  top = bottom;
  bottom = *from; 
  
  if (top <= bottom) return REJECT;

  // This is a match. Which one?
  vle1 = getKey (bottom, &keyLen1);
  if (top == bottom+1)
  {
    vle2 = vle1; 
    keyLen2 = keyLen1;
  }
  else
  {
    vle2 = getKey (top-1, &keyLen2);
  }
  // It is possible to have duplicate keys.
  if (pos == keyLen1-1 && pos == keyLen2-1)
  {
    return MATCH;
  }
  if (pos == keyLen1-1)
  {
    return MATCH_MORE;
  }
  return MORE;

}

/**
 * Get the name of the specified map
 * @param line the buffer to store the string. This will be a 
 *        full string with no chars of 0x00 replaced by a space.
 * @param len the length of the buffer.
 * @param mapIndex the index of the map. If a number that is less than zero
 *   specified, the body of the current map.
 * @return the length of the string, but also null terminate the line.
 */
int
SBMap::getName (char* line, int len, int mapIndex)
{
  if (status==false || maps == 0) return 0;
  if (mapIndex <0)
  {
    return packString (line, len, name, 32);
  }
  return packString (line, len, maps[mapIndex]->name, 32); 
}

/**
 * Get the name of the specified map
 * @param line the buffer to srore the string. This will be a 
 *        full string with no chars of 0x00 replaced by a space.
 * @param len the length of the buffer.
 * @param mapIndex the index of the map. If a number that is less than zero
 *   specified, the body of the current map.
 * @return the length of the string, but also null terminate the line.
 */
int
SBMap::getComment (char* line, int len, int mapIndex)
{
  if (status==false || maps == 0) return 0;
  if (mapIndex <0)
  {
    return packString (line, len, comment, commentSize);
  }
  return packString (line, len, maps[mapIndex]->comment, 
    maps[mapIndex]->commentSize); 
}

/**
 * pack a string into a buffer.
 * @param line is the string to pack to
 * @param len the length of line
 * @param input is the input buffer.
 * @param maxlen is the length in input buffer.
 */
int
SBMap::packString (char* line, int len, const unsigned char* input, int maxlen)
{
  int  i,index;
  int  last = -1;

  index=0;
  for (i=0; i<maxlen; i++)
  {
    if (index > len+1) break;
    if (last==0 && last == (int) input[i]) continue;
    last = (int) input[i];
    if (last == 0)
    {
      line[index++] = ' ';
    }
    else
    {
      line[index++] = last;
    }
  }
  if (last==0)
  {
    line[index-1] = 0;
  }
  else
  {
    line[index] = 0;
  }
  return index;

}

/**
 * build a new state machine.
 * @param the map to build the state machine for, -1 meas all.
 */ 
void
SBMap::buildStateMachine(int mapIndex)
{
  if (mapIndex < 0)
  {
    int i;
    for (i=0; i<mapSize; i++)
    {
      maps[i]->buildStateMachine();
    }
  }
  else
  {
    maps[mapIndex]->buildStateMachine ();
  }
}

/**
 * Return the state machine if we have one.
 * @param mapIndex is the index of the SBMapItem.
 */
const unsigned char*
SBMap::getStateMachine (int mapIndex)
{
  if (status ==false) return 0;
  return maps[mapIndex]->stateMachine;
}

/**
 * get rid of all state machines.
 */
void
SBMap::strip ()
{
  int  i;
  for (i=0; i<mapSize; i++)
  {
    maps[i]->strip();
  }
}

/**
 * serialize the current map
 * @param format is the file format. The possible values:
 * <ul>
 *  <li> SS_BINRAY - The most compact and fast map format </li>
 *  <li> SS_CTEXT - The compact and fast map format turned into a C program 
 *                  as comma separated hex char values. </li>
 *  <li> SS_TEXT - The disassembled binary text with TAGS.</li>
 * </ul>
 * The <b> SS_TEXT </b> '#' are comments.
 * <ul>
 *   <li> <B>COMM=</b> This will be stored in the file. 
 *         Multiple definition possible - multi-line comment.</li>
 *   <li> <B>NAME=</b> A maximum 32-byte-long name. </li>
 *   <li> <B>SECTION=</b> A maximum 32-byte-long section name. </li>
 *   <li> <B>KEY_WIDTH=</b> a size hint for the values on the left of the line  0,1,2,3. Default 0.</li>
 *   <li> <B>VALUE_WIDTH=</b> a size hint for the value on the right right 0,1,2,3. Default 0.</li>
 *   <li>  bytes -> bytes a map line.
 * </ul>
 * @param _fd is the file descriptor
 */
int
SBMap::serialize (SOutputStream&  is, SFileFormat _format)
{
  int  index = 0;
  int  w;
  int  i;

  SWriter writer(is);

  if (status == false) return -1;
  if (mapSize == 0) return 0;
  SString nw;
  SString* _fd = &nw;

  if (_format==SS_BUMAP || _format==SS_CUMAP)
  {
    int res =  serializeUMAP (_fd, _format);
    if (res > 0)
    {
      writer.write (nw);
      if (!writer.isOK()) return -1;
    }
    return res;
  }

  // Serialize header.
  if (sizeof (magicNN) != 16)
  {
    fprintf (stderr, "SBMap::serialize - something is wrong.\n");
    return -1;
  }
  // magicNN
  w = write1 (_fd, (const unsigned char*) magicNN, sizeof (magicNN), _format, FT_MAGIC, SS_LAST);
  
  if (w<0) return -1; 
  index += w;

  // Name
  if (name==0)
  {
    unsigned char nm[32];
    memset (nm, 0, 32);
    strcpy ((char*) nm, "UNNAMED");
    w = write1 (_fd, nm, 32, _format, FT_NAME, SS_LAST);
  }
  else
  {
    w = write1 (_fd, name, 32, _format, FT_NAME, SS_LAST);
  }
  if (w<0) return -1; 
  index += w;

  // Comment size
  w = write32 (_fd, (SS_WORD32) commentSize, _format, FT_IGNORE, SS_NORMAL);
  if (w<0) return -1; 
  index += w;

  // Comment
  if (commentSize>0)
  {
    w = write1 (_fd, comment, commentSize, _format, FT_COMMENT, SS_LAST);
    if (w<0) return -1; 
    index += w;
  }

  // Map type
  w = write32 (_fd, (SS_WORD32) mapType, _format, FT_MAP_TYPE, SS_LAST);
  if (w<0) return -1; 
  index += w;

  // Map size
  w = write32 (_fd, (SS_WORD32) mapSize, _format, FT_IGNORE, SS_LAST);
  if (w<0) return -1; 
  index += w;

  // SS_WORD32 indeces = 16 + 32 + 4 + commentSize + 4 + 4 + 4 * mapSize + 4;
  SS_WORD32 indeces = 0;

  // Write array of indeces
  for (i=0; i<mapSize; i++)
  {
    // If is is a bumap we need to convert it...
    maps[i]->convertFromBumap();
    // size
    w = write32 (_fd, (SS_WORD32) indeces, _format, FT_IGNORE,  SS_NORMAL);
    if (w<0) return -1; 
    index += w;

    indeces += maps[i]->getSerializeSize();
  }
  // Last index
  w = write32 (_fd, (SS_WORD32) indeces, _format, FT_IGNORE, SS_NORMAL);
  if (w<0) return -1; 
  index += w;

  // Now really serialize 'em one by one.
  for (i=0; i<mapSize; i++)
  {
    w = maps[i]->serialize (_fd, _format, (int) (i==mapSize-1));
    if (w<0) return -1; 
    index += w;
    // Stupidity check.
    if (w != maps[i]->getSerializeSize())
    {
      fprintf (stderr, "SBMap::serialize - something is wrong.\n");
      return -1;
    }
  }
  if (index > 0)
  {
    writer.write (nw);
    if (!writer.isOK()) return -1;
  }
  return index;
}

/**
 * serialize the current map as UMAP or bumap.
 * It simply takes the first mapitem and reverses it to create
 * the reverse map.
 * @param format is the file format. The possible values:
 * <ul>
 *  <li> SS_UMAP - c style umap file, constructor of SBMap </li>
 *  <li> SS_BUMAP - binary umap file. </li>
 * </ul>
 * @param _fd is the file descriptor
 */
int
SBMap::serializeUMAP (SString* _fd, SFileFormat _format)
{
  int    w;
  SBMapItem*  mapItem =0;

  // Serialize header.
  if (sizeof (magicBM) != 16)
  {
    fprintf (stderr, "SBMap::serializeUMAP - magicBM is wrong.\n");
    return -1;
  }
  int    index = 0;
  // magicNN
  w = write1 (_fd, (const unsigned char*) magicBM, sizeof (magicBM), _format, FT_MAGIC, SS_LAST);
  
  if (w<0) return -1; 
  index += w;

  // Name
  if (name==0)
  {
    unsigned char nm[32];
    memset (nm, 0, 32);
    strcpy ((char*) nm, "UNNAMED");
    w = write1 (_fd, nm, 32, _format, FT_NAME, SS_LAST);
  }
  else
  {
    w = write1 (_fd, name, 32, _format, FT_NAME, SS_LAST);
  }

  // Comment size
  w = write16 (_fd, (SS_WORD16) commentSize+(16+32+2), _format, FT_IGNORE, SS_NORMAL);

  if (w<0) return -1; 
  index += w;

  // Comment
  if (commentSize>0)
  {
    w = write1 (_fd, comment, commentSize, _format, FT_COMMENT, SS_LAST);
    if (w<0) return -1; 
    index += w;
  }


  // First we need to write the decode, then the encode 
  SS_WORD16* mat[2];
  mat[0] = new SS_WORD16[0x10000];
  CHECK_NEW (mat[0]);
  mat[1] = new SS_WORD16[0x10000];
  CHECK_NEW (mat[1]);
  mapItem = maps[0];

  mapItem->convertFromBumap();

  int  ind, rind;
  ind=0;  rind=1;
  if (mapItem->encode)
  {
    ind = 1;
  }
  if (ind) rind = 0;
  memset (mat[0], 0, 0x10000 * sizeof (SS_WORD16));
  memset (mat[1], 0, 0x10000 * sizeof (SS_WORD16));

  unsigned int matched;
  unsigned int keylen;
  unsigned int vlelen;
  const unsigned char* ckey;
  const unsigned char* cvle;
  SS_WORD64 key;  
  SS_WORD64 vle;  
  unsigned int i=0;
  unsigned int j=0;
  for (i=0; i<mapItem->codeSize; i++)
  {
    ckey = mapItem->getKey (i, &keylen, &matched);
    if (matched!=keylen)
    {
      fprintf (stderr, "SBMap::serializeUMAP map contains substrings\n");
      break;
    }
    // 1 word!
    if (keylen > (unsigned) (1 << mapItem->inWordSize))
    {
      fprintf (stderr, "SBMap::serializeUMAP map contains strings\n");
      break;
    }
    
    key=0;
    for (j=0; j<keylen; j++)
    {
      key = ckey[j] + (key<<8);
    }
    cvle = mapItem->getValue (i, &vlelen);
    if (vlelen > (unsigned) (1 << mapItem->outWordSize))
    {
      fprintf (stderr,  "SBMap::serializeUMAP map contains strings\n");
      break;
    }
    vle=0;
    for (j=0; j<vlelen; j++)
    {
      vle = cvle[j] + (vle<<8);
    }
    if (key > 0xffff || vle > 0xffff)
    {
      fprintf (stderr,"SBMap::serializeUMAP map contains big integers\n");
      break;
    }
    mat[ind][(int)key]=(SS_WORD16) vle;
    mat[rind][(int)vle]=(SS_WORD16) key;
  }
  if (i<mapItem->codeSize)
  {
    delete mat[0];
    delete mat[1];
    return -1;
  }
  // Determine high and low.
  SS_WORD16 high[2][2];
  SS_WORD16 low[2][2];
  high[0][0] = 255; high[0][1] = 0; high[1][0] = 255; high[1][1] = 0;
  low [0][0] = 255; low[0][1] = 0; low [1][0] = 255; low[1][1] = 0;
  
  unsigned int  tmp;
  // Key 0 and value 0 'does not compute'
  static bool warned = false;
  for (j=0; j<2; j++) for (i=0; i<0x10000; i++)
  {
    /* previously thought I will use the opposite
       array - but it could be used only for reversible
       maps. So we have to use OUR OWN array. gaspar */
    //if (i==0) mat[1-j][i] = 0;
    //tmp = mat[1-j][i];
    tmp = mat[j][i];
    if (tmp==0)
    {
      continue;
    }
    if (!warned && mat[1-j][tmp] != i)
    {
       fprintf (stderr, 
          "warning: bumap[%u] not reversible at 0x%04X -> 0x%04X\n", 
          j, i, tmp);
       warned = true;
    }
    if ((i&0xff) < low[j][0]) low[j][0] = (i&0xff);
    if ((i&0xff) > low[j][1]) low[j][1] = (i&0xff);
    if (((i>>8)&0xff) < high[j][0]) high[j][0] = ((i>>8)&0xff);
    if (((i>>8)&0xff) > high[j][1]) high[j][1] = ((i>>8)&0xff);
  }
  // OK we can write decode bounds.
  // Map size
  for (i=0;i<2;i++)
  {
    w = write16 (_fd, high[i][0], _format, FT_IGNORE, SS_LAST);
    if (w<0) { delete mat[0]; delete mat[1]; return -1; } index += w;
    w = write16 (_fd, high[i][1], _format, FT_IGNORE, SS_LAST);
    if (w<0) { delete mat[0]; delete mat[1]; return -1; } index += w;
    w = write16 (_fd, low[i][0], _format, FT_IGNORE, SS_LAST);
    if (w<0) { delete mat[0]; delete mat[1]; return -1; } index += w;
    w = write16 (_fd, low[i][1], _format, FT_IGNORE, SS_LAST);
    if (w<0) { delete mat[0]; delete mat[1]; return -1; } index += w;
  }

  // Now the two bounded arrays.
  int k;
  SS_LineEnd lend = SS_LAST;
  for (i=0;i<2;i++)
  {
    for (j=high[i][0]; j<=high[i][1]; j++)
    {
      for (k=low[i][0]; k<=low[i][1]; k++)
      {
        if (i==1 && j==high[i][1] && k==low[i][1])
        {
          lend = SS_EOF;
        }
        w = write16 (_fd, mat[i][(j<<8)+k], _format, FT_IGNORE, lend);
        if (w<0) { delete mat[0]; delete mat[1]; return -1; } index += w;
      }
    }
  }
  delete mat[0]; delete mat[1];
  return index;
}

#define STORE_STATE(_state, _bucket, _v) \
   stateMachineBuffer [_state * 64 + (4 * _bucket)+0]=(_v>>24)&0xff; \
   stateMachineBuffer [_state * 64 + (4 * _bucket)+1]=(_v>>16)&0xff; \
   stateMachineBuffer [_state * 64 + (4 * _bucket)+2]=(_v>>8)&0xff; \
   stateMachineBuffer [_state * 64 + (4 * _bucket)+3]=(_v>>0)&0xff
/**
 * build a new state machine.
 */ 
void
SBMapItem::buildStateMachine()
{
  if (itemType != SBMapNToN)
  {
    return;
  }
  if (stateMachineType == SS_DYNAMIC)
  {
    delete stateMachineBuffer;
  }
  stateMachine = 0;
  stateMachineType = SS_DYNAMIC;
  stateMachineSize = 1;
  stateMachineBufferSize = 1;
  stateMachineBuffer = new unsigned char[16*4];
  CHECK_NEW (stateMachineBuffer);

  int    pos;
  SS_WORD32  stt;
  int    from = 0;
  int    to = codeSize;

  for (pos=0; pos<16; pos++)
  {
    stt = addState ((SS_WORD32) REJECT<<30, 
      (const unsigned char) pos,
      0, from, to);
    STORE_STATE (0, pos, stt);
  }
  stateMachine = stateMachineBuffer;
}

/**
 * @author Gaspar Sinai <gsinai@iname.com> Tokyo 1999.12.18
 * addState - add a new state to the state machine.
 * @param oldState - the state carried forward till now.
 * @param in - the input character nibble.
 * @pos - the current position * 2, it is divisabel by 2 then 
 *        it is the upper nibble and the start of a char.
 * @
 * @return oldState if no new state matched, otherwise nextstate or 
 * nextindex.
 * <ul>
 *  <li> REJECT - return oldState; </li>
 *  <li> MORE - return index of more iterations (next state). </li>
 *  <li> MATCH_MORE - not used. </li>
 *  <li> MATCH - return index of this matched iteration.  </li>
 * </ul>
 * REJECT=0<<30, MORE=1<<30, MATCH_MORE=2<<30, MATCH=4<<30
 */
SS_WORD32 
SBMapItem::addState(SS_WORD32 oldState, const unsigned char in, unsigned int pos, 
      int _from, int _to)
{

  if (stateMachineSize >= stateMachineBufferSize)
  {
    int     newSize;
    unsigned char* newBuffer;

    newSize = stateMachineBufferSize << 1;
    newBuffer = new unsigned char[newSize * 16 * 4];
    CHECK_NEW (newBuffer);
    memcpy (newBuffer, stateMachineBuffer, 
      stateMachineBufferSize * 16 * 4);
    delete stateMachineBuffer;
    stateMachineBuffer = newBuffer;
    stateMachineBufferSize = newSize;
  }
  int    current = stateMachineSize;
  stateMachineSize++;

  int     i, matches=0;
  SS_WORD32  stt=0;

  SFound    found;
  int    from, to;
  unsigned int    newPos = pos+1;

  for (i=0; i<16; i++)
  {
    from = _from; to = _to;

    // If divisible by 2 it means that this is the 
    // second nibble and we may have to do some work...
    // Determine what STT to pass. That can be MATCH,
    // REJECT or MORE.
    if (newPos&1)
    {
      found = find (((in << 4) | i), pos >> 1, &from, &to);
      if (found == REJECT)
      {
        // Depending on old state it is a reject
        // or an old match
        stt = (oldState >> 30);
        if (stt==MATCH)
        {
          STORE_STATE (current, i, oldState);
          matches++;
        }
        else
        {
          stt = (SS_WORD32) REJECT << 30;
          STORE_STATE (current, i, stt);
        }
        continue;
      }
      if (found == MATCH)
      {
        stt = from;
        stt |= ((SS_WORD32) MATCH << 30);
        STORE_STATE (current, i, stt);
        matches++;
        continue;
      }
      if (found == MATCH_MORE)
      {
        stt = from;
        stt |= ((SS_WORD32) MATCH << 30);
        matches++;
      }
      if (found == MORE)
      {
        stt = (oldState >> 30);
        // Don't downgrade match;
        if (stt==MATCH)
        {
          stt = oldState;
        }
        else
        {
          // Upgrade reject.
          stt |= ((SS_WORD32) MORE << 30);
        }
        matches++;
      }
    }
    else
    {
      stt = oldState;
    }
    stt = addState ((SS_WORD32) stt, 
        (const unsigned char) i,
        newPos, from, to);

    STORE_STATE (current, i, stt);
    if ((stt>>30) != REJECT) matches++;
  }
  //
  // Save a great deal of memory here.
  //
  if (matches==0)
  {
    stateMachineSize = current;
    stt = (oldState >> 30);
    if (stt==REJECT || stt==MORE)
    {
      return  (SS_WORD32) REJECT << 30;
    }
    else
    {
      stt = oldState & 0x3fffffff;
      stt |= (SS_WORD32) MATCH << 30;
      return stt;
    }
  }
  stt = current & 0x3fffffff;
  stt |= ((SS_WORD32) MORE << 30);
  return stt;
}

/**
 * serialize the current map
 */
int
SBMapItem::serialize (SString* _fd, SFileFormat _format, int _last)
{
  if (itemType != SBMapNToN)
  {
    return (0);
  }
  int w;
  unsigned index;
  unsigned char out;
  SS_WORD32 lastIndex;
  unsigned int  i;
  int  fullIndex;
  
  // Serialize the body and the additional state machine.
  // CODE AREA.
  index = 0;


  // Name
  w = write1 (_fd, name, 32, _format, FT_SECTION, SS_LAST);
  if (w<0) return -1; 
  index += w;

  // Comment size
  w = write32 (_fd, (SS_WORD32) commentSize, _format, FT_IGNORE, SS_NORMAL);
  if (w<0) return -1; 
  index += w;

  // Comment
  w = write1 (_fd, comment, commentSize, _format, FT_COMMENT, SS_LAST);
  if (w<0) return -1; 
  index += w;

  // decode/encode
  out = encode;
  w = write8 (_fd, out, _format, FT_IS_ENCODE, SS_LAST);
  if (w<0) return -1; 
  index += w;
  
  // Input word size.
  out = inWordSize;
  w = write8 (_fd, out, _format, FT_SIZE_FROM, SS_LAST);
  if (w<0) return -1; 
  index += w;

  // Output word size.
  out = outWordSize;
  w = write8 (_fd, out, _format, FT_SIZE_TO, SS_LAST);
  if (w<0) return -1; 
  index += w;

  // Input length indicator size.
  out = inByteLength;
  w = write8 (_fd, out, _format, FT_LENGTH_FROM, SS_LAST);
  if (w<0) return -1; 
  index += w;

  // Output length indicator size.
  out = outByteLength;
  w = write8 (_fd, out, _format, FT_LENGTH_TO, SS_LAST);
  if (w<0) return -1; 
  index += w;

  // Pointer to a state machine.
  // First we need to know our own length...
  // Index from base.
  lastIndex = baseSize;

  if (stateMachineSize >0 && stateMachine != 0)
  {
    w = write32 (_fd, lastIndex, _format, FT_IGNORE, SS_LAST);
    if (w<0) return -1; 
    index += w;
  }
  else
  {
    w = write32 (_fd, 0, _format, FT_IGNORE, SS_LAST);
    if (w<0) return -1; 
    index += w;
  }

  // Spare...
  w = write32 (_fd, 0, _format, FT_IGNORE, SS_LAST);
  if (w<0) return -1; 
  index += w;

  // Write out the array of indeces.
  // First the size.

  w = write32 (_fd, codeSize, _format, FT_IGNORE, SS_NORMAL);
  if (w<0) return -1; 
  index += w;

  const unsigned char* mp;
  fullIndex=0;
  for (i=0; i<codeSize; i++)
  {
    // Have to rewrite index!
    //mp = &codeMap[i<<2];
    //lastIndex = TO32(mp);

    lastIndex = fullIndex;
    
    w = write32 (_fd, lastIndex, _format, FT_IGNORE, SS_LAST);
    if (w<0) return -1; 
    index += w;
    fullIndex += getLength(i);
  }
  
  w = write32 (_fd, baseSize, _format, FT_IGNORE, SS_LAST);
  if (w<0) return -1; 
  index += w;

  //* Write out the array itself
  for (i=0; i<codeSize; i++)
  {
    mp = &codeMap[i<<2];
    lastIndex = TO32(mp);
    if (stateMachineSize==0)
    {
      w=writeCodeArea(_fd, i, _format,
        (_last && i==codeSize-1) ? SS_EOF : SS_LAST);
      if (w<0) return -1;
      index += w;
      if (w != (int) getLength(i))
      {
        fprintf (stderr, "internal error\n");
        return -1;
      }
    }
    else
    {
      w=writeCodeArea(_fd, i, _format, SS_LAST);
      if (w<0) return -1; 
      index += w;
      if (w != (int) getLength(i))
      {
        fprintf (stderr, "internal error\n");
        return -1;
      }
    }
  }


  // Write out the state machine
  if (stateMachineSize >0 && stateMachine != 0)
  {
    w = write32 (_fd, stateMachineSize, 
        _format, FT_IGNORE, SS_LAST);

    if (w<0) return -1; 
    index += w;
    w = write1 (_fd, stateMachine, 
      stateMachineSize * 4 * 16, _format, FT_IGNORE, 
      (_last) ? SS_EOF : SS_LAST);

    if (w<0) return -1; 
    index += w;
  }
  return index;
}


/**
 * write a formatted text with a length.
 * @param _fd is the file descriptor to write to
 * @param _buf is the buffer tow write.
 * @param _length is the length in bytes (not in words!)
 * @param _slash is the length where slash is needed.
 * @param _format is the file format.
 * @param _last shows how to end the line.
 */
int
SBMapItem::writeTextBytes (SString* _fd, const unsigned char *_buf,
  int _length, int _slash, int _wordSize, SFileFormat _format, SS_LineEnd _last)
{
  int  i;
  const unsigned char*  cmnt;
  SS_WORD16  w16;
  SS_WORD32  w32;
  SS_LineEnd  lend;
  int    w;

  int  ws = (1<<_wordSize);

  if ((_length%ws) != 0)
  {
    fprintf (stderr, "MapItem::writeTextBytes: byte-length error.");
    fprintf (stderr, "Word=%d", ws);
    fprintf (stderr, "bytes, Length=%dbytes\n", _length);
  }
  if (_format!=SS_TEXT_MAP)
  {
    lend = SS_NORMAL;
    for (i=0; i<_length; i++)
    {
      if (i==_length-1) lend = _last;

      w = write8 (_fd, _buf[i], _format, FT_MAX, lend);
      if (w<0) return -1;
    }
    return _length;
  }


  if ((_length%ws) != 0)
  {
    cmnt  = (const unsigned char*) "#### WRONG LENGTH bytes:";
    w = write1 (_fd, cmnt, strlen ((const char*)cmnt), 
      _format, FT_MAX, SS_NORMAL);
    if (w<0) return -1;

    return writeTextBytes (_fd, _buf, _length, _slash, 0, 
        _format, SS_LAST);
  }

  if (_length==0)
  {
    cmnt  = (const unsigned char*) "";
    w = write1 (_fd, cmnt, 0, _format, FT_MAX, _last);
    if (w<0) return -1;
    return _length;
  }

  lend = SS_NORMAL;
  for (i=0; i<_length; i+=ws)
  {
    if (i==_slash && _format==SS_TEXT_MAP)
    {
      cmnt  = (const unsigned char*) "/ ";
      w = write1 (_fd, cmnt, 1, _format, FT_MAX, SS_NORMAL);
      if (w<0) return -1;
    }
    if (i+ws>=_length)
    {
      lend = _last;
    }
    switch (ws)
    {
    case 1:
      w = write8 (_fd, _buf[i], _format, FT_MAX, lend);
      break;
    case 2:
      cmnt = &_buf[i];
      w16 = TO16(cmnt);
      w = write16 (_fd, w16, _format, FT_MAX, lend);
      break;
    case 4:
      cmnt = &_buf[i];
      w32 = TO32(cmnt);
      w = write32 (_fd, w32, _format, FT_MAX, lend);
      break;
    case 8:
    default:
      fprintf (stderr, "writing 8 bytes is not implemented.\n");
      cmnt = &_buf[i];
      w32 = TO32(cmnt);
      w = write32 (_fd, w32, _format, FT_MAX, lend);
      break;
    }
    if (w<0) return -1;
  }
  return _length;
}

/**
 * write out the code area according to format.
 * @param _fd is the output file descriptor
 * @param _index is the array index of the code area
 * @param _format is the file format  SS_BINARY, SS_TEXT_MAP, SS_CTEXT
 * @param _last indicates that there will be no more data written after this.
 */
int
SBMapItem::writeCodeArea (SString* _fd, int _index, SFileFormat _format, SS_LineEnd _last)
{
  const unsigned char*  vle = &codeMap[_index<<2];

  unsigned int index = TO32(vle);
  unsigned int partLen;
  unsigned int fullLen;
  unsigned int slash;
  const unsigned char* cmnt;
  int  i, w;

  const unsigned char* starts = &base[index];
  fullLen = 0;

  // The first n bytes are length of string
  partLen=0; i = 1 << inByteLength;
  while (i-->0)
  {
    w = write8 (_fd, starts[fullLen], _format, FT_IGNORE, SS_NORMAL);
    if (w<0) return -1;
    partLen = starts[fullLen++] + (partLen<<8);
  }
  slash=0; i = 1 << inByteLength;
  while (i-->0)
  {
    w = write8 (_fd, starts[fullLen], _format, FT_IGNORE, SS_NORMAL);
    if (w<0) return -1;
    slash = starts[fullLen++] + (slash<<8);
  }

  w = writeTextBytes (_fd, &starts[fullLen], partLen, slash, 
    inWordSize, _format, SS_NORMAL);
  if (w<0) return -1;
  fullLen += w;

  if (_format==SS_TEXT_MAP)
  {
    cmnt  = (const unsigned char*) "\t-> ";
    w = write1 (_fd, cmnt, strlen ((const char*) cmnt),_format, FT_MAX, SS_NORMAL);
    if (w<0) return -1;
  }

  // Second part.
  partLen=0; i = 1 << outByteLength;
  while (i-->0)
  {
    w = write8 (_fd, starts[fullLen], _format, FT_IGNORE, SS_NORMAL);
    if (w<0) return -1;
    partLen = starts[fullLen++] + (partLen<<8);
  }

  w = writeTextBytes (_fd, &starts[fullLen], partLen, partLen, 
    outWordSize, _format, SS_NORMAL);
  if (w<0) return -1;
  fullLen += w;

  // Third part
  if (starts[fullLen] == 0)
  {
    w = write8 (_fd, starts[fullLen], _format, FT_IGNORE, _last);
  }
  else
  {
    w = write8 (_fd, starts[fullLen], _format, FT_IGNORE, 
      SS_NORMAL);
  }
  if (w<0) return -1;

  partLen = starts[fullLen++];

  if (_format == SS_TEXT_MAP)
  {
    if (partLen!=0)
    {
      cmnt  = (const unsigned char*) " # ";
      w = write1 (_fd, cmnt, strlen ((const char*)cmnt), _format, FT_MAX, SS_NORMAL);
    }
    else
    {
      cmnt  = (const unsigned char*) "";
      w = write1 (_fd, cmnt, 1, _format, FT_MAX, _last);
    }
    
    if (w<0) return -1;
  }
  if (partLen>0)
  {
    w = write1 (_fd, &starts[fullLen], partLen,  _format, FT_MAX, _last);
    if (w<0) return -1;
    fullLen += w;
  }

  return fullLen;
}

/**
 * size of map when serialized.
 */
int
SBMapItem::getSerializeSize ()
{
  if (stateMachineSize>0)
  {
    return (32+4+commentSize+5+4+4+4*(codeSize+2)+baseSize
      +4+stateMachineSize*4*16);
  }
  return (32+4+commentSize+5+4+4+4*(codeSize+2)+baseSize);
  
}

/**
 * write bytes - safe version
 * @param fd - file descriptor
 * @in - input bytes
 * @size - size of input.
 * @param _format is the file format the data is written,
 */
static int 
writeBytes(SString* _fd, const unsigned char* in, int size)
{
  if (size<=0) return 0;

  if (size==0) return 0;
  _fd->append ((const char*)in, (unsigned int)size);
  return size;
}

/**
 * Write a charatcer stream.
 * @param _fd the file descriptor
 * @param out the bytebuffer
 * @param size the size of buffer
 * @param _format is the file format the data is written,
 * @param ftype is the type if data
 */
static int
write1(SString* _fd, const unsigned char* in, int size, SFileFormat _format, const SS_FieldType ftype, const SS_LineEnd lend)
{
  int     i, index, written;
  const char*   pref;
  int    newLine;
  SS_LineEnd  nl;

  index = 0;
  switch (_format)
  {
  case SS_BINARY:
  case SS_BUMAP:
    written =  writeBytes (_fd, in, size);
    return written;
    break;

  // We strip 0's.;
  case SS_TEXT_MAP:
    if (ftype==FT_IGNORE) return size;
    newLine=1;
    while (index<size) 
    {
      if (newLine)
      {
        pref=textMapPrefix[ftype];
        if (writeBytes (_fd, (const unsigned char*) pref, 
          strlen (pref)) <  0)
        {
           return -1;
        }
        newLine=0;
      }
      // filter-out new lines.
      // Do not write 0,s.
      while(index<size && in[index]==0) index++;
      i = index;

      while(i<size && in[i]!=0)
      {
        if (in[i++]=='\n')
        {
          newLine =1;
          break;
        }
      }
      written = writeBytes (_fd, &in[index], i-index);
      if (written < 0) return -1;
      index = i;
    }
    if (lend == SS_LAST || lend == SS_EOF)
    {
      written = writeBytes (_fd, (const unsigned char*) "\n", 1);
      if (written < 0) return -1;
    }
    break;

  // Break it into chunks and send it.
  case SS_CTEXT:
  case SS_CUMAP:
  default:
    pref=cMapPrefix[ftype];
    if (writeBytes (_fd, (const unsigned char*) pref, 
      strlen (pref)) <  0)
    {
       return -1;
    }
    for (i=0; i<size; i++)
    {
      if (i+1 == size)
      {
        nl = lend;
      }
      else if (((i+1)%8)==0)
      {
        nl = SS_LAST;
      }
      else
      {
        nl = SS_NORMAL;
      }
      written = write8 (_fd, in[i], _format, FT_MAX, nl);
      if (written < 0) return -1;
    }
    break;
  }
  return size;
}

/**
 * Write a 16 bit integer in network order to output stream.
 * @param _fd the file descriptor
 * @param number is the integer in host order.
 * @param _format is the file format the data is written,
 * @param ftype is the type if data
 */
static int
write8 (SString* _fd, SS_WORD8 number, SFileFormat _format, const SS_FieldType ftype, const SS_LineEnd lend)
{
  unsigned char  out[64];
  const char*  pref;
  switch (_format)
  {
  case SS_BINARY:
  case SS_BUMAP:
    out[0] = number;
    return writeBytes (_fd, out, 1);

  case SS_TEXT_MAP:
    if (ftype == FT_IGNORE) return 1;
    pref=textMapPrefix[ftype];
    if (number > ' ' && number < 'z' && number != '#')
    {
      snprintf ((char*)out, 64, "'%c", number);
    }
    else
    {
      snprintf ((char*)out, 64, "%02X", number);
    }
    if (lend==SS_LAST || lend==SS_EOF)
    {
      strcat ((char*)out, "\n");
    }
    else
    {
      strcat ((char*)out, " ");
    }
    
    if (writeBytes (_fd, (const unsigned char*) pref, strlen(pref)) < 0)
    {
      return -1;
    }
    if (writeBytes (_fd, out, strlen((const char*)out)) > 0)
    {
      return 1;
    }
    return -1;
  case SS_CTEXT:
  case SS_CUMAP:
  default:
    pref=cMapPrefix[ftype];
    if (lend == SS_LAST)
    {
      snprintf ((char*)out, 64, "0x%02x,\n", number);
    }
    else if (lend == SS_EOF)
    {
      snprintf ((char*)out, 64, "0x%02x\n", number);
    }
    else
    {
      snprintf ((char*)out, 64, "0x%02x, ", number);
    }
    if (writeBytes (_fd, (const unsigned char*) pref, strlen(pref)) < 0)
    {
      return -1;
    }
    if (writeBytes (_fd, out, strlen((const char*)out)) > 0)
    {
      return 1;
    }
    return -1;
  }
  /*NOTREACHED*/
  return -1;
}

/**
 * Write a 16 bit integer in network order to output stream.
 * @param _fd the file descriptor
 * @param number is the integer in host order.
 * @param _format is the file format the data is written,
 * @param ftype is the type if data
 */
static int
write16 (SString* _fd, SS_WORD16 number, SFileFormat _format, const SS_FieldType ftype, const SS_LineEnd lend)
{
  unsigned char  out[64];
  unsigned char  num[2];
  const char*  pref;

  switch (_format)
  {
  case SS_BINARY:
  case SS_BUMAP:
    num[0] = ((number >> 8) & 0xff);
    num[1] = ((number >> 0) & 0xff);
    return writeBytes (_fd, num, 2);

  case SS_TEXT_MAP:
    if (ftype == FT_IGNORE) return 2;
    pref=textMapPrefix[ftype];
    snprintf ((char*)out, 64, "%04X", number);
    if (lend == SS_LAST || lend == SS_EOF)
    {
      strcat ((char*)out, "\n");
    }
    else
    {
      strcat ((char*)out, " ");
    }
    
    if (writeBytes (_fd, (const unsigned char*) pref, strlen(pref)) < 0)
    {
      return -1;
    }
    if (writeBytes (_fd, out, strlen((const char*)out)) > 0)
    {
      return 2;
    }
    return -1;
  case SS_CTEXT:
  case SS_CUMAP:
  default:
    num[0] = ((number >> 8) & 0xff);
    num[1] = ((number >> 0) & 0xff);
    pref=cMapPrefix[ftype];
    if (lend == SS_LAST)
    {
      snprintf ((char*)out, 64, "0x%02x, 0x%02x,\n", 
        (int) num[0], (int) num[1]);
    }
    else if (lend ==SS_EOF)
    {
      snprintf ((char*)out, 64, "0x%02x, 0x%02x\n",
        (int) num[0], (int) num[1]);
    }
    else
    {
      snprintf ((char*)out, 64, "0x%02x, 0x%02x, ", 
        (int) num[0], (int) num[1]);
    }
    if (writeBytes (_fd, (const unsigned char*) pref, strlen(pref)) < 0)
    {
      return -1;
    }
    if (writeBytes (_fd, out, strlen((const char*)out)) > 0)
    {
      return 2;
    }
    return -1;
  }
  /*NOTREACHED*/
  return -1;
}

/**
 * Write a 32 bit integer in network order to output stream.
 * @param _fd the file descriptor
 * @param number is the integer in host order.
 * @param _format is the file format the data is written,
 * @param ftype is the type if data
 */
static int
write32 (SString* _fd, SS_WORD32 number, SFileFormat _format, const SS_FieldType ftype, const SS_LineEnd lend)
{
  unsigned char  out[64];
  unsigned char  num[4];
  const char*  pref;

  switch (_format)
  {
  case SS_BINARY:
  case SS_BUMAP:
    num[0] = ((number >> 24) & 0xff);
    num[1] = ((number >> 16) & 0xff);
    num[2] = ((number >> 8) & 0xff);
    num[3] = ((number >> 0) & 0xff);
    return writeBytes (_fd, num, 4);
  case SS_TEXT_MAP:
    if (ftype == FT_IGNORE) return 4;
    pref=textMapPrefix[ftype];
    snprintf ((char*)out, 64, "%08X", number);
    if (lend == SS_LAST || lend == SS_EOF)
    {
      strcat ((char*)out, "\n");
    }
    else
    {
      strcat ((char*)out, " ");
    }
    
    if (writeBytes (_fd, (const unsigned char*) pref, strlen(pref)) < 0)
    {
      return -1;
    }
    if (writeBytes (_fd, out, strlen((const char*)out) ) > 0)
    {
      return 1;
    }
    return -1;
  case SS_CTEXT:
  case SS_CUMAP:
  default:
    pref=cMapPrefix[ftype];
    num[0] = ((number >> 24) & 0xff);
    num[1] = ((number >> 16) & 0xff);
    num[2] = ((number >> 8) & 0xff);
    num[3] = ((number >> 0) & 0xff);
    if (lend == SS_LAST)
    {
      snprintf ((char*)out, 64, "0x%02x, 0x%02x, 0x%02x, 0x%02x,\n", 
        (int) num[0], (int) num[1],
        (int) num[2], (int) num[3]);
    }
    else if (lend == SS_EOF)
    {
      snprintf ((char*)out, 64, "0x%02x, 0x%02x, 0x%02x, 0x%02x\n", 
        (int) num[0], (int) num[1],
        (int) num[2], (int) num[3]);
    }
    else
    {
      snprintf ((char*)out, 64, "0x%02x, 0x%02x, 0x%02x, 0x%02x,", 
        (int) num[0], (int) num[1],
        (int) num[2], (int) num[3]);
    }
    if (writeBytes (_fd, (const unsigned char*) pref, strlen(pref)) < 0)
    {
      return -1;
    }
    if (writeBytes (_fd, out, strlen((const char*)out)) > 0)
    {
      return 4;
    }
    return -1;
  }
  /*NOTREACHED*/
  return -1;
}

/**
 * return key value map to see what decodes to what
 * @param key will contain the keys
 * @param value will contain the values
 * @param _size is the maximum size of returned arrays
 * @return the real size of the arrays.
 */
unsigned int
SBMap::getDecoderMap (SStringVector* key, SStringVector* value,
        unsigned int _size)
{
  int all = getSize();
  int found = -1;
  int i;
  for (i=0; i<all; i++)
  {
    
    if (getType(i) == SBMap_DECODE)
    {
      /* circular maps are not supported */
      if (found >= 0) return 0;
      found = i;
    }
  }
  if (found < 0) return 0;
  key->clear();
  value->clear();
  SBMapItem* map = maps[(unsigned int)found];
  return map->getDecoderMap (key, value, _size);
}
/* SGC For maps with holes */
unsigned int
SBMap::getLinearPosition (unsigned int _index, SS_UCS4 key)
{
  return maps[_index]->getLinearPosition(key);
}
SS_UCS4 
SBMap::getLinearKey (unsigned int _index, unsigned int position)
{
  return maps[_index]->getLinearKey(position);
}
SS_UCS4 
SBMap::getLinearValue (unsigned int _index, unsigned int position)
{
  return maps[_index]->getLinearValue(position);
}


/**
 * return key value map to see what decodes to what
 * @param key will contain the keys
 * @param value will contain the values
 * @param _size is the maximum size of returned arrays
 * @return the real size of the arrays.
 */
unsigned int
SBMapItem::getDecoderMap (SStringVector* key, SStringVector* value,
        unsigned int _size)
{
  if (stateMachine != 0 ) return 0;
  if (inWordSize != 0) return 0;

  SEncoder _encoder("utf-8");
  char kc[32];
  if (itemType == SBMapBumap)
  {
    unsigned int count = 0;
    for (unsigned int i=highMin; i<=highMax; i++)
    {
      for (unsigned int j=lowMin; j<=lowMax; j++)
      {
        SS_UCS4 in = (i << 8) + j;
        unsigned int arrayIndex = ((int)i - highMin)
          * (lowMax - lowMin+1) + ((int)j - lowMin);
        // Array of 16 bit integers
        const unsigned char* mp = &base[arrayIndex<<1];
        SS_UCS4 u4out = TO16 (mp);
        if (u4out==0) continue;
        count++;
        if (count > _size) continue;
        SV_UCS4 ucs4vout;
        ucs4vout.append (u4out);
        if (in <= 0xff)
        {
           snprintf (kc, 32, "=%02X", (unsigned int)in);
        }
        else
        {
           snprintf (kc, 32, "=%04X", (unsigned int)in);
        }
        SString ks(kc);
        SString vs = _encoder.encode (ucs4vout);
        key->append (ks);
        value->append (vs);
      }
    }
    return count;
  }
  unsigned int ksize;
  unsigned int vlesize;
  for (unsigned int i=0; i<codeSize && i<_size; i++)
  {
     const unsigned char* k = getKey (i, &ksize);
     const unsigned char* v = getValue (i, &vlesize);
     SV_UCS4 ucs4v;
     for (unsigned int i=0; i<vlesize; i=i+(1<<outWordSize))
     {
       SS_UCS4 ucs4 = 0;
       switch (outWordSize)
       {
       case 0:
         ucs4 = (SS_UCS4) (v[i]);
         break;
       case 1:
        ucs4 = (SS_UCS4) (v[i]);
        ucs4 = (ucs4 << 8) + (SS_UCS4) (v[i+1]);
        break;
       case 2:
        ucs4 = (SS_UCS4) (v[i]);
        ucs4 = (ucs4 << 8) + (SS_UCS4) (v[i+1]);
        ucs4 = (ucs4 << 8) + (SS_UCS4) (v[i+2]);
         ucs4 = (ucs4 << 8) + (SS_UCS4) (v[i+3]);
         break;
       case 3:
         break;
       default:
         break;
       }
       ucs4v.append (ucs4);
     }
     SString ks ((const char*)k, ksize);
     SString vs = _encoder.encode (ucs4v);
     key->append (ks);
     value->append (vs);
  }
  return codeSize;
}

/**
 * Get the position where key at index is less or equal to input 
 * argument key. This can be used in interpolated linear maps.
 * @param key is the input argument key.
 * @return input position.
 */
unsigned int
SBMapItem::getLinearPosition (SS_UCS4 key)
{
  if (itemType!=SBMapNToN) return 0;
  /* 64 bit map is not supported by getLinearPosition */
  if (inWordSize>2) return 0;
  unsigned int bkeySize = 1<<inWordSize;
  unsigned char* bkey = new unsigned char [bkeySize];
  CHECK_NEW (bkey);
  switch (inWordSize)
  {
  case 0:
    if (key>0xff) { delete[] bkey; return 0;}
    bkey[0] = key&0xff;
    break;
  case 1:
    if (key>0xffff)  { delete[] bkey; return 0;}
    bkey[1] = key&0xff;
    bkey[0] = (key>>8)&0xff;
    break;
  case 2:
    bkey[3] = key&0xff;
    bkey[2] = (key>>8)&0xff;
    bkey[1] = (key>>16)&0xff;
    bkey[0] = (key>>24)&0xff;
    break;
  default:
    delete[] bkey;
    return 0;
  }
  unsigned int from = nextSorted(bkey, bkeySize);
  delete[] bkey;
  if (from ==0) return 0;
  /* nextSorted returns next element  - the insertion point */
  return (from-1);
}

/**
 * Get the key value in SS_UCS4 corresponding to index
 */
SS_UCS4 
SBMapItem::getLinearKey (unsigned int position)
{
  if (itemType!=SBMapNToN) return 0;
  /* 64 bit map is not supported by getLinearKey */
  if (position >= codeSize || inWordSize>2) return 0;
  unsigned int len;
  const unsigned char* key = getKey(position, &len);
  /* Multiple words are  not supported by getLinearKey */
  if (len != (unsigned int)(1<<inWordSize)) return 0;
  SS_UCS4 ret = 0;
  for (unsigned int i=0; i<len; i++) 
  {
    ret = ret << 8;
    ret += key[i];
  }
  return ret;
}

/**
 * Get the decoded value in SS_UCS4 corresponding to index
 */
SS_UCS4 
SBMapItem::getLinearValue (unsigned int position)
{
  if (itemType!=SBMapNToN) return 0;
  /* 64 bit map is not supported by getLinearValue */
  if (position >= codeSize || outWordSize>2) return 0;
  unsigned int len;
  const unsigned char* value = getValue(position, &len);
  /* Multiple words are  not supported by getLinearValue */
  if (len != (unsigned int)(1<<outWordSize)) return 0;
  SS_UCS4 ret = 0;
  for (unsigned int i=0; i<len; i++) 
  {
    ret = ret << 8;
    ret += value[i];
  }
  return ret;
}

/**
 * This routine can be called only if temType==SBMapBumap 
 */
SS_UCS4
SBMap::decode (SS_UCS2 in)
{
  SBMapItem* map = maps[0];
  unsigned int i = in >> 8;
  unsigned int j = in & 0xff;
  if ((int)i<map->highMin || (int)i>map->highMax) return 0;
  if ((int)j<map->lowMin || (int)j>map->lowMax) return 0;
  unsigned int arrayIndex = ((int)i - map->highMin)
     * (map->lowMax - map->lowMin+1) + ((int)j - map->lowMin);
  // Array of 16 bit integers
  const unsigned char* mp = &map->base[arrayIndex<<1];
  SS_UCS4 u4out = TO16 (mp);
  return u4out;
}

/**
 * This routine can be called only if temType==SBMapBumap 
 */
SS_UCS2
SBMap::encode (SS_UCS4 in)
{
  SBMapItem* map = maps[1];
  unsigned int i = in >> 8;
  unsigned int j = in & 0xff;
  if ((int)i<map->highMin || (int)i>map->highMax) return 0;
  if ((int)j<map->lowMin || (int)j>map->lowMax) return 0;
  unsigned int arrayIndex = ((int)i - map->highMin)
     * (map->lowMax - map->lowMin+1) + ((int)j - map->lowMin);
  // Array of 16 bit integers
  const unsigned char* mp = &map->base[arrayIndex<<1];
  SS_UCS2 u4out = TO16 (mp);
  return u4out;
}
bool
SBMap::isUMap()
{
  return (getSize() ==2 
         && maps[0]->itemType==SBMapItem::SBMapBumap
         && maps[1]->itemType==SBMapItem::SBMapBumap);
}
