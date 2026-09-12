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
 
#ifndef SUniMap_h
#define SUniMap_h

#include "stoolkit/STypes.h"
#include "stoolkit/SObject.h"
#include "stoolkit/SString.h"
#include "stoolkit/SStringVector.h"

/**
 * A unicode Map. It maps SV_UCS2 to SV_UCS4. It is generally useful
 * When mapping UCS4 to Font encodings. 
 * This is mase upong the .my maps
 * @author: Gaspar Sinai <gaspar@yudit.org>
 * @version: 2000-05-12
 */
class SUniMap  : public SObject
{
public:
  SUniMap (void);
  SUniMap (const SString& name);
  SUniMap (const SUniMap &m);
  SUniMap& operator = (const SUniMap &m);   
  virtual ~SUniMap();
  virtual SObject* clone () const;
  
  SS_UCS4 decode (SS_UCS2 in);
  SS_UCS2 encode (SS_UCS4 in);
  unsigned int getDecoderMap (SStringVector* key, SStringVector* value,
        unsigned int size);

  /*
   * Convert whatever is in 'in' if possible. 
   * and remove it.
   */
  unsigned int  lift (const SV_UCS4& in, unsigned int ini, 
           bool decode, SV_UCS4* out=0);

  int getInWordSize(bool encode);
  int getOutWordSize(bool encode);

  bool isOK() const;

  static void setPath(const SStringVector &p);
  static void guessPath();
  static const SStringVector& getPath();

  /* State keeping coder. */
  void reset(bool encode);
  void reset();
  void undo (bool encode);

  const SV_UCS4& decode (const SString& in);
  const SString& encode (const SV_UCS4& in);

  const SString& encodeBuffer();
  const SV_UCS4& decodeBuffer();

  /* For maps with holes */
  unsigned int getDecodePosition (SS_UCS4 key);
  SS_UCS4 getDecodeKey (unsigned int position);
  SS_UCS4 getDecodeValue (unsigned int position);

  unsigned int getEncodePosition (SS_UCS4 key);
  SS_UCS4 getEncodeKey (unsigned int position);
  SS_UCS4 getEncodeValue (unsigned int position);

  /* Vector all maps available on this path */
  static SStringVector list ();
  const SString& remainder() const;

  bool isUMap() const;
  bool isClustered() const;

protected:
  void load (const SString& name);
  void derefer();
  void setModel(int din, int dout, int ein, int eout);
  int  indexOf (bool encode) const;
  bool ok;
  void* bumap;
  int   eindex;
  int   dindex;

  void* delegate;
  void* dmodel;
  void* emodel;

  void* dmodel4;
  void* emodel4;

  SV_UCS4 ucs4vIn;
  SV_UCS4 ucs4vOut;
  SString sstringIn;
  SString sstringOut;
};

#endif /* SUniMap_h */
