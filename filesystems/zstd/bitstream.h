/*
 * bitstream
 * Part of FSE library
 * header file (to include)
 * Copyright (C) 2013-2016, Yann Collet.
 *
 * BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 as published by the
 * Free Software Foundation. This program is dual-licensed; you may select
 * either version 2 of the GNU General Public License ("GPL") or BSD license
 * ("BSD").
 *
 * You can contact the author at :
 * - Source repository : https://github.com/Cyan4973/FiniteStateEntropy
 */
#ifndef BITSTREAM_H_MODULE
#define BITSTREAM_H_MODULE

/*
*  This API consists of small unitary functions, which must be inlined for best performance.
*  Since link-time-optimization is not available for all compilers,
*  these functions are defined into a .h to be included.
*/

/*-****************************************
*  Dependencies
******************************************/
#include "error_private.h" /* error codes and messages */
#include "mem.h"	   /* unaligned access routines */

/*=========================================
*  Target specific
=========================================*/
#define STREAM_ACCUMULATOR_MIN_32 25
#define STREAM_ACCUMULATOR_MIN_64 57
#define STREAM_ACCUMULATOR_MIN ((U32)(ZSTD_32bits() ? STREAM_ACCUMULATOR_MIN_32 : STREAM_ACCUMULATOR_MIN_64))


/*-********************************************
*  bitStream decoding API (read backward)
**********************************************/
typedef struct {
	size_t bitContainer;
	unsigned bitsConsumed;
	const char *ptr;
	const char *start;
} BIT_DStream_t;

typedef enum {
	BIT_DStream_unfinished = 0,
	BIT_DStream_endOfBuffer = 1,
	BIT_DStream_completed = 2,
	BIT_DStream_overflow = 3
} BIT_DStream_status; /* result of BIT_reloadDStream() */
/* 1,2,4,8 would be better for bitmap combinations, but slows down performance a bit ... :( */

ZSTD_STATIC size_t BIT_initDStream(BIT_DStream_t *bitD, const void *srcBuffer, size_t srcSize);
ZSTD_STATIC size_t BIT_readBits(BIT_DStream_t *bitD, unsigned nbBits);
ZSTD_STATIC BIT_DStream_status BIT_reloadDStream(BIT_DStream_t *bitD);
ZSTD_STATIC unsigned BIT_endOfDStream(const BIT_DStream_t *bitD);

/* Start by invoking BIT_initDStream().
*  A chunk of the bitStream is then stored into a local register.
*  Local register size is 64-bits on 64-bits systems, 32-bits on 32-bits systems (size_t).
*  You can then retrieve bitFields stored into the local register, **in reverse order**.
*  Local register is explicitly reloaded from memory by the BIT_reloadDStream() method.
*  A reload guarantee a minimum of ((8*sizeof(bitD->bitContainer))-7) bits when its result is BIT_DStream_unfinished.
*  Otherwise, it can be less than that, so proceed accordingly.
*  Checking if DStream has reached its end can be performed with BIT_endOfDStream().
*/


ZSTD_STATIC size_t BIT_readBitsFast(BIT_DStream_t *bitD, unsigned nbBits);
/* faster, but works only if nbBits >= 1 */

/*-**************************************************************
*  Internal functions
****************************************************************/
ZSTD_STATIC unsigned BIT_highbit32(register U32 val) { return 31 - __builtin_clz(val); }

/*=====    Local Constants   =====*/
static const unsigned BIT_mask[] = {0,       1,       3,       7,	0xF,      0x1F,     0x3F,     0x7F,      0xFF,
				    0x1FF,   0x3FF,   0x7FF,   0xFFF,    0x1FFF,   0x3FFF,   0x7FFF,   0xFFFF,    0x1FFFF,
				    0x3FFFF, 0x7FFFF, 0xFFFFF, 0x1FFFFF, 0x3FFFFF, 0x7FFFFF, 0xFFFFFF, 0x1FFFFFF, 0x3FFFFFF}; /* up to 26 bits */



/*-********************************************************
* bitStream decoding
**********************************************************/
/*! BIT_initDStream() :
*   Initialize a BIT_DStream_t.
*   `bitD` : a pointer to an already allocated BIT_DStream_t structure.
*   `srcSize` must be the *exact* size of the bitStream, in bytes.
*   @return : size of stream (== srcSize) or an errorCode if a problem is detected
*/
ZSTD_STATIC size_t BIT_initDStream(BIT_DStream_t *bitD, const void *srcBuffer, size_t srcSize)
{
	if (srcSize < 1) {
		memset(bitD, 0, sizeof(*bitD));
		return ERROR(srcSize_wrong);
	}

	if (srcSize >= sizeof(bitD->bitContainer)) { /* normal case */
		bitD->start = (const char *)srcBuffer;
		bitD->ptr = (const char *)srcBuffer + srcSize - sizeof(bitD->bitContainer);
		bitD->bitContainer = ZSTD_readLEST(bitD->ptr);
		{
			BYTE const lastByte = ((const BYTE *)srcBuffer)[srcSize - 1];
			bitD->bitsConsumed = lastByte ? 8 - BIT_highbit32(lastByte) : 0; /* ensures bitsConsumed is always set */
			if (lastByte == 0)
				return ERROR(GENERIC); /* endMark not present */
		}
	} else {
		bitD->start = (const char *)srcBuffer;
		bitD->ptr = bitD->start;
		bitD->bitContainer = *(const BYTE *)(bitD->start);
		switch (srcSize) {
		case 7: bitD->bitContainer += (size_t)(((const BYTE *)(srcBuffer))[6]) << (sizeof(bitD->bitContainer) * 8 - 16);
		case 6: bitD->bitContainer += (size_t)(((const BYTE *)(srcBuffer))[5]) << (sizeof(bitD->bitContainer) * 8 - 24);
		case 5: bitD->bitContainer += (size_t)(((const BYTE *)(srcBuffer))[4]) << (sizeof(bitD->bitContainer) * 8 - 32);
		case 4: bitD->bitContainer += (size_t)(((const BYTE *)(srcBuffer))[3]) << 24;
		case 3: bitD->bitContainer += (size_t)(((const BYTE *)(srcBuffer))[2]) << 16;
		case 2: bitD->bitContainer += (size_t)(((const BYTE *)(srcBuffer))[1]) << 8;
		default:;
		}
		{
			BYTE const lastByte = ((const BYTE *)srcBuffer)[srcSize - 1];
			bitD->bitsConsumed = lastByte ? 8 - BIT_highbit32(lastByte) : 0;
			if (lastByte == 0)
				return ERROR(GENERIC); /* endMark not present */
		}
		bitD->bitsConsumed += (U32)(sizeof(bitD->bitContainer) - srcSize) * 8;
	}

	return srcSize;
}

ZSTD_STATIC size_t BIT_getUpperBits(size_t bitContainer, U32 const start) { return bitContainer >> start; }

ZSTD_STATIC size_t BIT_getMiddleBits(size_t bitContainer, U32 const start, U32 const nbBits) { return (bitContainer >> start) & BIT_mask[nbBits]; }

ZSTD_STATIC size_t BIT_getLowerBits(size_t bitContainer, U32 const nbBits) { return bitContainer & BIT_mask[nbBits]; }

/*! BIT_lookBits() :
 *  Provides next n bits from local register.
 *  local register is not modified.
 *  On 32-bits, maxNbBits==24.
 *  On 64-bits, maxNbBits==56.
 *  @return : value extracted
 */
ZSTD_STATIC size_t BIT_lookBits(const BIT_DStream_t *bitD, U32 nbBits)
{
	U32 const bitMask = sizeof(bitD->bitContainer) * 8 - 1;
	return ((bitD->bitContainer << (bitD->bitsConsumed & bitMask)) >> 1) >> ((bitMask - nbBits) & bitMask);
}

/*! BIT_lookBitsFast() :
*   unsafe version; only works only if nbBits >= 1 */
ZSTD_STATIC size_t BIT_lookBitsFast(const BIT_DStream_t *bitD, U32 nbBits)
{
	U32 const bitMask = sizeof(bitD->bitContainer) * 8 - 1;
	return (bitD->bitContainer << (bitD->bitsConsumed & bitMask)) >> (((bitMask + 1) - nbBits) & bitMask);
}

ZSTD_STATIC void BIT_skipBits(BIT_DStream_t *bitD, U32 nbBits) { bitD->bitsConsumed += nbBits; }

/*! BIT_readBits() :
 *  Read (consume) next n bits from local register and update.
 *  Pay attention to not read more than nbBits contained into local register.
 *  @return : extracted value.
 */
ZSTD_STATIC size_t BIT_readBits(BIT_DStream_t *bitD, U32 nbBits)
{
	size_t const value = BIT_lookBits(bitD, nbBits);
	BIT_skipBits(bitD, nbBits);
	return value;
}

/*! BIT_readBitsFast() :
*   unsafe version; only works only if nbBits >= 1 */
ZSTD_STATIC size_t BIT_readBitsFast(BIT_DStream_t *bitD, U32 nbBits)
{
	size_t const value = BIT_lookBitsFast(bitD, nbBits);
	BIT_skipBits(bitD, nbBits);
	return value;
}

/*! BIT_reloadDStream() :
*   Refill `bitD` from buffer previously set in BIT_initDStream() .
*   This function is safe, it guarantees it will not read beyond src buffer.
*   @return : status of `BIT_DStream_t` internal register.
			  if status == BIT_DStream_unfinished, internal register is filled with >= (sizeof(bitD->bitContainer)*8 - 7) bits */
ZSTD_STATIC BIT_DStream_status BIT_reloadDStream(BIT_DStream_t *bitD)
{
	if (bitD->bitsConsumed > (sizeof(bitD->bitContainer) * 8)) /* should not happen => corruption detected */
		return BIT_DStream_overflow;

	if (bitD->ptr >= bitD->start + sizeof(bitD->bitContainer)) {
		bitD->ptr -= bitD->bitsConsumed >> 3;
		bitD->bitsConsumed &= 7;
		bitD->bitContainer = ZSTD_readLEST(bitD->ptr);
		return BIT_DStream_unfinished;
	}
	if (bitD->ptr == bitD->start) {
		if (bitD->bitsConsumed < sizeof(bitD->bitContainer) * 8)
			return BIT_DStream_endOfBuffer;
		return BIT_DStream_completed;
	}
	{
		U32 nbBytes = bitD->bitsConsumed >> 3;
		BIT_DStream_status result = BIT_DStream_unfinished;
		if (bitD->ptr - nbBytes < bitD->start) {
			nbBytes = (U32)(bitD->ptr - bitD->start); /* ptr > start */
			result = BIT_DStream_endOfBuffer;
		}
		bitD->ptr -= nbBytes;
		bitD->bitsConsumed -= nbBytes * 8;
		bitD->bitContainer = ZSTD_readLEST(bitD->ptr); /* reminder : srcSize > sizeof(bitD) */
		return result;
	}
}

/*! BIT_endOfDStream() :
*   @return Tells if DStream has exactly reached its end (all bits consumed).
*/
ZSTD_STATIC unsigned BIT_endOfDStream(const BIT_DStream_t *DStream)
{
	return ((DStream->ptr == DStream->start) && (DStream->bitsConsumed == sizeof(DStream->bitContainer) * 8));
}

#endif /* BITSTREAM_H_MODULE */
