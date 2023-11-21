/* -----------------------------------------------------------------------------
Software License for The Fraunhofer FDK AAC Codec Library for Android

© Copyright  1995 - 2019 Fraunhofer-Gesellschaft zur Förderung der angewandten
Forschung e.V. All rights reserved.

 1.    INTRODUCTION
The Fraunhofer FDK AAC Codec Library for Android ("FDK AAC Codec") is software
that implements the MPEG Advanced Audio Coding ("AAC") encoding and decoding
scheme for digital audio. This FDK AAC Codec software is intended to be used on
a wide variety of Android devices.

AAC's HE-AAC and HE-AAC v2 versions are regarded as today's most efficient
general perceptual audio codecs. AAC-ELD is considered the best-performing
full-bandwidth communications codec by independent studies and is widely
deployed. AAC has been standardized by ISO and IEC as part of the MPEG
specifications.

Patent licenses for necessary patent claims for the FDK AAC Codec (including
those of Fraunhofer) may be obtained through Via Licensing
(www.vialicensing.com) or through the respective patent owners individually for
the purpose of encoding or decoding bit streams in products that are compliant
with the ISO/IEC MPEG audio standards. Please note that most manufacturers of
Android devices already license these patent claims through Via Licensing or
directly from the patent owners, and therefore FDK AAC Codec software may
already be covered under those patent licenses when it is used for those
licensed purposes only.

Commercially-licensed AAC software libraries, including floating-point versions
with enhanced sound quality, are also available from Fraunhofer. Users are
encouraged to check the Fraunhofer website for additional applications
information and documentation.

2.    COPYRIGHT LICENSE

Redistribution and use in source and binary forms, with or without modification,
are permitted without payment of copyright license fees provided that you
satisfy the following conditions:

You must retain the complete text of this software license in redistributions of
the FDK AAC Codec or your modifications thereto in source code form.

You must retain the complete text of this software license in the documentation
and/or other materials provided with redistributions of the FDK AAC Codec or
your modifications thereto in binary form. You must make available free of
charge copies of the complete source code of the FDK AAC Codec and your
modifications thereto to recipients of copies in binary form.

The name of Fraunhofer may not be used to endorse or promote products derived
from this library without prior written permission.

You may not charge copyright license fees for anyone to use, copy or distribute
the FDK AAC Codec software or your modifications thereto.

Your modified versions of the FDK AAC Codec must carry prominent notices stating
that you changed the software and the date of any change. For modified versions
of the FDK AAC Codec, the term "Fraunhofer FDK AAC Codec Library for Android"
must be replaced by the term "Third-Party Modified Version of the Fraunhofer FDK
AAC Codec Library for Android."

3.    NO PATENT LICENSE

NO EXPRESS OR IMPLIED LICENSES TO ANY PATENT CLAIMS, including without
limitation the patents of Fraunhofer, ARE GRANTED BY THIS SOFTWARE LICENSE.
Fraunhofer provides no warranty of patent non-infringement with respect to this
software.

You may use this FDK AAC Codec software or modifications thereto only for
purposes that are authorized by appropriate patent licenses.

4.    DISCLAIMER

This FDK AAC Codec software is provided by Fraunhofer on behalf of the copyright
holders and contributors "AS IS" and WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
including but not limited to the implied warranties of merchantability and
fitness for a particular purpose. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE for any direct, indirect, incidental, special, exemplary,
or consequential damages, including but not limited to procurement of substitute
goods or services; loss of use, data, or profits, or business interruption,
however caused and on any theory of liability, whether in contract, strict
liability, or tort (including negligence), arising in any way out of the use of
this software, even if advised of the possibility of such damage.

5.    CONTACT INFORMATION

Fraunhofer Institute for Integrated Circuits IIS
Attention: Audio and Multimedia Departments - FDK AAC LL
Am Wolfsmantel 33
91058 Erlangen, Germany

www.iis.fraunhofer.de/amm
amm-info@iis.fraunhofer.de
----------------------------------------------------------------------------- */

/******************* Library for basic calculation routines ********************

   Author(s):   M. Lohwasser

   Description: common bitbuffer read/write routines

*******************************************************************************/

#include "FDK_bitbuffer.h"

#include "genericStds.h"
#include "common_fix.h"
#include "fixminmax.h"

#ifndef NEW_BITBUFFER
const UINT BitMask[32 + 1] = {
    0x0,        0x1,        0x3,       0x7,       0xf,       0x1f,
    0x3f,       0x7f,       0xff,      0x1ff,     0x3ff,     0x7ff,
    0xfff,      0x1fff,     0x3fff,    0x7fff,    0xffff,    0x1ffff,
    0x3ffff,    0x7ffff,    0xfffff,   0x1fffff,  0x3fffff,  0x7fffff,
    0xffffff,   0x1ffffff,  0x3ffffff, 0x7ffffff, 0xfffffff, 0x1fffffff,
    0x3fffffff, 0x7fffffff, 0xffffffff};
#endif

void FDK_DeleteBitBuffer(HANDLE_FDK_BITBUF hBitBuf) { ; }

void FDK_InitBitBuffer(HANDLE_FDK_BITBUF hBitBuf, UCHAR *pBuffer, UINT bufSize,
                       UINT validBits, UCHAR config) {
  hBitBuf->ValidBits = validBits;
  hBitBuf->ReadOffset = 0;
  hBitBuf->BitNdx = 0;

  hBitBuf->Buffer = pBuffer;
  hBitBuf->bufSize = bufSize;
  hBitBuf->bufBits = (bufSize << 3);

  /*assure bufsize (2^n) */
  FDK_ASSERT(hBitBuf->ValidBits <= hBitBuf->bufBits);
  FDK_ASSERT((bufSize > 0) && (bufSize <= MAX_BUFSIZE_BYTES));
  FDK_ASSERT(!(bufSize & (bufSize - 1)));

#ifdef NEW_BITBUFFER
  FDK_ASSERT(!((uintptr_t)pBuffer & (sizeof(uintptr_t) - 1)));
  hBitBuf->WordCache = 0;
  hBitBuf->WordPos = 0;
  hBitBuf->WordPosMask = (hBitBuf->bufBits >> BIT_OFFSET_SHIFT) - 1;
#endif
}

void FDK_ResetBitBuffer(HANDLE_FDK_BITBUF hBitBuf) {
  hBitBuf->ValidBits = 0;
  hBitBuf->ReadOffset = 0;
  hBitBuf->BitNdx = 0;
#ifdef NEW_BITBUFFER
  hBitBuf->WordPos = 0;
  hBitBuf->WordCache = 0;
#endif
}

#if defined(NEW_BITBUFFER) && !defined(FUNCTION_FDK_get)
INT FDK_get(HANDLE_FDK_BITBUF hBitBuf, const UINT numberOfBits) {
  FDK_ASSERT(numberOfBits > 0);
  hBitBuf->ValidBits -= numberOfBits;
  uintptr_t r = hBitBuf->WordCache >> (BITS_IN_WORD - numberOfBits);
  UINT bitsInCache = hBitBuf->BitNdx;
  INT shift = bitsInCache - numberOfBits;
  if (shift > 0) {
    hBitBuf->WordCache <<= numberOfBits;
    bitsInCache = shift;
  } else {
    UINT wordPos = hBitBuf->WordPos;
    uintptr_t newData = FDK_WORD_BE(((uintptr_t *)hBitBuf->Buffer)[wordPos]);
    hBitBuf->WordPos = (wordPos + 1) & hBitBuf->WordPosMask;
    if (shift < 0) {
      r |= newData >> (shift + BITS_IN_WORD);
      bitsInCache = shift + BITS_IN_WORD;
      hBitBuf->WordCache = newData << -shift;
    } else {
      bitsInCache = BITS_IN_WORD;
      hBitBuf->WordCache = newData;
    }
  }
  hBitBuf->BitNdx = bitsInCache;
  return (INT)r;
}
#endif /* #ifndef FUNCTION_FDK_get */

#ifndef NEW_BITBUFFER
#ifndef FUNCTION_FDK_get32
INT FDK_get32(HANDLE_FDK_BITBUF hBitBuf) {
  UINT BitNdx = hBitBuf->BitNdx + 32;
  hBitBuf->BitNdx = BitNdx & (hBitBuf->bufBits - 1);
  hBitBuf->ValidBits = (UINT)((INT)hBitBuf->ValidBits - (INT)32);

  UINT byteOffset = (BitNdx - 1) >> 3;
  if (BitNdx <= hBitBuf->bufBits) {
    UINT cache = (hBitBuf->Buffer[(byteOffset - 3)] << 24) |
                 (hBitBuf->Buffer[(byteOffset - 2)] << 16) |
                 (hBitBuf->Buffer[(byteOffset - 1)] << 8) |
                 hBitBuf->Buffer[(byteOffset - 0)];

    if ((BitNdx = (BitNdx & 7)) != 0) {
      cache = (cache >> (8 - BitNdx)) |
              ((UINT)hBitBuf->Buffer[byteOffset - 4] << (24 + BitNdx));
    }
    return (cache);
  } else {
    UINT byte_mask = hBitBuf->bufSize - 1;
    UINT cache = (hBitBuf->Buffer[(byteOffset - 3) & byte_mask] << 24) |
                 (hBitBuf->Buffer[(byteOffset - 2) & byte_mask] << 16) |
                 (hBitBuf->Buffer[(byteOffset - 1) & byte_mask] << 8) |
                 hBitBuf->Buffer[(byteOffset - 0) & byte_mask];

    if ((BitNdx = (BitNdx & 7)) != 0) {
      cache = (cache >> (8 - BitNdx)) |
              ((UINT)hBitBuf->Buffer[(byteOffset - 4) & byte_mask]
               << (24 + BitNdx));
    }
    return (cache);
  }
}
#endif

#else // NEW_BITBUFFER
void FDK_syncWriter(HANDLE_FDK_BITBUF hBitBuf) {
  UINT bitsInCache = hBitBuf->BitNdx;
  if (bitsInCache) {
    UINT wordPos = hBitBuf->WordPos;
    uintptr_t *buf = (uintptr_t *)hBitBuf->Buffer;
    uintptr_t t = FDK_WORD_BE(buf[wordPos]) & (UINTPTR_MAX >> bitsInCache);
    buf[wordPos] =
        FDK_WORD_BE((hBitBuf->WordCache << (BITS_IN_WORD - bitsInCache)) | t);
  }
}

void FDK_flushWriter(HANDLE_FDK_BITBUF hBitBuf) {
  UINT bitsInCache = hBitBuf->BitNdx;
  if (bitsInCache) {
    ((uintptr_t *)hBitBuf->Buffer)[hBitBuf->WordPos] =
        FDK_WORD_BE(hBitBuf->WordCache << (BITS_IN_WORD - bitsInCache));
  }
}
#endif

void FDK_put(HANDLE_FDK_BITBUF hBitBuf, UINT value, const UINT numberOfBits) {
  if (numberOfBits != 0) {
#ifdef NEW_BITBUFFER
    UINT bitsInCache = hBitBuf->BitNdx;
    INT remainingBits = bitsInCache + numberOfBits - BITS_IN_WORD;
    if (remainingBits <= 0) {
      uintptr_t t = (hBitBuf->WordCache << numberOfBits) | value;
      if (remainingBits == 0) {
        ((uintptr_t *)hBitBuf->Buffer)[hBitBuf->WordPos++] = FDK_WORD_BE(t);
        //hBitBuf->WordCache = 0;
        hBitBuf->BitNdx = 0;
      } else {
        hBitBuf->WordCache = t;
        hBitBuf->BitNdx = bitsInCache + numberOfBits;
      }
    } else {
      uintptr_t t = (hBitBuf->WordCache << (BITS_IN_WORD - bitsInCache)) |
                    (value >> remainingBits);
      ((uintptr_t *)hBitBuf->Buffer)[hBitBuf->WordPos++] = FDK_WORD_BE(t);
      hBitBuf->WordCache = value;
      hBitBuf->BitNdx = remainingBits;
    }
#else
    UINT byteOffset0 = hBitBuf->BitNdx >> 3;
    UINT bitOffset = hBitBuf->BitNdx & 0x7;

    hBitBuf->BitNdx = (hBitBuf->BitNdx + numberOfBits) & (hBitBuf->bufBits - 1);

    //UINT byteMask = hBitBuf->bufSize - 1;

    UINT byteOffset1 = (byteOffset0 + 1);
    UINT byteOffset2 = (byteOffset0 + 2);
    UINT byteOffset3 = (byteOffset0 + 3);

    // Create tmp containing free bits at the left border followed by bits to
    // write, LSB's are cleared, if available Create mask to apply upon all
    // buffer bytes
    UINT tmp = (value << (32 - numberOfBits)) >> bitOffset;
    UINT mask = ~((BitMask[numberOfBits] << (32 - numberOfBits)) >> bitOffset);

    // read all 4 bytes from buffer and create a 32-bit cache
    UINT cache = (((UINT)hBitBuf->Buffer[byteOffset0]) << 24) |
                 (((UINT)hBitBuf->Buffer[byteOffset1]) << 16) |
                 (((UINT)hBitBuf->Buffer[byteOffset2]) << 8) |
                 (((UINT)hBitBuf->Buffer[byteOffset3]) << 0);

    cache = (cache & mask) | tmp;
    hBitBuf->Buffer[byteOffset0] = (UCHAR)(cache >> 24);
    hBitBuf->Buffer[byteOffset1] = (UCHAR)(cache >> 16);
    hBitBuf->Buffer[byteOffset2] = (UCHAR)(cache >> 8);
    hBitBuf->Buffer[byteOffset3] = (UCHAR)(cache >> 0);

    if ((bitOffset + numberOfBits) > 32) {
      UINT byteOffset4 = (byteOffset0 + 4);
      // remaining bits: in range 1..7
      // replace MSBits of next byte in buffer by LSBits of "value"
      int bits = (bitOffset + numberOfBits) & 7;
      cache =
          (UINT)hBitBuf->Buffer[byteOffset4] & (~(BitMask[bits] << (8 - bits)));
      cache |= value << (8 - bits);
      hBitBuf->Buffer[byteOffset4] = (UCHAR)cache;
    }
#endif
    hBitBuf->ValidBits += numberOfBits;
  }
}

void FDK_pushBackReader(HANDLE_FDK_BITBUF hBitBuf, const UINT numberOfBits) {
#ifdef NEW_BITBUFFER
  hBitBuf->ValidBits += numberOfBits;
  INT bitsInCache = numberOfBits + hBitBuf->BitNdx;
  UINT wordPos = hBitBuf->WordPos;
  if (bitsInCache > BITS_IN_WORD) {
    wordPos -= (bitsInCache - 1) >> BIT_OFFSET_SHIFT;
    hBitBuf->WordPos = wordPos & hBitBuf->WordPosMask;
  }
  uintptr_t newData = FDK_WORD_BE(
      ((uintptr_t *)hBitBuf->Buffer)[(wordPos - 1) & hBitBuf->WordPosMask]);
  bitsInCache = -bitsInCache & BIT_OFFSET_MASK;
  hBitBuf->WordCache = newData << bitsInCache;
  hBitBuf->BitNdx = BITS_IN_WORD - bitsInCache;
#else
  hBitBuf->ValidBits = (UINT)((INT)hBitBuf->ValidBits + (INT)numberOfBits);
  hBitBuf->BitNdx = ((UINT)((INT)hBitBuf->BitNdx - (INT)numberOfBits)) &
                    (hBitBuf->bufBits - 1);
#endif
}

void FDK_pushBackWriter(HANDLE_FDK_BITBUF hBitBuf, const UINT numberOfBits) {
#ifdef NEW_BITBUFFER
  UINT bitsInCache = hBitBuf->BitNdx;
  hBitBuf->ValidBits -= numberOfBits;
  UINT wordPos = hBitBuf->WordPos;
  uintptr_t *buf = (uintptr_t *)hBitBuf->Buffer;
  if (bitsInCache) {
    uintptr_t t = FDK_WORD_BE(buf[wordPos]);
    t &= UINTPTR_MAX >> (BITS_IN_WORD - bitsInCache);
    buf[wordPos] =
        FDK_WORD_BE((hBitBuf->WordCache << (BITS_IN_WORD - bitsInCache)) | t);
  }
  wordPos = hBitBuf->ValidBits >> BIT_OFFSET_SHIFT;
  bitsInCache = hBitBuf->ValidBits & BIT_OFFSET_MASK;
  if (bitsInCache) {
    hBitBuf->WordCache =
        FDK_WORD_BE(buf[wordPos] >> (BITS_IN_WORD - bitsInCache));
  }
  hBitBuf->WordPos = wordPos;
  hBitBuf->BitNdx = bitsInCache;
#else
  hBitBuf->ValidBits = (UINT)((INT)hBitBuf->ValidBits - (INT)numberOfBits);
  hBitBuf->BitNdx = ((UINT)((INT)hBitBuf->BitNdx - (INT)numberOfBits)) &
                    (hBitBuf->bufBits - 1);
#endif
}

void FDK_pushForwardReader(HANDLE_FDK_BITBUF hBitBuf, const UINT numberOfBits) {
#ifdef NEW_BITBUFFER
  hBitBuf->ValidBits -= numberOfBits;
  UINT bitsInCache = hBitBuf->BitNdx;
  if (bitsInCache > numberOfBits) {
    bitsInCache -= numberOfBits;
    hBitBuf->WordCache <<= numberOfBits;
  } else {
    UINT wordPos = hBitBuf->WordPos + ((numberOfBits - bitsInCache) >> BIT_OFFSET_SHIFT);
    bitsInCache = (numberOfBits - bitsInCache) & BIT_OFFSET_MASK;
    hBitBuf->WordCache = FDK_WORD_BE(((uintptr_t *)hBitBuf->Buffer)[wordPos & hBitBuf->WordPosMask])
                         << bitsInCache;
    hBitBuf->WordPos = (wordPos + 1) & hBitBuf->WordPosMask;
    bitsInCache = BITS_IN_WORD - bitsInCache;
  }
  hBitBuf->BitNdx = bitsInCache;
#else
  hBitBuf->ValidBits = (UINT)((INT)hBitBuf->ValidBits - (INT)numberOfBits);
  hBitBuf->BitNdx =
      (UINT)((INT)hBitBuf->BitNdx + (INT)numberOfBits) & (hBitBuf->bufBits - 1);
#endif
}

void FDK_pushForwardWriter(HANDLE_FDK_BITBUF hBitBuf, const UINT numberOfBits) {
#ifdef NEW_BITBUFFER
  UINT bitsInCache = hBitBuf->BitNdx;
  hBitBuf->ValidBits += numberOfBits;
  UINT wordPos = hBitBuf->WordPos;
  uintptr_t *buf = (uintptr_t *)hBitBuf->Buffer;
  if (bitsInCache) {
    uintptr_t t = FDK_WORD_BE(buf[wordPos]);
    INT remainingBits = bitsInCache + numberOfBits - BITS_IN_WORD;
    if (remainingBits < 0) {
      // Blend nBits from buffer
      t <<= bitsInCache;
      hBitBuf->WordCache = (hBitBuf->WordCache << numberOfBits) |
                           (t >> (BITS_IN_WORD - numberOfBits));
      hBitBuf->BitNdx = bitsInCache + numberOfBits;
      return;
    }
    t &= UINTPTR_MAX >> (BITS_IN_WORD - bitsInCache);
    buf[wordPos] =
        FDK_WORD_BE((hBitBuf->WordCache << (BITS_IN_WORD - bitsInCache)) | t);
  }
  wordPos = hBitBuf->ValidBits >> BIT_OFFSET_SHIFT;
  bitsInCache = hBitBuf->ValidBits & BIT_OFFSET_MASK;
  if (bitsInCache) {
    hBitBuf->WordCache = FDK_WORD_BE(buf[wordPos]) >> (BITS_IN_WORD - bitsInCache);
  }
  hBitBuf->WordPos = wordPos;
  hBitBuf->BitNdx = bitsInCache;
#else
  hBitBuf->ValidBits = (UINT)((INT)hBitBuf->ValidBits + (INT)numberOfBits);
  hBitBuf->BitNdx =
      (UINT)((INT)hBitBuf->BitNdx + (INT)numberOfBits) & (hBitBuf->bufBits - 1);
#endif
}

UINT FDK_getValidBits(HANDLE_FDK_BITBUF hBitBuf) { return hBitBuf->ValidBits; }

INT FDK_getFreeBits(HANDLE_FDK_BITBUF hBitBuf) {
  return (hBitBuf->bufBits - hBitBuf->ValidBits);
}

void FDK_Feed(HANDLE_FDK_BITBUF hBitBuf, const UCHAR *RESTRICT inputBuffer,
              const UINT bufferSize, UINT *bytesValid) {
  inputBuffer = &inputBuffer[bufferSize - *bytesValid];

  UINT bTotal = 0;

  UINT bToRead =
      fMin(hBitBuf->bufBits,
           (UINT)fMax(0, (INT)(hBitBuf->bufBits - hBitBuf->ValidBits))) >>
      3;
  UINT noOfBytes = fMin(bToRead, *bytesValid);

  while (noOfBytes > 0) {
    /* split read to buffer size */
    bToRead = hBitBuf->bufSize - hBitBuf->ReadOffset;
    bToRead = fMin(bToRead, noOfBytes);

    /* copy 'bToRead' bytes from 'ptr' to inputbuffer */
    FDKmemcpy(&hBitBuf->Buffer[hBitBuf->ReadOffset], inputBuffer,
              bToRead * sizeof(UCHAR));

    /* add noOfBits to number of valid bits in buffer */
    hBitBuf->ValidBits = (UINT)((INT)hBitBuf->ValidBits + (INT)(bToRead << 3));
    bTotal += bToRead;
    inputBuffer += bToRead;

    hBitBuf->ReadOffset =
        (hBitBuf->ReadOffset + bToRead) & (hBitBuf->bufSize - 1);
    noOfBytes -= bToRead;
  }
#ifdef NEW_BITBUFFER
  if (hBitBuf->BitNdx) FDK_pushBackReader(hBitBuf, 0);  // Force cache update
#endif
  *bytesValid -= bTotal;
}

void FDK_Fetch(HANDLE_FDK_BITBUF hBitBuf, UCHAR *outBuf, UINT *writeBytes) {
  UCHAR *RESTRICT outputBuffer = outBuf;
  UINT bTotal = 0;

  UINT bToWrite = (hBitBuf->ValidBits) >> 3;
  UINT noOfBytes = fMin(bToWrite, *writeBytes);

  while (noOfBytes > 0) {
    /* split write to buffer size */
    bToWrite = hBitBuf->bufSize - hBitBuf->WriteOffset;
    bToWrite = fMin(bToWrite, noOfBytes);

    /* copy 'bToWrite' bytes from bitbuffer to outputbuffer */
    FDKmemcpy(outputBuffer, &hBitBuf->Buffer[hBitBuf->WriteOffset],
              bToWrite * sizeof(UCHAR));

    /* sub noOfBits from number of valid bits in buffer */
    hBitBuf->ValidBits -= bToWrite << 3;
    bTotal += bToWrite;
    outputBuffer += bToWrite;

    hBitBuf->WriteOffset =
        (hBitBuf->WriteOffset + bToWrite) & (hBitBuf->bufSize - 1);
    noOfBytes -= bToWrite;
  }

  *writeBytes = bTotal;
}
