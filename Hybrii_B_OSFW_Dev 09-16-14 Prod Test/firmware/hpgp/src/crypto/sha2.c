/* sha256.c */
/*
    This file is part of the ARM-Crypto-Lib.
    Copyright (C) 2006-2010  Daniel Otte (daniel.otte@rub.de)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/**
 * \file		sha256.c
 * \author		Daniel Otte
 * \date		16.05.2006
 *
 * \par License:
 * 	GPL
 *
 * \brief SHA-256 implementation.
 *
 *
 */

//#include <stdint.h>
#include <string.h> /* for memcpy, memmove, memset */
#include "sha2.h"

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN
#endif

#if defined LITTLE_ENDIAN
#elif defined BIG_ENDIAN
#else
	#error specify endianess!!!
#endif

void BigMultiply(uint32_t u, uint32_t v, uint32_t * dest)
{
      uint32_t u1, u0, v1, v0, udiff, vdiff, high, mid, low;
      uint32_t prodh, prodl, was;
      int neg;

      u1 = HHALF(u);
      u0 = LHALF(u);
      v1 = HHALF(v);
      v0 = LHALF(v);

      low = u0 * v0;

      /* This is the same small-number optimization as before. */
      if (u1 == 0 && v1 == 0)
          dest[0] = low;

      if (u1 >= u0)
              udiff = u1 - u0, neg = 0;
      else
              udiff = u0 - u1, neg = 1;
      if (v0 >= v1)
              vdiff = v0 - v1;
      else
              vdiff = v1 - v0, neg ^= 1;
      mid = udiff * vdiff;

      high = u1 * v1;

      /* prod = (high << 2N) + (high << N); */
      prodh = high + HHALF(high);
      prodl = LHUP(high);

      /* if (neg) prod -= mid << N; else prod += mid << N; */
      if (neg) {

              was = prodl;
              prodl -= LHUP(mid);
              prodh -= HHALF(mid) + (prodl > was);
      } else {
              was = prodl;
              prodl += LHUP(mid);
              prodh += HHALF(mid) + (prodl < was);
      }

      /* prod += low << N */
      was = prodl;
      prodl += LHUP(low);
      prodh += HHALF(low) + (prodl < was);
      /* ... + low; */
      if ((prodl += low) < low)
              prodh++;

      /* return 4N-bit product */
      dest[1] = prodh;
      dest[0] = prodl;
}

  
void BigAdd16(uint32_t * a, uint16_t b, uint32_t * dest)
{
    dest[0] = a[0] + b;
    dest[1] = a[1] + (dest[0] < b);
}

/**
 * rotate x right by n positions
 */
static
uint32_t rotr32( uint32_t x, uint8_t n){
	return ((x>>n) | (x<<(32-n)));
}

static
uint32_t rotl32( uint32_t x, uint8_t n){
	return ((x<<n) | (x>>(32-n)));
}


/*************************************************************************/

// #define CHANGE_ENDIAN32(x) (((x)<<24) | ((x)>>24) | (((x)& 0x0000ff00)<<8) | (((x)& 0x00ff0000)>>8))
static
uint32_t change_endian32(uint32_t x){
	return (((x)<<24) | ((x)>>24) | (((x)& 0x0000ff00)<<8) | (((x)& 0x00ff0000)>>8));
}


/* sha256 functions as macros for speed and size, cause they are called only once */

#define CH(x,y,z)  (((x)&(y)) ^ ((~(x))&(z)))
#define MAJ(x,y,z) (((x)&(y)) ^ ((x)&(z)) ^ ((y)&(z)))

#define SIGMA_0(x) (rotr32((x), 2) ^ rotr32((x),13) ^ rotl32((x),10))
#define SIGMA_1(x) (rotr32((x), 6) ^ rotr32((x),11) ^ rotl32((x),7))
#define SIGMA_a(x) (rotr32((x), 7) ^ rotl32((x),14) ^ ((x)>>3))
#define SIGMA_b(x) (rotl32((x),15) ^ rotl32((x),13) ^ ((x)>>10))
#if 0
const
uint32_t ktable[]={
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};
#else
    uint32_t ktable[64];


#endif


/**
 * block must be, 512, Bit = 64, Byte, long !!!
 */
void sha2_small_common_nextBlock (sha2_small_common_ctx_t *state, const void* block){
	uint32_t w[16], wx;
	uint8_t  i;
	uint32_t a[8],t1,t2;

	/* init w */
#if defined LITTLE_ENDIAN
	for (i=0; i<16; ++i){
		w[i]= change_endian32(((uint32_t*)block)[i]);
	}
#elif defined BIG_ENDIAN
		memcpy((void*)w, block, 64);
#endif
/*
	for (i=16; i<64; ++i){
		w[i] = SIGMA_b(w[i-2]) + w[i-7] + SIGMA_a(w[i-15]) + w[i-16];
	}
*/
/* init working variables */
	memcpy((void*)a,(void*)(state->h), 8*4);

/* do the, fun stuff, */
	for (i=0; i<64; ++i){
		if(i<16){
			wx = w[i];
		}else{
			wx = SIGMA_b(w[14]) + w[9] + SIGMA_a(w[1]) + w[0];
			memmove(&(w[0]), &(w[1]), 15*4);
			w[15] = wx;
		}
		t1 = a[7] + SIGMA_1(a[4]) + CH(a[4],a[5],a[6]) + ktable[i] + wx;
		t2 = SIGMA_0(a[0]) + MAJ(a[0],a[1],a[2]);
		memmove(&(a[1]), &(a[0]), 7*4); 	/* a[7]=a[6]; a[6]=a[5]; a[5]=a[4]; a[4]=a[3]; a[3]=a[2]; a[2]=a[1]; a[1]=a[0]; */
		a[4] += t1;
		a[0] = t1 + t2;
	}

/* update, the, state, */
	for (i=0; i<8; ++i){
		state->h[i] += a[i];
	}
	state->length += 1;
}


void sha2_small_common_lastBlock(sha2_small_common_ctx_t *state, const void* block, uint16_t length_b){
	uint8_t lb[512/8]; /* local block */
	uint32_t len[2];
	uint8_t i;
	while(length_b>=512){
		sha2_small_common_nextBlock(state, block);
		length_b -= 512;
		block = (uint8_t*)block+64;
	}
//	len = state->length*512 + length_b;

	BigMultiply(state->length, 512, len);
    BigAdd16(len, length_b, len);

	memset(lb, 0, 64);
	memcpy(lb, block, (length_b+7)/8);

	/* set the final one bit */
	lb[length_b/8] |= 0x80>>(length_b & 0x7);
	/* pad with zeros */
	if (length_b>=512-64){ /* not enouth space for 64bit length value */
		sha2_small_common_nextBlock(state, lb);
		memset(lb, 0, 64);
	}
	/* store the 64bit length value */
#if defined LITTLE_ENDIAN
	 	/* this is now rolled up */
	
	i=7;
	do{
		lb[63-i] = ((uint8_t*)len)[i];
	}while(i--);
#elif defined BIG_ENDIAN
	*((uint32_t)&(lb[56])) = len[1];
	*((uint32_t)&(lb[60])) = len[0];
#endif
	sha2_small_common_nextBlock(state, lb);
}

/*************************************************************************/
#if 0
const
uint32_t sha256_init_vector[]={
	0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A,
    0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19 };
#else
uint32_t sha256_init_vector[8];


#endif

/*************************************************************************/

/**
 * \brief \c sh256_init initialises a sha256 context for hashing.
 * \c sh256_init c initialises the given sha256 context for hashing
 * @param state pointer to a sha256 context
 * @return none
 */
void sha256_init(sha256_ctx_t *state){
	state->length=0;
    ktable[0] = 0x428a2f98; ktable[1] = 0x71374491; ktable[2] = 0xb5c0fbcf; ktable[3] = 0xe9b5dba5;
    ktable[4] = 0x3956c25b; ktable[5] = 0x59f111f1; ktable[6] = 0x923f82a4; ktable[7] = 0xab1c5ed5;
	ktable[8] = 0xd807aa98; ktable[9] =  0x12835b01; ktable[10] =  0x243185be; ktable[11] =  0x550c7dc3; 
    ktable[12] =  0x72be5d74; ktable[13] =  0x80deb1fe; ktable[14] =  0x9bdc06a7; ktable[15] =  0xc19bf174;
	ktable[16] = 0xe49b69c1; ktable[17] =  0xefbe4786; ktable[18] =  0x0fc19dc6; ktable[19] =  0x240ca1cc; 
    ktable[20] =  0x2de92c6f; ktable[21] =  0x4a7484aa; ktable[22] =  0x5cb0a9dc; ktable[23] =  0x76f988da;
	ktable[24] = 0x983e5152; ktable[25] =  0xa831c66d; ktable[26] =  0xb00327c8; ktable[27] =  0xbf597fc7; 
    ktable[28] =  0xc6e00bf3; ktable[29] =  0xd5a79147; ktable[30] =  0x06ca6351; ktable[31] =  0x14292967;
	ktable[32] = 0x27b70a85; ktable[33] =  0x2e1b2138; ktable[34] =  0x4d2c6dfc; ktable[35] =  0x53380d13; 
    ktable[36] =  0x650a7354; ktable[37] =  0x766a0abb; ktable[38] =  0x81c2c92e; ktable[39] =  0x92722c85;
	ktable[40] = 0xa2bfe8a1; ktable[41] =  0xa81a664b; ktable[42] =  0xc24b8b70; ktable[43] =  0xc76c51a3; 
    ktable[44] =  0xd192e819; ktable[45] =  0xd6990624; ktable[46] =  0xf40e3585; ktable[47] =  0x106aa070;
	ktable[48] = 0x19a4c116; ktable[49] =  0x1e376c08; ktable[50] =  0x2748774c; ktable[51] =  0x34b0bcb5; 
    ktable[52] =  0x391c0cb3; ktable[53] =  0x4ed8aa4a; ktable[54] =  0x5b9cca4f; ktable[55] =  0x682e6ff3;
	ktable[56] = 0x748f82ee; ktable[57] =  0x78a5636f; ktable[58] =  0x84c87814; ktable[59] =  0x8cc70208; 
    ktable[60] =  0x90befffa; ktable[61] =  0xa4506ceb; ktable[62] =  0xbef9a3f7; ktable[63] =  0xc67178f2;
	
	sha256_init_vector[0]= 0x6A09E667;
	sha256_init_vector[1] = 0xBB67AE85;
	sha256_init_vector[2] = 0x3C6EF372; 
	sha256_init_vector[3] = 0xA54FF53A;
	sha256_init_vector[4] =  0x510E527F;
	sha256_init_vector[5] = 0x9B05688C;
	sha256_init_vector[6]  = 0x1F83D9AB;
	sha256_init_vector[7]  = 0x5BE0CD19;
	
	memcpy(state->h, sha256_init_vector, 8*4);
}

/*************************************************************************/
void sha256_nextBlock (sha256_ctx_t *state, const void* block){
	sha2_small_common_nextBlock(state, block);
}

/*************************************************************************/
void sha256_lastBlock (sha256_ctx_t *state, const void* block, uint16_t length_b){
	sha2_small_common_lastBlock(state, block, length_b);
}
/*************************************************************************/

/**
 * \brief function to process the last block being hashed
 * @param state Pointer to the context in which this block should be processed.
 * @param block Pointer to the message wich should be hashed.
 * @param length is the length of only THIS block in BITS not in bytes!
 *  bits are big endian, meaning high bits come first.
 * 	if you have a message with bits at the end, the byte must be padded with zeros
 */

/*************************************************************************/

/*
 * length in bits!
 */
void sha256(void* dest, const void* msg, uint32_t length_b){ /* length could be choosen longer but this is for µC */
	sha256_ctx_t s;
	sha256_init(&s);
	while(length_b >= SHA256_BLOCK_BITS){
		sha256_nextBlock(&s, msg);
		msg = (uint8_t*)msg + SHA256_BLOCK_BITS/8;
		length_b -= SHA256_BLOCK_BITS;
	}
	sha256_lastBlock(&s, msg, length_b);
	sha256_ctx2hash(dest,&s);
}



/*************************************************************************/

void sha256_ctx2hash(void* dest, const sha256_ctx_t *state){
#if defined LITTLE_ENDIAN
	uint8_t i, j, *s=(uint8_t*)(state->h);
	i=8;
	do{
		j=3;
		do{
			*((uint8_t*)dest) = s[j];
			dest = (uint8_t*)dest + 1;
		}while(j--);
		s += 4;
	}while(--i);
#elif BIG_ENDIAN
	memcpy(dest, state->h, 32);
#else
# error unsupported endian type!
#endif
}


