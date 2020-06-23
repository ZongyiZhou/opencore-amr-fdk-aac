/* -----------------------------------------------------------------------------
Software License for The Fraunhofer FDK AAC Codec Library for Android

© Copyright  1995 - 2018 Fraunhofer-Gesellschaft zur Förderung der angewandten
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

   Author(s):   Manuel Jander

   Description: x86 version of count leading zero / bits

*******************************************************************************/

#if !defined(CLZ_X86_H)
#define CLZ_X86_H

#ifdef __GNUC__

#include <x86intrin.h>

#define FUNCTION_fixnormz_D
#define FUNCTION_fixnorm_D

inline INT fixnormz_D(LONG value) {
#if !defined(__LZCNT__) || defined(__SANITIZE_ADDRESS__)
  if (value == 0) return 32;
#endif
  return __builtin_clz(value);
}

#ifdef __x86_64__
#define FUNCTION_fixnormz64
inline INT fNormz(INT64 value) {
#if !defined(__LZCNT__) || defined(__SANITIZE_ADDRESS__)
  if (value == 0) return 64;
#endif
  return (INT)__builtin_clzll(value);
}

// clz without zero check
inline INT clz64(INT64 value) {
#ifdef __LZCNT__
  return (INT)__builtin_clzll(value);
#else
  __asm__("bsrq %1, %0" : "=r"(value) : "r"(value));
  return (INT)(63 - value);
#endif
}
#endif

// clz without zero check
inline INT clz32(INT value) {
#ifdef __LZCNT__
  return __builtin_clz(value);
#else
  __asm__("bsrl %1, %0" : "=r"(value) : "r"(value));
  return (INT)(31 - value);
#endif
}
#elif _MSC_VER >= 1500

#include <intrin.h>

#define FUNCTION_fixnormz_D
#define FUNCTION_fixnorm_D
#define FUNCTION_fixnormz_S

inline INT fixnormz_D(LONG value) {
#ifdef __AVX2__
  return _lzcnt_u32(value);
#else
  if (value == 0) return 32;
  unsigned long result;
  _BitScanReverse(&result, value);
  return result ^ 31;
#endif
}

#ifdef _M_X64
#define FUNCTION_fixnormz64
inline INT fNormz(INT64 value) {
#ifdef __AVX2__
  return (INT)_lzcnt_u64(value);
#else
  if (value == 0) return 64;
  unsigned long result;
  _BitScanReverse64(&result, value);
  return result ^ 63;
#endif
}

// clz without zero check
inline INT clz64(INT64 value) {
#ifdef __AVX2__
  return (INT)_lzcnt_u64(value);
#else
  unsigned long result;
  _BitScanReverse64(&result, value);
  return 63 - result;
#endif
}
#endif

inline INT fixnormz_S(SHORT value) {
#ifdef __AVX2__
  return _lzcnt_u32(value) - 16;
#else
  if (value == 0) return 16;
  unsigned long result;
  _BitScanReverse(&result, value);
  return result ^ 15;
#endif
}

inline INT clz32(INT value) {
#ifdef __AVX2__
  return (INT)_lzcnt_u32(value);
#else
  unsigned long result;
  _BitScanReverse(&result, value);
  return 31 - result;
#endif
}

#endif /* toolchain */

inline INT fixnorm_D(LONG value) {
  if (value == 0) {
    return 0;
  }
  value ^= value >> 31;
  return fixnormz_D(value) - 1;
}

#endif /* !defined(CLZ_X86_H) */
