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
 
/**
 * Generic map (n to n)
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 1999-12-04
 */
#ifndef SBMap_H
#define SBMap_H

#include "stoolkit/STypes.h"
#include "stoolkit/SIO.h"
#include "stoolkit/SIOStream.h"
#include "stoolkit/SStringVector.h"

typedef enum {
  SS_BINARY=0, 
  SS_TEXT_MAP,
  SS_CTEXT,
  SS_KEYMAP,
  SS_BUMAP,
  SS_CUMAP
} SFileFormat;

class SBMapBuffer
{
public:
  SBMapBuffer(int fromWordSize, int toWordSize);
  ~SBMapBuffer();

  void reset ();

  void append (const void* buffer, int length);
  void appendFromNet (const void* buffer, int length);
  void appendToNet (const void* buffer, int length);
  void ensureCapacity (int length);

  // Shift macthed input with size.
  void    shift (int size);


  // The length of data expected in the input.
  // 0 1 2 3
  int  fromWordSize;

  // The length of data in buffer
  // 0 1 2 3
  int  toWordSize;

  // This is the size of one of the buffers below
  int       bufferSize;

  // This is the length of the array. You should do a  toWordSize*length
  int    length;
  union {
    SS_WORD8  *u8;
    SS_WORD16  *u16;
    SS_WORD32  *u32;
    SS_WORD64  *u64;
  } u;

};

class SStateModel
{
public:
  SStateModel (int inFromLength, 
    int inToLength,
    int outFromLength,
    int outToLength);
  ~SStateModel ();  

  void    reset ();


  // State Machine. Do not overwrite it.
  // high is used for state machine state.
  int    low;
  int    high;

  int    nextPos;
  int    lastPos;

  int    lastMatch;

  // These are used for curcular-matches.
  int    circle;
  SS_WORD64  circleResult;
  int    circleCount;
  int    circleSize;
  int    needReset;

  SBMapBuffer    out;
  SBMapBuffer    in;

};

typedef enum {
  SS_NORMAL=0,
  SS_LAST,
  SS_EOF,
  SS_MAX
} SS_LineEnd;

class SBMapItem
{
public:
  // These flags are the apper bits of states in state machine.
  enum   SFound {REJECT=0, MORE, MATCH_MORE, MATCH };
  enum   SBMapItemType {SBMapNToN=0, SBMapBumap}; 

  // This will create an SBMapBumap static Map
  // If matrix is null, it will me a straight map.
  SBMapItem (int encode, unsigned int inWordSize, int outWordSize, SS_WORD16 highMin, SS_WORD16 highMax, SS_WORD16 lowMin, SS_WORD16 lowMax, const unsigned char* matrix=0);

  // This will create an SBMapNToN static Map
  SBMapItem (const unsigned char* buffer);

  // This will create an SBMapNToN static Map
  SBMapItem (int _encode, 
    const unsigned char* _name, const unsigned char* _comment, 
    unsigned int _commentSize,
    unsigned int _inWordSize, unsigned int _outWordSize, 
    unsigned int _inLengthSize, unsigned int _outLengthSize);
  ~SBMapItem ();
  unsigned int getDecoderMap (SStringVector* key, SStringVector* value,
        unsigned int size);

  /* For maps with holes */
  unsigned int getLinearPosition (SS_UCS4 key);
  SS_UCS4 getLinearKey (unsigned int position);
  SS_UCS4 getLinearValue (unsigned int position);

  // to is also used to hash out state machine states.
  SFound find(const unsigned char in, unsigned int pos, int* from, int* to);
  const unsigned char* getComment (int arrayIndex, unsigned int* length);
  const unsigned char* getValue (int arrayIndex, unsigned int* length);
  const unsigned char* getKey (int arrayIndex, unsigned int* length,
    unsigned int* matchedLength=0);

  unsigned int getLength(unsigned int arrayIndex);
  void   convertFromBumap();

  // This takes
  int  add (const unsigned char* key, int keySize, int matchSize, 
      const unsigned char* value, int valueSize,
      const unsigned char* comment, int commSize);

  int  addLine (const unsigned char* line, unsigned int length, bool reverse=false);

  void buildStateMachine ();
  int  serialize (SString* fd, SFileFormat format, int last=0);
  int getSerializeSize ();


  // Get rid of the state machine.
  void  strip ();

  //
  // The comment field 
  // byte size.
  SS_WORD32  commentSize;
  const unsigned char*  comment;

  // Set to one if encode.
  unsigned char encode;

  //
  // The input/output word in bytes
  //
  unsigned char inWordSize;
  unsigned char outWordSize;

  //
  // The size of the length indicator (in bytes) in from of strings
  //
  unsigned char inByteLength;
  unsigned char outByteLength;

  // This is an array of SS_WORD32 s but they may not be aligned.
  // WORD32 - 4 byte word size.
  SS_WORD32    codeSize;
  const unsigned char*  codeMap;

  // This is the buffer where the references refer to in codeMap.
  // byte size
  SS_WORD32    baseSize;
  const unsigned char*   base;

  // 64 byte Word size
  SS_WORD32    		stateMachineSize;
  const unsigned char*  stateMachine;

  // 32 bytes
  const unsigned char*   name;

  SBMapItemType    itemType;

  SS_WORD16    highMin;
  SS_WORD16    highMax;
  SS_WORD16    lowMin;
  SS_WORD16    lowMax;

protected:
  int       writeCodeArea (SString* _fd, int _index, 
          SFileFormat _format, SS_LineEnd _last);

  int      writeTextBytes (SString* fd, 
          const unsigned char *from,
          int length, int slash, int wordSize,
          SFileFormat _format, SS_LineEnd _last); 
  unsigned int nextSorted (const unsigned char* key, unsigned int keylen);

  unsigned char*     toHex (const unsigned char* in, unsigned int size, unsigned int* len, unsigned int* match);
  unsigned int      stateMachineBufferSize;
  // This is writable
  unsigned char*     stateMachineBuffer;

  unsigned int       codeMapBufferSize;
  unsigned char*     codeMapBuffer;

  unsigned int       baseBufferSize;
  unsigned char*     baseBuffer;

  unsigned int       commentBufferSize;
  unsigned char*     commentBuffer;

  unsigned char*     nameBuffer;

  enum   SType { SS_STATIC=0, SS_DYNAMIC=1 };
  SType    stateMachineType;
  SType    baseType;
  SType    codeMapType;
  SType    commentType;
  SType    nameType;

  // This add one element to state machine, if needed.
  SS_WORD32   addState(SS_WORD32 oldState, 
        const unsigned char in, 
        unsigned int pos, 
        int from, int to);


};
//
// This is for low level routines.
// Encode means reverse map should be used.

#define SS_ACCEPT -1
#define SS_REJECT -2

/**
 * This is really what you should use
 */
class SBMap 
{
public:
  // This is actually reverse = encode logic.
  enum   SBMapType {SBMap_DECODE=0, SBMap_ENCODE};

  SBMap ();

  void setType (int mapType);
  void setName (const unsigned char* name);
  void setComment (const unsigned char* comment, int commentSize);

  ~SBMap ();

  /* for bumaps only - don't call this otherwise ! */
  SS_UCS4 decode (SS_UCS2 in);
  SS_UCS2 encode (SS_UCS4 in);

  unsigned int getDecoderMap (SStringVector* key, SStringVector* value,
        unsigned int size);

  /* For maps with holes */
  unsigned int getLinearPosition (unsigned int _index, SS_UCS4 key);
  SS_UCS4 getLinearKey (unsigned int _index, unsigned int position);
  SS_UCS4 getLinearValue (unsigned int _index, unsigned int position);


  // This item will be owned by SBMap.
  bool add (SBMapItem* item, int position=-1);

  bool  setFileImage (const SFileImage& image);

  bool  setArray (unsigned char* buffer, int size);

  inline bool getStatus() { return status; }

  inline unsigned int getInWordSize (int mapIndex)
  {
    return (unsigned int) maps[mapIndex]->inWordSize;
  }
  inline unsigned int getOutWordSize (int mapIndex)
  {
    return (unsigned int) maps[mapIndex]->outWordSize;
  }

  bool makeUnicodeMap ();
  bool makeStraightMap ();

  // Return max index
  int getSize (int index=-1);

  // Return SBMap_ENCODE or SBMap_DECODE
  SBMapType getType (int index);

  int  encode (int mapIndex, const void* in, int in_size,
    SStateModel *stateModel, int more=0);

  // Same as encode, but put it in the circle
  int  circle (SBMapType type, const void* in, int in_size,
    SStateModel *stateModel, int more=0);

  // Return the name field into an array if called with no args return
   // The name of the whole map. Return the size, but null terminate as well
  int  getName (char* line, int len, int mapIndex=-1);

  // Read 'between the lines' and return the comment.
  // Return the size, bu null terminate too.
  int  getComment (char* line, int len, int mapIndex=-1);

  // Has state machine ?
  const unsigned char*  getStateMachine (int mapIndex);

  void buildStateMachine (int mapIndex=-1);

  // Add data to buffer...
  int serialize (SOutputStream& fd, SFileFormat format=SS_BINARY);

  inline SBMapItem*  getItem (int _index)
  {
    return maps[_index];
  }

  // Get rid of state machines.
  void strip ();
  int    mapType;
  bool   isUMap();

protected:
  // For humap, cumap
  int serializeUMAP (SString* fd, SFileFormat format=SS_BINARY);

  int  packString (char* line, int len, const unsigned char* input, int maxlen);
  enum  Type { SBMap_MMAP, SBMap_ARRAY, SBMap_DYNAMIC };
  
  void   setOutput(SBMapItem* map, SStateModel* stateModel);

  bool  status;

  void clear ();
  bool processBuffer ();
  bool processSBMapBuffer ();
  bool processBMBuffer ();

  SFileImage	image;	

  unsigned char*    buffer;
  unsigned int      bufferSize;
  Type      	    bufferType;

  // 32 bytes
  const unsigned char*   name;
  unsigned char*  nameBuffer;

  Type    nameType;

  const unsigned char*  comment;
  SS_WORD32  commentSize;

  unsigned char*  commentBuffer;
  Type    commentType;

  const unsigned char*  base;

  // From buffer. checkBuffer sets them, clear clear them.

  // Points to beginning of tables.
  int    mapSize;
  SBMapItem**  maps;
};

#endif /* SBMap_H */
