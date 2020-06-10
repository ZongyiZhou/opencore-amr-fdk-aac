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

   Description: Fixed point specific mathematical functions for x86

*******************************************************************************/

#if !defined(FIXPOINT_MATH_X86_H)
#define FIXPOINT_MATH_X86_H

#if defined(_MSC_VER) && (_MSC_VER > 1200)
#include <intrin.h>
#include <immintrin.h>

inline int lrintf_fast(const float value) {
  return _mm_cvt_ss2si(_mm_set_ss(value));
}
#define lrintf(v) lrintf_fast(v)
#define lroundf(v) lrintf_fast(v)

inline int lrint_fast(const double value) {
  return _mm_cvtsd_si32(_mm_set_sd(value));
}
#define lrint(v) lrint_fast(v)
#define lround(v) lrint_fast(v)
#endif  // _MSC_VER

#define FUNCTION_sqrtFixp
inline FIXP_DBL sqrtFixp(const FIXP_DBL op) {
#ifdef __LP64__
  return lrint(sqrt((INT64)op << 31));
#else
  return lrint(sqrt(op * (double)(1UL << 31)));
#endif
}

#define FUNCTION_invSqrtNorm2
/**
 * \brief calculate 1.0/sqrt(op)
 * \param op_m mantissa of input value.
 * \param result_e pointer to return the exponent of the result
 * \return mantissa of the result
 */
inline FIXP_DBL invSqrtNorm2(FIXP_DBL op_m, INT *result_e) {
  union {
    double d;
    UINT64 i;
  } result;
  if (op_m == (FIXP_DBL)0) {
    *result_e = 16;
    return ((LONG)0x7fffffff);
  }
  result.d = sqrt(2) / sqrt(op_m);
  UINT mant = (UINT)(result.i >> 22) & 0x3FFFFFFF | 0x40000000;
  *result_e = (INT)(result.i >> 52) - 1023 + 16;
  return (FIXP_DBL)mant;
}

#define FUNCTION_invFixp
/**
 * \brief calculate 1.0/op
 * \param op mantissa of the input value.
 * \return mantissa of the result with implizit exponent of 31
 */
inline FIXP_DBL invFixp(FIXP_DBL op) {
  if ((op == (FIXP_DBL)0) || (op == (FIXP_DBL)1)) {
    return ((LONG)0x7fffffff);
  }
#ifdef __GNUC__
  int q, r;
  __asm__("idiv %2" : "=a"(q), "=d"(r) : "r"(op), "a"(1 << 31), "d"(0));
  return q;
#else
  return (1 << 31) / -op;
#endif
}

/**
 * \brief calculate 1.0/(op_m * 2^op_e)
 * \param op_m mantissa of the input value.
 * \param op_e pointer into were the exponent of the input value is stored, and
 * the result will be stored into.
 * \return mantissa of the result
 */
inline FIXP_DBL invFixp(FIXP_DBL op_m, int *op_e) {
  union {
    double d;
    UINT64 i;
    INT i32[2];
  } result;
  if ((op_m == (FIXP_DBL)0) || (op_m == (FIXP_DBL)1)) {
    *op_e = 31 - *op_e;
    return ((LONG)0x7fffffff);
  }
  result.d = 1.0 / op_m;
  INT mant = (INT)(result.i >> 22) & 0x3FFFFFFF | 0x40000000;
#ifdef __LP64__
  INT sign = (INT64)result.i >> 63;
  *op_e = ((INT)(result.i >> 52) & 0x7FF) - 1023 + 32 - *op_e;
#else
  INT sign = result.i32[1] >> 31;
  *op_e = ((result.i32[1] >> 20) & 0x7FF) - 1023 + 32 - *op_e;
#endif
  return (FIXP_DBL)((mant ^ sign) - sign);
}

#define FUNCTION_schur_div
/**
 * \brief Divide two FIXP_DBL values with given precision.
 * \param num dividend
 * \param denum divisor
 * \param count amount of significant bits of the result (starting to the MSB)
 * \return num/divisor
 */
inline FIXP_DBL schur_div(FIXP_DBL num, FIXP_DBL denum, INT count) {
  (void)count;
  /* same asserts than for fallback implementation */
  FDK_ASSERT(num >= (FIXP_DBL)0);
  FDK_ASSERT(denum > (FIXP_DBL)0);
  FDK_ASSERT(num <= denum);

  return (num == denum) ? (FIXP_DBL)MAXVAL_DBL
                        : (FIXP_DBL)(((INT64)num << 31) / denum);
}

#ifndef __LP64__
#define FUNCTION_llRightShift32
inline UINT llRightShift32(UINT64 value, INT shift) {
#ifdef _MSC_VER
  return (UINT)__ull_rshift(value, shift);
#else
  __asm__("shrd %%cl, %%edx, %%eax" : "+A"(value) : "c"(shift));
  return (UINT)value;
#endif
}
#endif // __LP64__

FDK_INLINE FIXP_DBL fLog2FP(FIXP_DBL x_m, INT x_e, INT *result_e) {
  if (x_m <= FL2FXCONST_DBL(0.0f)) {
    *result_e = DFRACT_BITS - 1;
    x_m = FL2FXCONST_DBL(-1.0f);
  } else {
    UINT clz = CntLeadingZeros(x_m);
    double r = log(x_m) / log(2.0) + (x_e - DFRACT_BITS + 1);
    int offset = x_e - clz + 1;
    if (offset) {
      UINT norm = fNorm((FIXP_DBL)offset);
      r *= 1 << (norm - 1);
      *result_e = DFRACT_BITS - norm;
    } else {
      r *= 1 << 30;
      *result_e = 1;
    }
    x_m = lrint(r);
  }
  return x_m;
}

FDK_INLINE FIXP_DBL fLog2FP(FIXP_DBL x_m, INT x_e) {
  if (x_m <= FL2FXCONST_DBL(0.0f)) {
    x_m = FL2FXCONST_DBL(-1.0f);
  } else {
    const double coef = (double)(DFRACT_FIX_SCALE >> LD_DATA_SHIFT) / log(2.0);
    INT offset = (x_e - DFRACT_BITS + 1) << (DFRACT_BITS - LD_DATA_SHIFT - 1);
    x_m = lrint(log(x_m) * coef) + offset;
  }
  return x_m;
}

#define FUNCTION_fLog2E
FDK_INLINE FIXP_DBL fLog2(FIXP_DBL x_m, INT x_e, INT *result_e) {
  return fLog2FP(x_m, x_e, result_e);
}

#endif /* !defined(FIXPOINT_MATH_X86_H) */
