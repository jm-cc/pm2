/** @file tbx_checksum.c
 *  @brief Various checksum algorithms.
 *  Code gathered from various places. See comments for licenses
 *  and authors.
 */

/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2010-2012 "the PM2 team" (see AUTHORS file)
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


#include <stdio.h>
#include <stdint.h>
#include <string.h>
#ifdef __SSE2__
#include <emmintrin.h>
#endif
#ifdef __SSE4__
#include <smmintrin.h>
#endif
#include "tbx.h"

#if defined(__SSE2__) && !defined(__INTEL_COMPILER)
#  define TBX_SSE_XOR 1
#else
#  define TBX_SSE_XOR 0
#endif
#if defined (__SSE4_2__) && !defined(__INTEL_COMPILER)
#  define TBX_SSE_CRC32 1
#else
#  define TBX_SSE_CRC32 0
#endif

#if defined(__SSE2__) && defined(__INTEL_COMPILER)
__m128i _mm_set_epi64x(__int64 i1, __int64 i2);
#endif

#ifdef __x86_64__
#define cpuid(func, ax, bx, cx, dx)\
	__asm__ __volatile__ ("cpuid":\
	"=a" (ax), "=b" (bx), "=c" (cx), "=d" (dx) : "a" (func));

static inline int tbx_support_sse4_1(void)
{ 
  int ax, bx, cx, dx;
  cpuid(1, ax, bx, cx, dx);
  return (cx >> 19) & 1;
}
static inline int tbx_support_sse4_2(void)
{ 
  int ax, bx, cx, dx;
  cpuid(1, ax, bx, cx, dx);
  return (cx >> 20) & 1;
}
static inline int tbx_support_sse3(void)
{ 
  int ax, bx, cx, dx;
  cpuid(1, ax, bx, cx, dx);
  return (cx >> 9) & 1;
}
static inline int tbx_support_sse2(void)
{ 
  int ax, bx, cx, dx;
  cpuid(1, ax, bx, cx, dx);
  return (dx >> 26) & 1;
}
static inline int tbx_support_sse(void)
{ 
  int ax, bx, cx, dx;
  cpuid(1, ax, bx, cx, dx);
  return (dx >> 25) & 1;
}
#else /* __x86_64__ */
static inline int tbx_support_sse4_2(void)
{ return 0; }
static inline int tbx_support_sse4_1(void)
{ return 0; }
static inline int tbx_support_sse3(void)
{ return 0; }
static inline int tbx_support_sse2(void)
{ return 0; }
static inline int tbx_support_sse(void)
{ return 0; }
#endif /* __x86_64__ */

static uint32_t tbx_checksum_dummy(const void*_data TBX_UNUSED, size_t _len TBX_UNUSED);
static uint32_t tbx_checksum_xor32(const void*_data, size_t _len);
static uint32_t tbx_checksum_adler32(const void*_data, size_t len);
static uint32_t tbx_checksum_murmurhash2a (const void *_data, size_t len);
static uint32_t tbx_checksum_murmurhash64a(const void*_data, size_t len);
static uint32_t tbx_checksum_hsieh(const void *_data, size_t len);


static uint32_t tbx_checksum_plain32(const void*_data, size_t _len);
static uint32_t tbx_checksum_and_copy_plain32(void*_dest, const void*_src, size_t _len);

static uint32_t tbx_checksum_block64(const void*_data, size_t _len);
static uint32_t tbx_checksum_and_copy_block64(void*_dest, const void*_src, size_t _len);

static uint32_t tbx_checksum_jenkins(const void*_data, size_t _len);
static uint32_t tbx_checksum_and_copy_jenkins(void*_dest, const void*_src, size_t _len);

static uint32_t tbx_checksum_fletcher64(const void *_data, size_t _len);
static uint32_t tbx_checksum_and_copy_fletcher64(void*_dest, const void*_src, size_t _len);

static uint32_t tbx_checksum_fnv1a(const void*_data, size_t _len);
static uint32_t tbx_checksum_and_copy_fnv1a(void*_dest, const void*_src, size_t _len);

static uint32_t tbx_checksum_knuth(const void*_data, size_t _len);
static uint32_t tbx_checksum_and_copy_knuth(void*_dest, const void*_src, size_t _len);

static uint32_t tbx_checksum_softcrc(const void*_data, size_t _len);

#if (TBX_SSE_CRC32 == 1)
static uint32_t tbx_checksum_crc32(const void*_data, size_t _len);
static uint32_t tbx_checksum_and_copy_crc32(void*_dest, const void*_src, size_t _len);
#endif

static const struct tbx_checksum_s checksums[] =
{
  { .short_name = "dummy",         .name = "No checksum",           .func = &tbx_checksum_dummy },
  { .short_name = "xor",           .name = "SSE2 xor32",            .func = &tbx_checksum_xor32 },
  { .short_name = "plain",         .name = "plain32",              
    .func              = &tbx_checksum_plain32,
    .checksum_and_copy = &tbx_checksum_and_copy_plain32 
  },
  { .short_name = "block64",       .name = "block64",            
    .func              = &tbx_checksum_block64 ,
    .checksum_and_copy = &tbx_checksum_and_copy_block64
  },
  { .short_name = "adler",         .name = "Adler32",               .func = &tbx_checksum_adler32 },
  { .short_name = "fletcher",      .name = "Fletcher64",
    .func              = &tbx_checksum_fletcher64,
    .checksum_and_copy = &tbx_checksum_and_copy_fletcher64
  },
  { .short_name = "jenkins",       .name = "Jenkins one-at-a-time",
    .func              = &tbx_checksum_jenkins ,
    .checksum_and_copy = &tbx_checksum_and_copy_jenkins
  },
  { .short_name = "fnv1a",         .name = "FNV1a", 
    .func              = &tbx_checksum_fnv1a,
    .checksum_and_copy = &tbx_checksum_and_copy_fnv1a
  },
  { .short_name = "knuth",         .name = "Knuth",
    .func              = &tbx_checksum_knuth,
    .checksum_and_copy = &tbx_checksum_and_copy_knuth
  },
  { .short_name = "murmurhash2a",  .name = "MurmurHash2a",          .func = &tbx_checksum_murmurhash2a },
  { .short_name = "murmurhash64a", .name = "MurmurHash64a",         .func = &tbx_checksum_murmurhash64a },
  { .short_name = "hsieh",         .name = "Paul Hsieh SuperFast",  .func = &tbx_checksum_hsieh },
  { .short_name = "softcrc",       .name = "software CRC32",        .func = &tbx_checksum_softcrc },
#if (TBX_SSE_CRC32 == 1)
  { .short_name = "crc",           .name = "SSE4.2 CRC32",         
    .func              = &tbx_checksum_crc32,
    .checksum_and_copy = &tbx_checksum_and_copy_crc32
  },
#endif /* TBX_SSE_CRC32 */
  { .short_name = NULL, .name = NULL, .func = NULL }
};

tbx_checksum_t tbx_checksum_get(const char*short_name)
{
  int i = 0;

  if(short_name == NULL || (strcmp(short_name, "default") == 0) || (strcmp(short_name, "1") == 0))
    {
      if(TBX_SSE_CRC32 && tbx_support_sse4_2())
	short_name = "crc";
      else if(TBX_SSE_XOR && tbx_support_sse2())
	short_name = "xor";
      else
	short_name = "fnv1a";
    }

  if((strcmp("xor", short_name) == 0) &&
     (!tbx_support_sse2()))
    TBX_FAILUREF("tbx_checksum: FATAL- CPU does not support SSE2, cannot use 'xor' checksum.\n");

  if((strcmp("crc", short_name) == 0) &&
     (!tbx_support_sse4_2()))
    TBX_FAILUREF("tbx_checksum: FATAL- CPU does not support SSE4.2, cannot use 'crc' checksum.\n");

  while(checksums[i].short_name != NULL)
    {
      if(strcmp(checksums[i].short_name, short_name) == 0)
	return &checksums[i];
      i++;
    }
  return NULL;
}


/* ********************************************************* */

/** xor-fold a 64 bits checksum into 32 bits */
static inline uint32_t tbx_checksum_fold64(uint64_t sum)
{
  return (uint32_t)((sum & 0xFFFFFFFF) ^ ((sum & 0xFFFFFFFF00000000ULL) >> 32));
}

static inline void tbx_checksum_copy_tail(void*_dest, const void*_src, size_t done, size_t len)
{
  char*dest = _dest;
  const char*src = _src;
  while(done < len)
    {
      dest[done] = src[done];
      done++;
    }
}


/** Dummy checksum function (that actually does not checksum).
 */
uint32_t tbx_checksum_dummy(const void*_data TBX_UNUSED, size_t _len TBX_UNUSED)
{
  return 0;
}

/* ********************************************************* */
/** 32 bit XOR checksum
 * . optimized with SSE2 128 bits XOR if available.
 * . using 64 bit XOR then folded to 32 bits else.
 */
uint32_t tbx_checksum_xor32(const void*_data, size_t _len)
{
  const uint64_t *data = (const uint64_t*)_data;
  size_t i;
#if (TBX_SSE_XOR == 1)
  const size_t len64 = _len / sizeof(uint64_t);
  const size_t len128 = len64 / 2;
  __m128i sum128 = _mm_setzero_si128();
  for(i = 0; i < len128; i++) 
    sum128 = _mm_xor_si128(sum128, _mm_set_epi64x(data[i * 2], data[i * 2 + 1]));
  size_t tail = len128 * (2 * sizeof(uint64_t));
  uint32_t sum_tail = 0;
  while(tail < _len)
    {
      uint32_t tail_data = 0;
      size_t tail_chunk = 0;
      while((tail < _len) && (tail_chunk < sizeof(uint32_t)))
	{
	  tail_data <<= 8;
	  tail_data |= ((const uint8_t*)_data)[tail];
	  tail++;
	}
      sum_tail ^= tail_data;
    }
  return (uint32_t)((_mm_extract_epi16(sum128, 0) | (_mm_extract_epi16(sum128, 1) << 16)) ^ 
		    (_mm_extract_epi16(sum128, 2) | (_mm_extract_epi16(sum128, 3) << 16)) ^ 
		    (_mm_extract_epi16(sum128, 4) | (_mm_extract_epi16(sum128, 5) << 16)) ^ 
		    (_mm_extract_epi16(sum128, 6) | (_mm_extract_epi16(sum128, 7) << 16)) ^ 
		    sum_tail);
#else /* TBX_SSE_XOR */
  const size_t len = _len / sizeof(*data);
  uint64_t sum64 = 0;
  for(i = 0; i < len; i++)
    sum64 ^= data[i];
  return tbx_checksum_fold64(sum64);
#endif /* TBX_SSE_XOR */
}

/* ********************************************************* */
/** Plain 32 bits sum
 * @author Alexandre DENIS
 */
static uint32_t tbx_checksum_plain32(const void*_data, size_t _len)
{
  const uint32_t *data = (const uint32_t*)_data;
  const size_t len = _len / sizeof(uint32_t);
  uint32_t sum = 0;
  size_t i;
  for(i = 0; i < len; i++)
    {
      sum += data[i];
    }
  return sum;
}

static uint32_t tbx_checksum_and_copy_plain32(void*_dest, const void*_src, size_t _len)
{
  const uint32_t*src = (const uint32_t*)_src;
  uint32_t*dest = (uint32_t*)_dest;
  const size_t len = _len / sizeof(uint32_t);
  uint32_t sum = 0;
  size_t i;
  for(i = 0; i < len; i++)
    {
      const uint32_t data = src[i];
      sum += data;
      dest[i] = data;
    }
  tbx_checksum_copy_tail(_dest, _src, len * sizeof(uint32_t), _len);
  return sum;
}


/* ********************************************************* */
/** Optimized sum with 64 bits blocks with xor-folding for a 32 bit result.
 * @todo tail of block is missing for length not multiple of 8 bytes.
 * @author Alexandre DENIS
 */
static uint32_t tbx_checksum_block64(const void*_data, size_t _len)
{
  const uint64_t *data = (const uint64_t*)_data;
  const size_t len = _len / sizeof(uint64_t);
  uint64_t sum = 0;
  size_t i;
  for(i = 0; i < len; i++)
    {
      sum += data[i];
    }
  return tbx_checksum_fold64(sum);
}

static uint32_t tbx_checksum_and_copy_block64(void*_dest, const void*_src, size_t _len)
{
  uint64_t*dest = _dest;
  const uint64_t*src = _src;
  const size_t len = _len / sizeof(uint64_t);
  uint64_t sum = 0;
  size_t i;
  for(i = 0; i < len; i++)
    {
      const uint64_t b = src[i];
      sum += b;
      dest[i] = b;
    }
  tbx_checksum_copy_tail(_dest, _src, len * sizeof(uint64_t), _len);
  return tbx_checksum_fold64(sum);
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

#define DO1(buf)  {s1 = (unsigned long)((long)s1 + *buf++); s2 += s1;}
#define DO2(buf)  DO1(buf); DO1(buf);
#define DO4(buf)  DO2(buf); DO2(buf);
#define DO8(buf)  DO4(buf); DO4(buf);
#define DO16(buf) DO8(buf); DO8(buf);

uint32_t tbx_checksum_adler32(const void*_data, size_t len)
{
	const char *buf = (const char *)_data;
	unsigned long s1;
	unsigned long s2;
	size_t k;

	s1 = s2 = 0;
	while (len > 0) {
		k = len < ADLER32_NMAX ? len : ADLER32_NMAX;
		len -= k;

		while (k >= 16)	{
			DO16(buf);
			k -= 16;
		}

		while (k) {
			DO1(buf);
			k--;
		}

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
static uint32_t tbx_checksum_fletcher64(const void *_data, size_t _len)
{
  const uint32_t *data = (const uint32_t*)_data;
  const size_t len = _len / sizeof(*data);
  uint64_t sum1 = 0xffffffff, sum2 = 0xffffffff;
  size_t i;
  for(i = 0; i < len ; i++) 
    {
      sum1 += data[i];
      sum2 += sum1;
    }
  /* Second reduction step to reduce sums to 32 bits */
  sum1 = (sum1 & 0xffffffff) + (sum1 >> 32);
  sum2 = (sum2 & 0xffffffff) + (sum2 >> 32);
  return (uint32_t)(sum1 + sum2);
}

static uint32_t tbx_checksum_and_copy_fletcher64(void*_dest, const void*_src, size_t _len)
{
  const uint64_t*src = _src;
  uint64_t*dest = _dest;
  const size_t len = _len / sizeof(uint64_t);
  uint64_t sum1 = 0xffffffff, sum2 = 0xffffffff;
  size_t i;
  for(i = 0; i < len; i++)
    {
      const uint64_t data = src[i];
      sum1 += data;
      sum2 += sum1;
      dest[i] = data;
    }
  tbx_checksum_copy_tail(_dest, _src, len * sizeof(uint32_t), _len);
  sum1 = (sum1 & 0xffffffff) + (sum1 >> 32);
  sum2 = (sum2 & 0xffffffff) + (sum2 >> 32);
  return (uint32_t)(sum1 + sum2);
}


/* ********************************************************* */
/** Jenkins one-at-a-time hash function.
 * Algorithm from Bob Jenkins article in Dr. Dobbs Journal (1997)
 * code copy/pasted from Puk hashtables
 * @author Alexandre DENIS
 */
uint32_t tbx_checksum_jenkins(const void*_data, size_t _len)
{
  const uint64_t *data = (const uint64_t*)_data;
  const size_t len = _len / sizeof(*data);
  uint64_t hash = 0;
  size_t i;
  for(i = 0; i < len; ++i) 
    {
      hash += data[i];
      hash += (hash << 10);
      hash ^= (hash >> 6);
    }
  hash += (hash << 3);
  hash ^= (hash >> 11);
  hash += (hash << 15);
  return tbx_checksum_fold64(hash);
}
static uint32_t tbx_checksum_and_copy_jenkins(void*_dest, const void*_src, size_t _len)
{ 
  const uint64_t*src = _src;
  uint64_t*dest = _dest;
  const size_t len = _len / sizeof(uint64_t);
  uint64_t hash = 0;
  size_t i;
  for(i = 0; i < len; ++i) 
    {
      const uint64_t data = src[i];
      hash += data + (hash << 10) + (hash >> 6);
      dest[i] = data;
    }
  hash += (hash << 3);
  hash ^= (hash >> 11);
  hash += (hash << 15);
  return tbx_checksum_fold64(hash);
}

/* ********************************************************* */
/** Fowler-Noll-Vo hash function, version FNV1a.
 * Algorithm from Glenn Fowler, Landon Curt Noll, and Phong Vo.
 * Code from the reference FNV implementation (public domain).
 * @author Glenn Fowler, Landon Curt Noll, and Phong Vo.
 */
static uint32_t tbx_checksum_fnv1a(const void*_data, size_t _len)
{
  const uint64_t *data = (const uint64_t *)_data;
  const size_t len = _len / sizeof(*data);
  uint64_t hash = 14695981039346656037ULL;
  size_t i;
  for(i = 0; i < len ; i++) 
    {
      hash ^= data[i];
      hash *= 1099511628211ULL;
    }
  return tbx_checksum_fold64(hash);
}

static uint32_t tbx_checksum_and_copy_fnv1a(void*_dest, const void*_src, size_t _len)
{
  const uint64_t*src = _src;
  uint64_t*dest = _dest;
  const size_t len = _len / sizeof(uint64_t);
  uint64_t hash = 14695981039346656037ULL;
  size_t i;
  for(i = 0; i < len; i++)
    {
      const uint64_t data = src[i];
      hash ^= data;
      hash *= 1099511628211ULL;
      dest[i] = data;
    }
  tbx_checksum_copy_tail(_dest, _src, len * sizeof(uint64_t), _len);
  return tbx_checksum_fold64(hash);
}


/* ********************************************************* */
/** Knuth hashing.
 * Algorithm by Donald Knuth
 */
static uint32_t tbx_checksum_knuth(const void*_data, size_t _len)
{
  const uint64_t *data = (const uint64_t *)_data;
  const size_t len = _len / sizeof(*data);
  uint64_t h = _len;
  size_t i;
  for(i = 0; i < len; i++)
    {
      h = ((h << 5) ^ (h >> 27)) ^ data[i];
    }
  return tbx_checksum_fold64(h);
}
static uint32_t tbx_checksum_and_copy_knuth(void*_dest, const void*_src, size_t _len)
{
  const uint64_t*src = _src;
  uint64_t*dest = _dest;
  const size_t len = _len / sizeof(uint64_t);
  uint64_t h = _len;
  size_t i;
  for(i = 0; i < len; i++)
    {
      const uint64_t data = src[i];
      h = ((h << 5) ^ (h >> 27)) ^ data;
      dest[i] = data;
    }
  return tbx_checksum_fold64(h);

}

/* ********************************************************* */
/** MurmurHash2A, by Austin Appleby
 * Reference implementation in public domain, from:
 * http://sites.google.com/site/murmurhash/
 */
#define mmix(h,k) { k *= m; k ^= k >> r; k *= m; h *= m; h ^= k; }
uint32_t tbx_checksum_murmurhash2a (const void *_data, size_t len)
{
	const unsigned char *data = (const unsigned char *)_data;
	const unsigned int m = 0x5bd1e995;
	const int r = 24;
	unsigned int t;
	unsigned int h;
	unsigned int l;
 
	t = h = 0;
	l = len;
  
	while(len >= 4)	{
		unsigned int k = *(unsigned int*)data;
      
		mmix(h,k);
		data += 4;
		len -= 4;
	}
  
	switch (len)
	{
	        case 3: 
			t ^= (unsigned int)(data[2] << 16);
	        case 2: 
			t ^= (unsigned int)(data[1] << 8);
	        case 1: 
			t ^= data[0];
	}
  
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
uint32_t tbx_checksum_murmurhash64a(const void*_data, size_t len)
{
	const uint64_t seed = 0;
	const uint64_t m = 0xc6a4a7935bd1e995ULL;
	const int r = 47;
	const uint64_t *data = (const uint64_t *)_data;
	const uint64_t *end = data + (len/sizeof(*data));
	uint64_t h;

	h = seed ^ (len * m);  
	while(data != end) {
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
	        case 7: 
			h ^= (uint64_t)(data2[6]) << 48;
	        case 6: 
			h ^= (uint64_t)(data2[5]) << 40;
	        case 5: 
			h ^= (uint64_t)(data2[4]) << 32;
	        case 4: 
			h ^= (uint64_t)(data2[3]) << 24;
	        case 3: 
			h ^= (uint64_t)(data2[2]) << 16;
	        case 2: 
			h ^= (uint64_t)(data2[1]) << 8;
	        case 1: 
			h ^= (uint64_t)(data2[0]);
			h *= m;
	};
  
	h ^= h >> r;
	h *= m;
	h ^= h >> r;
	
	/* xor-fold the 64 bit hash into 32 bits */
	return (uint32_t)((h & 0xFFFFFFFF) ^ ((h & 0xFFFFFFFF00000000ULL) >> 32));
} 


/* ********************************************************* */
/** Paul Hsieh SuperFastHash
 * distributed by author under the terms of GPLv2
 */
#define get16bits(d) (*((const uint16_t *) (d)))
uint32_t tbx_checksum_hsieh(const void *_data, size_t len)
{
	const unsigned char *data = (const unsigned char *)_data;
	uint32_t rem, hash, tmp;
  
	hash = (uint32_t)len;  
	rem = len & 3;
	len >>= 2;
  
	/* Main loop */
	for (;len > 0; len--) {
		hash  += get16bits(data);
		tmp    = (uint32_t)(get16bits(data+2) << 11) ^ hash;
		hash   = (hash << 16) ^ tmp;
		data  += 2*sizeof (uint16_t);
		hash  += hash >> 11;
	}
  
	/* Handle end cases */
	switch (rem) {
	        case 3: hash += get16bits(data);
			hash ^= hash << 16;
			hash ^= (uint32_t)(data[sizeof(uint16_t)] << 18);
			hash += hash >> 11;
			break;
	        case 2: hash += get16bits(data);
			hash ^= hash << 11;
			hash += hash >> 17;
			break;
	        case 1: hash += *data;
			hash ^= hash << 10;
			hash += hash >> 1;
			break;
	}
  
	/* Force "avalanching" of final 127 bits */
	hash ^= hash << 3;
	hash += hash >> 5;
	hash ^= hash << 4;
	hash += hash >> 17;
	hash ^= hash << 25;
	hash += hash >> 6;
  
	return hash;
}

/* ********************************************************* */
/** software CRC32 */

/* algorithm by Charles Michael Heard */

#define POLYNOMIAL 0x04c11db7L

static unsigned long crc_table[256];

void gen_crc_table(void)
/* generate the table of CRC remainders for all possible bytes */
{
  register int i, j;  register unsigned long crc_accum;
  for ( i = 0;  i < 256;  i++ )
    {
      crc_accum = ( (unsigned long) i << 24 );
      for ( j = 0;  j < 8;  j++ )
	{
	  if ( crc_accum & 0x80000000L )
	    crc_accum = ( crc_accum << 1 ) ^ POLYNOMIAL;
	  else
	    crc_accum = ( crc_accum << 1 ); }
      crc_table[i] = crc_accum; 
    }
}

unsigned long update_crc(unsigned long crc_accum, const char *data_blk_ptr, int data_blk_size)
/* update the CRC on the data block one byte at a time */
{
  register int j;
  for ( j = 0;  j < data_blk_size;  j++ )
    {
      const int i = ( (int) ( crc_accum >> 24) ^ *data_blk_ptr++ ) & 0xff;
      crc_accum = ( crc_accum << 8 ) ^ crc_table[i]; }
  return crc_accum;
}

static uint32_t tbx_checksum_softcrc(const void*_data, size_t _len)
{
  static int init_done = 0;
  if(!init_done)
    {
      gen_crc_table();
      init_done = 1;
    }
  return update_crc(0xFFFFFFFF, _data, _len);
}



/* ********************************************************* */
/** CRC32 using SSE 4.2
 * Code from Linux kernel 2.6.27 (GPL v2)
 * @author Austin Zhang
 */

#if (TBX_SSE_CRC32 == 1)

#define SCALE_F sizeof(unsigned long)
#ifdef __x86_64__
#define REX_PRE "0x48, "
#else
#define REX_PRE
#endif


static inline uint32_t crc32c_intel_le_hw_byte(uint32_t crc, unsigned char const *data, size_t length)
{
	while (length--) {
		__asm__ __volatile__(
			".byte 0xf2, 0xf, 0x38, 0xf0, 0xf1"
			:"=S"(crc)
			:"0"(crc), "c"(*data)
			);
		data++;
	}
  
	return crc;
}
static inline uint32_t crc32c_intel_le_hw(uint32_t crc, unsigned char const *p, size_t len)
{
	unsigned int iquotient;
	unsigned int iremainder;
	unsigned long *ptmp;
  
	iquotient = len / SCALE_F;
	iremainder = len % SCALE_F;
	ptmp = (unsigned long *)p;

	while (iquotient--) {
		__asm__ __volatile__(
			".byte 0xf2, " REX_PRE "0xf, 0x38, 0xf1, 0xf1;"
			:"=S"(crc)
			:"0"(crc), "c"(*ptmp)
			);
		ptmp++;
	}
  
	if (iremainder)
		crc = crc32c_intel_le_hw_byte(crc, (unsigned char *)ptmp, iremainder);
  
	return crc;
}

static uint32_t tbx_checksum_crc32(const void*_data, size_t _len)
{
	return crc32c_intel_le_hw(0, _data, _len);
}
static uint32_t tbx_checksum_and_copy_crc32(void*_dest, const void*_src, size_t _len)
{
  uint32_t*dest = _dest;
  const uint32_t*src = _src;
  const size_t len = _len / sizeof(uint32_t);
  uint64_t c = 0;
  size_t i;
  for(i = 0; i < len; i++)
    {
      const uint32_t b = src[i];
      c = __builtin_ia32_crc32si(c, b);
      dest[i] = b;
    }
  tbx_checksum_copy_tail(_dest, _src, len * sizeof(uint32_t), _len);
  return (uint32_t)c;
}
#endif /* TBX_SSE_CRC32 */

