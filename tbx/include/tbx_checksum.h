/** @file tbx_checksum.h
 *  @brief Various checksum algorithms.
 *  Code gathered from various places. See comments for licenses
 *  and authors.
 */

/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2010 "the PM2 team" (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#ifndef TBX_CHECKSUM_H
#define TBX_CHECKSUM_H

#include <stdint.h>

/** Dummy checksum function that actually does not checksum.
 */
static inline uint32_t tbx_checksum_dummy(const void*_data, size_t _len)
{
  return 0;
}

/* ********************************************************* */
/** Plain 32 bits sum
 * @author Alexandre DENIS
 */
static inline uint32_t tbx_checksum_plain32(const void*_data, size_t _len)
{
  const uint32_t*data = _data;
  const unsigned int len = _len / sizeof(uint32_t);
  int i;
  uint32_t sum = 0;
  for(i = 0; i < len; i++)
    {
      sum += data[i];
    }
  return sum;
}


/* ********************************************************* */
/** Optimized sum with 64 bits blocks with xor-folding for a 32 bit result.
 * @todo tail of block is missing for length not multiple of 8 bytes.
 * @author Alexandre DENIS
 */
static inline uint32_t tbx_checksum_block64(const void*_data, size_t _len)
{
  const uint64_t*data = _data;
  const unsigned int len = _len / sizeof(uint64_t);
  int i;
  uint64_t sum = 0;
  for(i = 0; i < len; i++)
    {
      sum += data[i];
    }
  const uint32_t sum32 = (sum & 0xFFFFFFFF) ^ ((sum & 0xFFFFFFFF00000000) >> 32);
  return sum32;
}



/* ********************************************************* */
/** Adler32 from Zlib, modified to work on blocks instead of streams
 */
/* adler32.c -- compute the Adler-32 checksum of a data stream
 * Copyright (C) 1995 Mark Adler

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

 */

#define ADLER32_BASE 65521 /* largest prime smaller than 65536 */
#define ADLER32_NMAX 5552 /* NMAX is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1 */

#define DO1(buf)  {s1 += *buf++; s2 += s1;}
#define DO2(buf)  DO1(buf); DO1(buf);
#define DO4(buf)  DO2(buf); DO2(buf);
#define DO8(buf)  DO4(buf); DO4(buf);
#define DO16(buf) DO8(buf); DO8(buf);
static inline uint32_t tbx_checksum_adler32(const void*_data, size_t len)
{
  const char*buf = _data;
  const unsigned long adler = 0;
  unsigned long s1 = adler & 0xffff;
  unsigned long s2 = (adler >> 16) & 0xffff;
  int k;
  while (len > 0)
    {
      k = len < ADLER32_NMAX ? len : ADLER32_NMAX;
      len -= k;
      while (k >= 16)
	{
	  DO16(buf);
	  k -= 16;
	}
      if (k != 0)
	do
	  {
	    DO1(buf);
	  } while (--k);
      s1 %= ADLER32_BASE;
      s2 %= ADLER32_BASE;
    }
  return (s2 << 16) | s1;
}


/* ********************************************************* */
/** Fletcher-64 Algorithm freely inspired by John Fletcher article in
 * IEEE Transactions on Communications (1982), heavily optimized for
 * 64 bits words to get half-decent performance.
 * @author Alexandre DENIS
 */
static inline uint32_t tbx_checksum_fletcher64(const void*_data, size_t _len)
{
  const uint32_t*data = _data;
  int len = _len / 4;
  uint64_t sum1 = 0xffffffff, sum2 = 0xffffffff;
  int i;
  for(i = 0; i < len ; i++)
    {
      sum1 += data[i];
      sum2 += sum1;
    }
  /* Second reduction step to reduce sums to 32 bits */
  sum1 = (sum1 & 0xffffffff) + (sum1 >> 32);
  sum2 = (sum2 & 0xffffffff) + (sum2 >> 32);
  const uint32_t sum = sum1 + sum2;
  return sum;
}


/* ********************************************************* */
/** Jenkins one-at-a-time hash function.
 * Algorithm from Bob Jenkins article in Dr. Dobbs Journal (1997)
 * code copy/pasted from Puk hashtables
 * @author Alexandre DENIS
 */
static inline uint32_t tbx_checksum_jenkins(const void*_data, size_t _len)
{
  const uint32_t*data = _data;
  const int len = _len / 4;
  uint32_t hash, i;
  for(hash = i = 0; i < len; ++i)
    {
      hash += data[i];
      hash += (hash << 10);
      hash ^= (hash >> 6);
    }
  hash += (hash << 3);
  hash ^= (hash >> 11);
  hash += (hash << 15);
  return hash;
}


/* ********************************************************* */
/** Fowler-Noll-Vo hash function, version FNV1a.
 * Algorithm from Glenn Fowler, Landon Curt Noll, and Phong Vo.
 * Code from the reference FNV implementation (public domain).
 * @author Glenn Fowler, Landon Curt Noll, and Phong Vo.
 */
static inline uint32_t tbx_checksum_fnv1a(const void*_data, size_t _len)
{
  const uint64_t*data = _data;
  const int len = _len / 8;
  uint64_t hash = 14695981039346656037ULL;
  int i;
  for(i = 0; i < len ; i++)
    {
      hash ^= data[i];
      hash *= 1099511628211;
    }
  return hash;
}


/* ********************************************************* */
/** Knuth hashing.
 * Algorithm by Donald Knuth
 */
static inline uint32_t tbx_checksum_knuth(const void*_data, size_t _len)
{
  const uint64_t*data = _data;
  int len = _len / 8;
  uint64_t h = _len;
  int i;
  for(i = 0; i < len; i++)
    {
      h = ((h << 5) ^ (h >> 27)) ^ data[i];
    }
  const uint32_t h32 = (h & 0xFFFFFFFF) ^ ((h & 0xFFFFFFFF00000000) >> 32);
  return h32;
}


/* ********************************************************* */
/** MurmurHash2A, by Austin Appleby
 * Reference implementation in public domain, from:
 * http://sites.google.com/site/murmurhash/
 */
#define mmix(h,k) { k *= m; k ^= k >> r; k *= m; h *= m; h ^= k; }
static inline uint32_t tbx_checksum_murmurhash2a (const void*_data, size_t len)
{
  const unsigned int m = 0x5bd1e995;
  const int r = 24;
  unsigned int l = len;
  
  const unsigned char * data = (const unsigned char *)_data;
  
  unsigned int h = 0;
  
  while(len >= 4)
    {
      unsigned int k = *(unsigned int*)data;
      
      mmix(h,k);
      
      data += 4;
      len -= 4;
    }
  
  unsigned int t = 0;
  
  switch(len)
    {
    case 3: t ^= data[2] << 16;
    case 2: t ^= data[1] << 8;
    case 1: t ^= data[0];
    };
  
  mmix(h,t);
  mmix(h,l);
  
  h ^= h >> 13;
  h *= m;
  h ^= h >> 15;
  
  return h;
}


/* ********************************************************* */
/** MurmurHash64A, by Austin Appleby
 * 64-bit version of MurmurHash2A, folded to 32 bits.
 * Reference implementation in public domain, onverted from C++ to C.
 */
static inline uint32_t tbx_checksum_murmurhash64a(const void*_data, size_t _len)
{
  const int seed = 0;
  const int len = _len;
  const uint64_t m = 0xc6a4a7935bd1e995;
  const int r = 47;
  const uint64_t * data = (const uint64_t *)_data;
  const uint64_t * end = data + (len/8);
  uint64_t h = seed ^ (len * m);
  
  while(data != end)
    {
      uint64_t k = *data++;
      
      k *= m; 
      k ^= k >> r; 
      k *= m; 
      
      h ^= k;
      h *= m; 
    }
  
  const unsigned char * data2 = (const unsigned char*)data;
  
  switch(len & 7)
    {
    case 7: h ^= (uint64_t)(data2[6]) << 48;
    case 6: h ^= (uint64_t)(data2[5]) << 40;
    case 5: h ^= (uint64_t)(data2[4]) << 32;
    case 4: h ^= (uint64_t)(data2[3]) << 24;
    case 3: h ^= (uint64_t)(data2[2]) << 16;
    case 2: h ^= (uint64_t)(data2[1]) << 8;
    case 1: h ^= (uint64_t)(data2[0]);
      h *= m;
    };
  
  h ^= h >> r;
  h *= m;
  h ^= h >> r;
  /* xor-fold the 64 bit hash into 32 bits */
  const uint32_t h32 = (h & 0xFFFFFFFF) ^ ((h & 0xFFFFFFFF00000000) >> 32);
  return h32;
} 

#endif /* TBX_CHECKSUM_H */
