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
  return lrint(sqrt((double)((INT64)op << 31)));
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
 * \return mantissa of the result with implicit exponent of 31
 */
inline FIXP_DBL invFixp(FIXP_DBL op) {
#ifdef __GNUC__
  if (op == 0 || op == 1) {
    return MAXVAL_DBL;
  }
  int q, r;
  __asm__("idiv %2" : "=a"(q), "=d"(r) : "r"(op), "a"(1U << 31), "d"(0));
  return q;
#elif defined(_MSC_VER) && _MSC_VER >= 1920
  if (op == 0 || op == 1) {
    return MAXVAL_DBL;
  }
  int r;
  return _div64(1U << 31, op, &r);
#else
  if (op == 0 || op == -1) {
    return MINVAL_DBL;
  }
  return -(MINVAL_DBL / op);
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
  if (op_m == 0) {
    *op_e = 31;
    return MAXVAL_DBL;
  }
#ifdef __GNUC__
  INT cls = clz32((op_m - 1) ^ (op_m >> 31));
  INT r;
  __asm__(
    "xor %%eax, %%eax\n\t"
    "mov $0x40000000, %%edx\n\t"
    "shrd %%cl, %%edx, %%eax\n\t"
    "shr %%cl, %%edx\n\t"
    "idivl %1"
    : "=a"(r)
    : "r"(op_m), "c"(cls)
    : "edx"
  );
  *op_e = cls - *op_e;
  return r;
#elif defined(_MSC_VER) && _MSC_VER >= 1920
  INT cls = clz32((op_m - 1) ^ (op_m >> 31));
  INT64 num = __ull_rshift(1ULL << 62, cls);
  INT r, q = _div64(num, op_m, &r);
  return q;
#else
  union {
    double d;
    UINT64 i;
    INT i32[2];
  } result;
  result.d = 1.0 / op_m;
  INT mant = (INT)(result.i >> 22) & 0x3FFFFFFF | 0x40000000;
#ifdef __LP64__
  INT sign = (INT64)result.i >> 63;
  *op_e = ((INT)(result.i >> 52) & 0x7FF) - 1023 + 32 - *op_e;
#else
  INT sign = result.i32[1] >> 31;
  *op_e = ((result.i32[1] >> 20) & 0x7FF) - 1023 + 32 - *op_e;
#endif  // __LP64__
  return (FIXP_DBL)((mant ^ sign) - sign);
#endif
}

#define FUNCTION_schur_div
/**
 * \brief Divide two FIXP_DBL values with given precision.
 * \param num dividend
 * \param denum divisor
 * \param count amount of significant bits of the result (starting to the MSB)
 * \return num/divisor
 */
inline FIXP_DBL schur_div(FIXP_DBL num, FIXP_DBL denum, INT) {
  /* same asserts than for fallback implementation */
  FDK_ASSERT(num >= (FIXP_DBL)0);
  FDK_ASSERT(denum > (FIXP_DBL)0);
  FDK_ASSERT(num <= denum);

  if ((UINT)num >= (UINT)denum) return MAXVAL_DBL;
#ifdef __GNUC__
  INT q, r;
  __asm__(
    "div %2"
    : "=a"(q), "=d"(r)
    : "r"(denum), "a"(num << 31), "d"((UINT)num >> 1)
  );
  return q;
#elif defined(_MSC_VER) && _MSC_VER >= 1920
  UINT q, r;
  q = _udiv64((UINT64)num << 31, (UINT)denum, &r);
  return q;
#else
  return (FIXP_DBL)(((UINT64)num << 31) / (UINT)denum);
#endif
}

#define FUNCTION_fMultIfloor
inline INT fMultIfloor(FIXP_DBL a, INT b) { return fMult(a, (FIXP_DBL)b); }

#define FUNCTION_fMultNorm
inline FIXP_DBL fMultNorm(FIXP_DBL f1, FIXP_DBL f2) { return fMult(f1, f2); }

#ifdef __LP64__
inline FIXP_DBL fMultNorm(FIXP_DBL f1, FIXP_DBL f2, INT *result_e) {
  INT64 product = (INT64)f1 * f2;
  if (product == 0) return 0;

  INT cls = clz64(product ^ (product >> 63)) - 1;
  *result_e = 1 - cls;
  return (FIXP_DBL)(product << cls >> 32);
}

inline FIXP_DBL fMultNorm(FIXP_DBL f1_m, INT f1_e, FIXP_DBL f2_m, INT f2_e,
                          INT result_e) {
  INT64 product = (INT64)f1_m * f2_m;
  if (product == 0) return 0;

  INT64 scalefactor = f1_e + f2_e - result_e - 31;
  scalefactor <<= 52;
  __m128d d = _mm_cvtsi64_sd(_mm_setzero_pd(), product);
  __m128i i64 = _mm_cvtsi64_si128(scalefactor);
  d = _mm_castsi128_pd(_mm_add_epi16(i64, _mm_castpd_si128(d)));
  INT s = (product | scalefactor) >> 32;
  INT r = _mm_cvtsd_si32(d);
  return (~s & r) < 0 ? MAXVAL_DBL : r;
}

#define FUNCTION_fMultI
inline INT fMultI(FIXP_DBL a, INT b) {
  INT64 r = (INT64)a * b + (1 << 30);
  return (INT)(r >> 31);
}

#else
inline FIXP_DBL fMultNorm(FIXP_DBL f1, FIXP_DBL f2, INT *result_e) {
  if ((f1 | f2) == 0) return 0;
  union {
    INT64 i64;
    struct {
      UINT l32;
      INT h32;
    };
  } product;
  product.i64 = (INT64)f1 * f2;

  INT sign = product.h32 >> 31;
  INT t = product.h32 ^ sign;
  INT cls = clz32(t) - 1;
  if (cls == 31) cls += clz32(product.l32 ^ sign);
  *result_e = 1 - cls;
  if (cls < 32) {
#ifdef __GNUC__
    __asm__(
      "shld %%cl, %1, %0"
      : "+r"(product.h32)
      : "r"(product.l32), "c"(cls)
    );
#elif defined(_MSC_VER) && _MSC_VER >= 1900
    product.i64 = __ll_lshift(product.i64, cls);
#else
    product.i64 <<= cls;
#endif
    return product.h32;
  } else {
    return product.l32 << (cls - 32);
  }
}

inline FIXP_DBL fMultNorm(FIXP_DBL f1_m, INT f1_e, FIXP_DBL f2_m, INT f2_e,
                          INT result_e) {
  if ((f1_m | f2_m) == 0) return 0;
  double product = (double)f1_m * f2_m;

  INT scalefactor = f1_e + f2_e - result_e - 31;
  __m128d d = _mm_set_pd(0, product);
  __m128i i64 = _mm_slli_epi64(_mm_cvtsi32_si128(scalefactor), 52);
  d = _mm_castsi128_pd(_mm_add_epi16(i64, _mm_castpd_si128(d)));
  INT s = ~(f1_m ^ f2_m | scalefactor);
  INT r = _mm_cvtsd_si32(d);
  return (s & r) < 0 ? MAXVAL_DBL : r;
}

#define FUNCTION_llRightShift32
inline UINT llRightShift32(UINT64 value, INT shift) {
#ifdef __GNUC__
  union {
    UINT64 u64;
    struct {
      UINT l32;
      UINT h32;
    };
  } v = {value};
  __asm__("shrd %%cl, %1, %0" : "+r"(v.l32) : "r"(v.h32), "c"(shift));
  return v.l32;
#elif defined(_MSC_VER) && _MSC_VER >= 1900
  return (UINT)__ull_rshift(value, shift);
#else
  return (UINT)(value >> shift);
#endif
}
#endif  // __LP64__

#define FUNCTION_fDivNorm
inline FIXP_DBL fDivNorm(FIXP_DBL L_num, FIXP_DBL L_denum, INT *result_e) {
  FDK_ASSERT(L_num >= (FIXP_DBL)0);
  FDK_ASSERT(L_denum > (FIXP_DBL)0);

  if (L_num == 0) {
    *result_e = 0;
    return 0;
  }

  union {
    double d;
    UINT64 i;
    INT i32[2];
  } result;
  result.d = (double)L_num / L_denum;

  INT mant = (INT)(result.i >> 22) & 0x3FFFFFFF | 0x40000000;
#ifdef __LP64__
  *result_e = ((INT)(result.i >> 52) & 0x7FF) - 1023 + 1;
#else
  *result_e = ((result.i32[1] >> 20) & 0x7FF) - 1023 + 1;
#endif  // __LP64__
  return mant;
}

inline FIXP_DBL fDivNorm(FIXP_DBL num, FIXP_DBL denom) {
  return schur_div(num, denom, 31);
}

#define FUNCTION_fDivNormSigned
inline FIXP_DBL fDivNormSigned(FIXP_DBL L_num, FIXP_DBL L_denum,
                               INT *result_e) {
  FDK_ASSERT(L_denum);
  if (L_num == 0) {
    *result_e = 0;
    return 0;
  }

  union {
    double d;
    UINT64 i;
    INT i32[2];
  } result;
  result.d = (double)L_num / L_denum;

  INT mant = (INT)(result.i >> 22) & 0x3FFFFFFF | 0x40000000;
#ifdef __LP64__
  INT sign = (INT64)result.i >> 63;
  *result_e = ((INT)(result.i >> 52) & 0x7FF) - 1023 + 1;
#else
  INT sign = result.i32[1] >> 31;
  *result_e = ((result.i32[1] >> 20) & 0x7FF) - 1023 + 1;
#endif  // __LP64__
  return (FIXP_DBL)((mant ^ sign) - sign);
}

inline FIXP_DBL fDivNormSigned(FIXP_DBL num, FIXP_DBL denom) {
  INT sign_n = num >> 31, sign_d = denom >> 31;
  INT r = schur_div(num ^ sign_n - sign_n, denom ^ sign_d - sign_d, 31);
  sign_n ^= sign_d;
  return r ^ sign_n - sign_n;
}

inline FIXP_DBL fLog2FP(FIXP_DBL x_m, INT x_e, INT *result_e) {
  if (x_m <= FL2FXCONST_DBL(0.0f)) {
    *result_e = DFRACT_BITS - 1;
    x_m = FL2FXCONST_DBL(-1.0f);
  } else {
    UINT clz = clz32(x_m);
    double r = log(x_m) / log(2.0) + (x_e - DFRACT_BITS + 1);
    int offset = x_e - clz + 1;
    UINT norm = clz32(offset ^ (offset >> 31));
    r *= 1 << (norm - 2);
    *result_e = DFRACT_BITS + 1 - norm;
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

#define FUNCTION_fLog2
FDK_INLINE FIXP_DBL fLog2(FIXP_DBL x_m, INT x_e, INT *result_e) {
  return fLog2FP(x_m, x_e, result_e);
}

FDK_INLINE FIXP_DBL fLog2_lookup(FIXP_DBL x_m, INT x_e);

FDK_INLINE FIXP_DBL fLog2(FIXP_DBL x_m, INT x_e) {
  return fLog2_lookup(x_m, x_e);
}
#endif /* !defined(FIXPOINT_MATH_X86_H) */
