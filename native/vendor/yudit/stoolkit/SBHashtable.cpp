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
 
#include "SBHashtable.h"
#include "SObject.h"
#include "SExcept.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_DATA_BYTES 8
#define HASH_INITIAL_SIZE 7
#define PSIZE (sizeof(char*))
#define RESIZE_AT 70 // Percent
#define PRIME_GROW_PERC 70 // Percent. 

static unsigned int largerPrime (unsigned int in);
static unsigned int sqrimax (unsigned int sq);
static char* getNullVector();
int SBHashtable::debugLevel = 0;

class SBucketItem : public SObject
{
public:
  SBucketItem (const SString& key, const char* value, int len);
  SBucketItem (const SBucketItem& i);
  virtual ~SBucketItem()  {}
  virtual SObject* clone () const {
   SBucketItem* n =  new SBucketItem(*this);
   CHECK_NEW(n);
   return n;
  }

  // Hope for the best - this should be aligned...
  union 
  {
     double d;
     void* v;
     char  c[MAX_DATA_BYTES];
  }  value;
  SString key;
  unsigned int hash;
};

#define SBucketVector  SBucketItem**
static SBucketVector _subBucketAppend (SBucketVector bv, SBucketItem* bi);

static unsigned int _subBucketVectorSize (SBucketVector bv);

#define GET_BUCKET(_arr, _mod) *((SBucketItem***)&_arr[_mod*PSIZE])

//typedef SVector<SBucketItem> SBucketVector;

/**
 * Create a single bucketitem
 * @param k is the key
 * @param v is the value
 * @param len is the size. max is 8
 */
SBucketItem::SBucketItem (const SBucketItem& item) : SObject(), key (item.key)
{
  hash =item.hash;
  memcpy (&value, &item.value, MAX_DATA_BYTES);
}
/**
 * Create a single bucketitem
 * @param k is the key
 * @param v is the value
 * @param len is the size. max is 8
 */
SBucketItem::SBucketItem (const SString& k, const char* v, int len) : SObject(), key(k) 
{
  if (len > MAX_DATA_BYTES)
  {
    fprintf (stderr, "Internal error:  SBucketItem:: bad data length\n");
    // Dump here.
  }
  hash = key.hashCode ();
  memcpy (&value, v, len);
}

/**
 * Set the debug level
 * @param level 0 means no debug printouts.
 * @return the previous level
 */
int
SBHashtable::debug (int level)
{
  int old = debugLevel;
  debugLevel = level;
  return old;
}

SObject*
SBHashtable::clone() const
{
 SBHashtable * n  = new SBHashtable (*this);
 CHECK_NEW(n);
 return n;
}

/**
 * This is the vector class. I don't prefer using STL.
 */
SBHashtable::SBHashtable(void)
{
    buffer = new SHShared(HASH_INITIAL_SIZE * PSIZE);
    CHECK_NEW(buffer);
    memset (buffer->array, 0, HASH_INITIAL_SIZE * PSIZE);
}

/**
 * Create a vector from a const vector. This is 
 * used when you do
 *      SBHashtable v = old;
 * @param v is the old
 */
SBHashtable::SBHashtable (const SBHashtable& v)
{
  // Assign the common buffer and  increment the reference count
  buffer = (SHShared*) v.buffer;
  buffer->count++;
}

/**
 * Assign a vector from a const vector. This is 
 * used when you do
 *      SBHashtable v;
 *      v = old;
 * @param v is the old
 */
SBHashtable&
SBHashtable::operator=(const SBHashtable& v)
{
  refer (v);
  return *this;
}

/**
 * Assign a vector from a const vector by changing the reference.
 * @param v is the input vector
 */
void
SBHashtable::refer(const SBHashtable& v)
{
  if (&v==this) return;
  cleanup ();
  buffer = (SHShared*) v.buffer;
  buffer->count++;
  return ;
}

/**
 * The destructor
 */
SBHashtable::~SBHashtable()
{
  cleanup();
}

/**
 * Return the bucket array size.
 */
unsigned int
SBHashtable::size( ) const
{
  return buffer->arraySize/PSIZE;
}

/**
 * Return the bucket array size.
 * param sub is the subbucket index.
 */
unsigned int
SBHashtable::size(int sub) const
{
  SBucketVector bv = GET_BUCKET (buffer->array, sub);
  if (bv == 0) return 0;
  return _subBucketVectorSize (bv);
}


/**
 * Remake the hash
 */
void
SBHashtable::rehash()
{
  if (buffer->count != 1)
  {
    fprintf (stderr, "Internal error:  SBuffer::rehash\n");
    exit (1);
  }
  SHShared *sb = buffer;
  unsigned int bigger = largerPrime(buffer->arraySize/PSIZE);
  if (debugLevel > 1)
  {
    fprintf (stderr, "SBHashtable::rehash from %u to %u\n",
      (unsigned int)(buffer->arraySize/PSIZE), bigger);
  }
  buffer = new SHShared (bigger * PSIZE);
  CHECK_NEW(buffer);
  memset (buffer->array, 0, bigger * PSIZE);

  SBucketVector bo;
  SBucketVector bn;
  for (unsigned int i=0; i<sb->arraySize/PSIZE; i++)
  {
    bo = GET_BUCKET (sb->array, i);
    if (bo == 0) continue;
    // Move the items over...
    for (unsigned j=0; bo[j] != 0; j++)
    {
       SBucketItem* bi =bo[j];
       unsigned int mod = bi->hash % (buffer->arraySize/PSIZE);
       bn = GET_BUCKET (buffer->array, mod);
       GET_BUCKET(buffer->array, mod) = _subBucketAppend (bn, bi);
       buffer->vectorSize++;
    }
    delete bo;
  }
  delete sb;
}

/**
 * Replace char's with len length at index
 * @param index is the reference index of len blocks
 * @param e is the pointer to the char array to be saved
 * @param len is the length of the block.
 */
void 
SBHashtable::put (const SString& index, const char* e, int len, bool replace)
{
  // Resize buckets
  if (buffer->vectorSize*PSIZE >= (buffer->arraySize * RESIZE_AT)/100)
  {
	rehash ();
  }
  unsigned int mod = index.hashCode() % (buffer->arraySize/PSIZE);
  SBucketItem* sbi = new SBucketItem (index, e, len);
  CHECK_NEW(sbi);
  SBucketVector bv = GET_BUCKET (buffer->array, mod);

  if (bv==0)
  {
//fprintf (stderr, "NEW BV %u\n", mod);
    bv = new SBucketItem*[2];
    CHECK_NEW(bv);
    bv[0] = sbi;
    bv[1] = 0;
    GET_BUCKET (buffer->array, mod)=bv;
    buffer->vectorSize++;
    return;
  }
//fprintf (stderr, "OLD BV %u\n", mod);

  // Find the old one and delete it.
  if (replace)
  {
    for (unsigned int i=0; bv[i]!= 0; i++) 
    {
      SBucketItem* bi = bv[i];
      if (bi->key == index)
      {
        delete bi;
        bv[i] = sbi;
        return;
      }
    }
  }
  bv = _subBucketAppend (bv, sbi); 
  GET_BUCKET(buffer->array, mod) = bv;
  buffer->vectorSize++;
  //fprintf (stderr, "NOREPLACE AFTER %u (%u)\n", _subBucketVectorSize (bv), mod);
  //fprintf (stderr, "NOREPLACE AFTER %u\n", _subBucketVectorSize (bv));
  //fprintf (stderr, "NOREPLACE AFTER %u\n", 
   //     _subBucketVectorSize (GET_BUCKET(buffer->array, mod)));
}


/**
 * Remove char's with len length at index 
 * @param index is the reference index of len blocks
 * @param len is the length of the block.
 */
void 
SBHashtable::remove (const SString& index)
{
  derefer ();
  unsigned int mod = index.hashCode() % (buffer->arraySize/PSIZE);
  SBucketVector bv = GET_BUCKET (buffer->array, mod);
  if (bv == 0) return;
  SBucketItem* bi;
  for (unsigned int i=0; bv[i] != 0; i++)
  {
     bi = bv[i];
     if (bi->key == index)
     {
        buffer->vectorSize--;
        delete bi;

        unsigned int j;
        for (j=i; bv[j]!=0; j++)
        {
           bv[j] = bv[j+1];
        }
        if (j==1)
        {
           delete [] bv;
	       bv = NULL;
	       GET_BUCKET (buffer->array, mod) = 0;
           break;
        }
        break;
      }
  }
}

/**
 * Clear the array and make a new SHShared.
 */
void
SBHashtable::clear ()
{
  cleanup ();
  buffer = new SHShared(HASH_INITIAL_SIZE * PSIZE);
  CHECK_NEW(buffer);
  memset (buffer->array, 0, HASH_INITIAL_SIZE * PSIZE);
}

/**
 * clean up. usually called before delete.
 * remove the buffer or its reference.  This is private!
 */
void
SBHashtable::cleanup ()
{
  if (!buffer) return; // already clean.
  if (buffer->count==1) {
    SBucketVector bv;
    for (unsigned int i=0; i<buffer->arraySize/PSIZE; i++)
    {
       bv = GET_BUCKET (buffer->array, i);
       if (bv)
       {
          SBucketItem* bi;
          for (unsigned int j=0; bv[j] != 0; j++)
          {
             bi = bv[j];
             delete bi;
          }
          delete [] bv;
	  bv = NULL;
       }
    }
    delete buffer;
  } else {
    buffer->count--;
  }
  buffer = 0;
}

/**
 * Return the element at index.
 * @param index is the reference index of len blocks
 */
const char*
SBHashtable::get (const SString& index) const
{
  unsigned int mod = index.hashCode() % (buffer->arraySize/PSIZE);
  SBucketVector bv = GET_BUCKET (buffer->array, mod);
  if (bv == 0) return getNullVector();
  SBucketItem* bi;
  for (unsigned int i=0; bv[i] != 0; i++)
  {
     bi = bv[i];
     if (bi->key == index)
     {
        return ((char*) &bi->value);
     }
  }
  return getNullVector();
}

/**
 * Return the element at index.
 * @param index is the reference index of len blocks
 */
const char*
SBHashtable::get (unsigned int bucket, unsigned int subbucket) const
{
  SBucketVector bv = GET_BUCKET (buffer->array, bucket);
  if (bv == 0) return getNullVector();
  for (unsigned int i=0; bv[i] != 0; i++)
  {
     if (i==subbucket)
     {
         SBucketItem* bi = bv[i];
         return ((const char*) &bi->value);
     }
  }
  return getNullVector();
}

/**
 * This could be used if you don't want to disturb the rehash.
 */
void
SBHashtable::put (unsigned int bucket, unsigned int subbucket, const char* e, int len)
{
  derefer ();
  SBucketVector bv = GET_BUCKET (buffer->array, bucket);
  if (bv == 0) return;
  SBucketItem* bi = bv[subbucket];
  memcpy (&bi->value, e, len);
}

/**
 * Return the element at index.
 * @param index is the reference index of len blocks
 */
const SString&
SBHashtable::key (unsigned int bucket, unsigned int subbucket) const
{
  SBucketVector bv = GET_BUCKET (buffer->array, bucket);
  if (bv == 0) return SStringNull;
  SBucketItem* bi = bv[subbucket];
  return (bi->key);
}

/**
 * Get the list of keys
 * @param keys is the string list of output
 */
void
SBHashtable::keys(SStringVector* keys) const
{
  keys->clear();
  for (unsigned int i=0; i<size(); i++)
  {
    for (unsigned int j=0; j<size(i); j++)
    {
      keys->append(key(i,j));
    }
  }
}

/**
 * The array will change. If the buffer is shared copy the buffer.
 */
void
SBHashtable::derefer()
{
  if (buffer->count==1) return;
  buffer->count--;
  SHShared* oldBuffer = buffer;
  // This copies the array..
  buffer = new SHShared (*oldBuffer);
  CHECK_NEW(buffer);

  for (unsigned int i=0; i<oldBuffer->arraySize/PSIZE; i++)
  {
    SBucketVector bv = GET_BUCKET (oldBuffer->array, i);
    if (bv ==0)
    {
        GET_BUCKET (buffer->array, i) = 0;
        continue;
    }

    // This will not work this way. The deferer routine
    // that comes after this can not recreate objects -
    // It can not put two things in the same array!
    // Note that we don't count references of contained object.
    // Copying read-only hash is ok, but read write can be expensive.
    // GET_BUCKET (buffer->array, i) = new SBucketVector(*bv);

    // Copy the whole array comment this 
    // Uncomment of the above one does not work.
    unsigned int ssize = _subBucketVectorSize (bv);
    SBucketVector nv = new SBucketItem* [ssize+1];

    CHECK_NEW (nv);
    for (unsigned int j=0; j<ssize; j++)
    {
        SBucketItem* item = bv[j];
        nv[j] = new SBucketItem(*item);
    }
    nv[ssize] = 0;
    GET_BUCKET (buffer->array, i) = nv;
  }
}

/**
 * These are needed for a real fast hash
 */
static unsigned int
_subBucketVectorSize (SBucketVector bv)
{
   if (bv == 0) return 0;
   unsigned int i;
   for (i=0; bv[i]!=0; i++) {}
   return i;
}

/**
 * These are needed for a real fast hash
 */
static SBucketVector
_subBucketAppend (SBucketVector bv, SBucketItem* bi)
{
   unsigned int count = _subBucketVectorSize (bv);
   SBucketVector nv = new SBucketItem*[count +2];
   CHECK_NEW (nv);
   if (count != 0)
   {
     memcpy (nv, bv, count * sizeof (SBucketItem*));
   }
   if (bv != 0)
   {
     delete [] bv;
     bv = NULL;
   }
   nv[count] = bi;
   nv[count+1] = 0;
   return nv;
}

/**
 * Give me a lerger prime than number.
 */
static unsigned int largerPrime (unsigned int base)
{
  /* It is enough to check if  it is  dividable by
     the square root of the fn. */
  unsigned int sqrb = sqrimax (base + (base*PRIME_GROW_PERC)/100);
  unsigned int sqro = sqrimax (base) + 1;
  if (sqro >= sqrb) sqrb = sqro+1;
  unsigned int result = (sqrb * sqrb) -1;
  unsigned int i;
  while (true)
  {
      for (i=2; i<sqrb; i++)
      {
            if ((result%i)==0) break;
      }
      if (i==sqrb) break;
      result--;
  }
  return result;
}

/**
 * Get the square root approximately - do not use util version
 * because that one returns less.
 * @param sq is the square
 */
static unsigned int sqrimax (unsigned int sq)
{
  if (sq==1) return 1;
  int r=sq-1; int x=sq;
  while (r < x)
  {
      x = r; r= (x+(sq/x))/2;
  }
  return r;
}

/*
 * Compose a static null vector
 */
static char* getNullVector()
{
  static union 
  {
      double d;
      void* v;
      char  c[MAX_DATA_BYTES];
  }   nullVector;
  memset (&nullVector, 0, MAX_DATA_BYTES);
  return (char*) &nullVector;
}

SOHashtable::SOHashtable (void) : SBHashtable()
{
}

SOHashtable::SOHashtable (const SOHashtable& base) : SBHashtable (base)
{
}

SOHashtable&
SOHashtable::operator=(const SOHashtable& v)
{
  refer (v);
  return *this;
}

SObject*
SOHashtable::clone() const
{
  return new SOHashtable (*this);
}

SOHashtable::~SOHashtable  ()
{
  cleanup ();
}

const SObject*
SOHashtable::get (const SString key) const
{
  return (SObject*) (*(SObject**)SBHashtable::get (key));
}

const SObject*
SOHashtable::get (unsigned int row, unsigned int col) const
{
  return (SObject*) (*(SObject**)SBHashtable::get (row, col));
}

void
SOHashtable::put (const SString& key, const SObject& e, bool replace)
{
  derefer();
  if (replace)
  {
    SObject* old = (SObject*) get (key);
    if (old)
    {
      delete old;
    }
  }
  SObject* r = e.clone(); SBHashtable::put (key, (char*) &r, sizeof (SObject*), replace);
}

void
SOHashtable::put (unsigned int bucket, unsigned int subbucket, SObject& e)
{
  derefer();
  SObject* old = (SObject*) get (bucket, subbucket); if (old) delete old;
  SObject* r = e.clone(); SBHashtable::put (bucket, subbucket, (char*) &r, sizeof (SObject*));
}

void
SOHashtable::remove (const SString& key)
{
  derefer();
  SObject* old = (SObject*) get (key); if (old) delete old;
  SBHashtable::remove (key);
}

void
SOHashtable::clear ()
{
  cleanup ();
  SBHashtable::clear();
}

void
SOHashtable::refer (const SOHashtable& v)
{
  if (&v != this)
  {
     cleanup();
     SBHashtable::refer(v);
  } 
}

void
SOHashtable::derefer()
{
  if (buffer->count==1) return;

  SBHashtable::derefer();
  const SObject* r; SString k;
  for (unsigned int i=0; i<size(); i++)
  {
    for (unsigned int j=0; j<size(i); j++)
    {
       k = key (i, j); 
       r = (SObject*) get (i, j);
       SObject* newr = r->clone();
       SBHashtable::put (i, j, (char*) &newr, sizeof (newr)); 
    }
  }
}

void
SOHashtable::cleanup()
{
  if (buffer->count!=1)
  {
    return;
  }
  SObject* r; 
  for (unsigned int i=0; i<size(); i++)
  {
    for (unsigned int j=0; j<size(i); j++)
    {
       r = (SObject*) get (i, j);
       delete r;
    }
  }
  SBHashtable::cleanup();
}
