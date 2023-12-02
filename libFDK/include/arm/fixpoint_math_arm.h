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

   Author(s):   Zongyi Zhou

   Description: Fixed point specific mathematical functions for ARM

*******************************************************************************/

#ifndef FIXPOINT_MATH_ARM_H
#define FIXPOINT_MATH_ARM_H

#ifdef __ARM_ARCH_8__
#define lrint(v) lrint_fast(v)
inline int lrint_fast(double v) {
#ifdef __GNUC__
  int r;
  __asm__ (
    "fcvtns %w0, %d1"
    :"=r"(r): "w"(v)
  );
  return r;
#else
  int64_t x = vcvtnd_s64_f64(v);
  return vqmovnd_s64(x);
#endif // __GNUC__
}

#define FUNCTION_sqrtFixp
inline FIXP_DBL sqrtFixp(const FIXP_DBL op) {
  return lrint(sqrt((double)((INT64)op << 31)));
}

#if defined(__llvm__) || __GNUC__ >= 7
#define FUNCTION_invSqrtNorm2
inline FIXP_DBL invSqrtNorm2(FIXP_DBL op_m, INT *result_e) {
  union {
    double d;
    UINT64 i;
  } result;
  if (op_m == 0) {
    *result_e = 16;
    return 0x7fffffff;
  }
  // d = op_m / 2.0;
#ifdef __GNUC__
  float64x1_t d;
  __asm__ (
    "scvtf %d0, %w1, #1"
    :"=w"(d) :"r"(op_m)
  );
#else
  float64x1_t d = vcvt_n_f64_s64(int64x1_t{op_m}, 1);
#endif
  // Use 2 Newton-Ralphson iterations to reach 32-bit precision
  float64x1_t x0 = vrsqrte_f64(d);
  float64x1_t x1 = x0 * x0;
  x0 = x0 * vrsqrts_f64(d, x1);
  x1 = x0 * x0;
  x0 = x0 * vrsqrts_f64(d, x1);
  result.d = x0[0];
  UINT mant = ((UINT)(result.i >> 22) & 0x3FFFFFFF) | 0x40000000;
  *result_e = (INT)(result.i >> 52) - 1023 + 16;
  return (FIXP_DBL)mant;
}

#define FUNCTION_invFixp
inline FIXP_DBL invFixp(FIXP_DBL op) {
  if (op == 0 || op == -1) {
    return MINVAL_DBL;
  }
  return -(MINVAL_DBL / op);
}

inline FIXP_DBL invFixp(FIXP_DBL op_m, int *op_e) {
  if (op_m == 0) {
    *op_e = 31;
    return MAXVAL_DBL;
  }
#ifdef __GNUC__
  INT cls;
  __asm__("cls %w0, %w1" : "=r"(cls) : "r"(op_m - 1));
  INT64 r = ((1LL << 61) >> cls) / op_m;
  *op_e = cls + 1 - *op_e;
  return (FIXP_DBL)r;
#else
  union {
    double d;
    UINT64 i;
  } result;
  // Use 2 Newton-Ralphson iterations to reach 36-bit precision
  float64x1_t d = {(double)op_m};
  float64x1_t x = vrecpe_f64(d);
  x *= vrecps_f64(d, x);
  x *= vrecps_f64(d, x);
  result.d = x[0];
  INT mant = ((INT)(result.i >> 22) & 0x3FFFFFFF) | 0x40000000;
  *op_e = ((INT)(result.i >> 52) & 0x7FF) - 1023 + 32 - *op_e;
  return (INT64)result.i < 0 ? -mant : mant;
#endif
}
#endif // defined(__llvm__) || __GNUC__ >= 7

#define FUNCTION_schur_div
inline FIXP_DBL schur_div(FIXP_DBL num, FIXP_DBL denum, INT) {
  if ((UINT)num >= (UINT)denum) return MAXVAL_DBL;

  return (FIXP_DBL)(((UINT64)num << 31) / (UINT)denum);
}

#define FUNCTION_fMultNorm
inline FIXP_DBL fMultNorm(FIXP_DBL f1, FIXP_DBL f2, INT *result_e) {
  INT64 product = (INT64)f1 * f2;
  if (product == 0) return 0;

  INT cls;
#ifdef __GNUC__
  __asm__("cls %x0, %x1" : "=r"(cls) : "r"(product));
#else
  cls = fNormz(product ^ (product >> 63));
#endif
  *result_e = 1 - cls;
  return (FIXP_DBL)(product << cls >> 32);
}

inline FIXP_DBL fMultNorm(FIXP_DBL f1, FIXP_DBL f2) { return fMult(f1, f2); }

inline FIXP_DBL fMultNorm(FIXP_DBL f1_m, INT f1_e, FIXP_DBL f2_m, INT f2_e,
                          INT result_e) {
  INT64 product = (INT64)f1_m * f2_m;
  if (product == 0) return 0;

  INT64 scalefactor = f1_e + f2_e - result_e - 31;
  product = vqrshld_s64(product, scalefactor);
  return vqmovnd_s64(product);
}

#define FUNCTION_fMultI
inline INT fMultI(FIXP_DBL a, INT b) {
  INT64 r = (INT64)a * b + (1 << 30);
  return (INT)(r >> 31);
}

#ifdef __GNUC__
#define FUNCTION_fLog2
FDK_INLINE FIXP_DBL fLog2(FIXP_DBL x_m, INT x_e, INT *result_e) {
  if (x_m <= FL2FXCONST_DBL(0.0f)) {
    *result_e = DFRACT_BITS - 1;
    x_m = FL2FXCONST_DBL(-1.0f);
  } else {
    UINT clz = fixnormz_D(x_m);
    int offset = x_e - clz;
    UINT mant = x_m << clz << 1;
    UINT index = mant >> (DFRACT_BITS - LOG2_LUT_SIZE_LOG2);
    UINT frac = mant & LOG2_LUT_MASK;
    UINT64 r = log2_lut[index] * (UINT64)(LOG2_LUT_MASK + 1 - frac) +
               log2_lut[index + 1] * (UINT64)frac;
    int norm;
    __asm__("cls %w0, %w1" : "=r"(norm) : "r"(offset + 1));
    x_m = offset << (norm - 1);
    *result_e = DFRACT_BITS - norm;
    const UINT round_diff = (1 << (LOG2_RESULT_SCALE_BITS - 1)) -
                            (1 << (63 - LOG2_LUT_SIZE_LOG2 - norm));
    x_m += (r - round_diff) >> (64 - LOG2_LUT_SIZE_LOG2 - norm);
  }
  return x_m;
}

FDK_INLINE FIXP_DBL fLog2_lookup(FIXP_DBL x_m, INT x_e);

FDK_INLINE FIXP_DBL fLog2(FIXP_DBL x_m, INT x_e) {
  return fLog2_lookup(x_m, x_e);
}
#endif

#else // 32-bit ARM
#ifdef __ARM_ARCH_5TE__

#ifdef __GNUC__
#define lrint(v) lrint_fast(v)
inline int lrint_fast(double v) {
  int x;
  __asm__ (
    "vcvtr.s32.f64 %0, %P1\n\t"
    :"=t"(x) :"w"(v)
  );
  return x;
}
#endif

#define FUNCTION_schur_div
inline FIXP_DBL schur_div(FIXP_DBL num, FIXP_DBL denum, INT) {
  if ((UINT)num >= (UINT)denum) return MAXVAL_DBL;

  return (INT)((double)num * (1U << 31) / (double)denum);
}

#define FUNCTION_sqrtFixp
inline FIXP_DBL sqrtFixp(const FIXP_DBL op) {
  FIXP_DBL result = lrint(sqrt(op) * sqrt(1UL << 31));
  return result;
}

#define FUNCTION_invSqrtNorm2
inline FIXP_DBL invSqrtNorm2(FIXP_DBL op_m, INT *result_e) {
  union {
    double d;
    UINT64 i;
  } result;
  if (op_m == 0) {
    *result_e = 16;
    return 0x7fffffff;
  }
  result.d = sqrt(2) / sqrt(op_m);
  UINT mant = ((UINT)(result.i >> 22) & 0x3FFFFFFF) | 0x40000000;
  *result_e = (INT)(result.i >> 52) - 1023 + 16;
  return (FIXP_DBL)mant;
}

#define FUNCTION_invFixp
inline FIXP_DBL invFixp(FIXP_DBL op) {
  if ((UINT)op <= 1) {
    return 0x7fffffff;
  }
#ifdef __ARM_FEATURE_IDIV
  return (1 << 31) / -op;
#else
  return (FIXP_DBL)(2147483648.0 / op);
#endif
}

inline FIXP_DBL invFixp(FIXP_DBL op_m, int *op_e) {
  union {
    double d;
    UINT64 i;
    INT i32[2];
  } result;
  if ((UINT)op_m <= 1) {
    *op_e = 31 - *op_e;
    return ((LONG)0x7fffffff);
  }
  result.d = 1.0 / op_m;
  INT mant = ((INT)(result.i >> 22) & 0x3FFFFFFF) | 0x40000000;
  *op_e = ((result.i32[1] >> 20) & 0x7FF) - 1023 + 32 - *op_e;
  return (INT)result.i32[1] < 0 ? -mant : mant;
}

#endif  // __ARM_ARCH_5TE__

#define FUNCTION_fMultNorm
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
  INT cls = fixnormz_D(t) - 1;
  if (cls == 31) cls += fixnormz_D(product.l32 ^ sign);
  *result_e = 1 - cls;
  if (cls < 32) {
#ifdef __GNUC__
    __asm__(
      "rsb %1, %3, #32\n\t"
      "lsl %0, %0, %3\n\t"
#if __thumb__
      "lsr %1, %2, %1\n\t"
      "orr %0, %0, %1"
#else
      "orr %0, %0, %2, lsr %1"
#endif
      : "+r"(product.h32), "=&r"(t)
      : "r"(product.l32), "r"(cls)
    );
#else
    product.i64 <<= cls;
#endif
    return product.h32;
  } else {
    return product.l32 << (cls - 32);
  }
}

inline FIXP_DBL fMultNorm(FIXP_DBL f1, FIXP_DBL f2) { return fMult(f1, f2); }

inline FIXP_DBL fMultNorm(FIXP_DBL f1_m, INT f1_e, FIXP_DBL f2_m, INT f2_e,
                          INT result_e) {
  if ((f1_m | f2_m) == 0) return 0;

  union {
    INT i32[2];
    INT64 i64;
    double d;
  } scalefactor;
#ifdef __ARM_ARCH_7_A__
  INT64 product = (INT64)f1_m * f2_m;
  scalefactor.i32[0] = f1_e + f2_e - result_e - 31;
  scalefactor.i32[1] = scalefactor.i32[0] >> 31;
  int64x1_t t = vqrshl_s64((int64x1_t){product}, (int64x1_t){scalefactor.i64});
  int32x2_t r = vqmovn_s64(vcombine_s64(t, (int64x1_t){0}));
  return r[0];
#else
  scalefactor.i32[0] = 0;
  scalefactor.i32[1] = (f1_e + f2_e - result_e - 31 + 1023) << 20;
  double d = (double)f1_m * f2_m * scalefactor.d;
  return lrint(d);
#endif
}

#ifdef __GNUC__
#define FUNCTION_llRightShift32
inline UINT llRightShift32(UINT64 value, INT shift) {
  union {
    UINT64 u64;
    struct {
      UINT l32;
      UINT h32;
    };
  } v = {value};
  INT s;
  __asm__(
    "rsb %1, %3, #32\n\t"
    "lsr %0, %0, %3\n\t"
#if __thumb__
    "lsl %1, %2, %1\n\t"
    "orr %0, %0, %1"
#else
    "orr %0, %0, %2, lsl %1"
#endif
    : "+r"(v.l32), "=&r"(s)
    : "r"(v.h32), "r"(shift)
  );
  return v.l32;
}
#endif // __GNUC__

#endif // __ARM_ARCH_8__

#define FUNCTION_fMultIfloor
inline INT fMultIfloor(FIXP_DBL a, INT b) { return fMult(a, (FIXP_DBL)b); }

#if defined(__ARM_ARCH_8__) || defined(__ARM_ARCH_5TE__)
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

  INT mant = ((INT)(result.i >> 22) & 0x3FFFFFFF) | 0x40000000;
#ifdef __ARM_ARCH_8__
  *result_e = ((INT)(result.i >> 52) & 0x7FF) - 1023 + 1;
#else
  *result_e = ((result.i32[1] >> 20) & 0x7FF) - 1023 + 1;
#endif  // __ARM_ARCH_8__
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

  INT mant = ((INT)(result.i >> 22) & 0x3FFFFFFF) | 0x40000000;
#ifdef __ARM_ARCH_8__
  INT sign = (INT64)result.i >> 63;
  *result_e = ((INT)(result.i >> 52) & 0x7FF) - 1023 + 1;
#else
  INT sign = result.i32[1] >> 31;
  *result_e = ((result.i32[1] >> 20) & 0x7FF) - 1023 + 1;
#endif  // __ARM_ARCH_8__
  return (FIXP_DBL)((mant ^ sign) - sign);
}

inline FIXP_DBL fDivNormSigned(FIXP_DBL num, FIXP_DBL denom) {
  INT sign_n = num >> 31, sign_d = denom >> 31;
  INT r = schur_div(num ^ sign_n - sign_n, denom ^ sign_d - sign_d, 31);
  sign_n ^= sign_d;
  return r ^ sign_n - sign_n;
}

#endif  // defined(__ARM_ARCH_8__) || defined(__ARM_ARCH_5TE__)

#endif /* !defined(FIXPOINT_MATH_ARM_H) */
