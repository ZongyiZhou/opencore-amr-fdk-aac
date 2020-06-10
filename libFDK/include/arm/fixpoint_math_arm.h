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
  int64x1_t x = vcvtn_s64_f64((float64x1_t){v});
  return vqmovnd_s64(x[0]);
#endif // __GNUC__
}

#define FUNCTION_sqrtFixp
inline FIXP_DBL sqrtFixp(const FIXP_DBL op) {
  return lrint(sqrt((INT64)op << 31));
}

#define FUNCTION_invSqrtNorm2
inline FIXP_DBL invSqrtNorm2(FIXP_DBL op_m, INT *result_e) {
  union {
    double d;
    UINT64 i;
  } result;
  if (op_m == (FIXP_DBL)0) {
    *result_e = 16;
    return ((LONG)0x7fffffff);
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
  if ((op == (FIXP_DBL)0) || (op == (FIXP_DBL)1)) {
    return 0x7fffffff;
  }
  return (1 << 31) / -op;
}

inline FIXP_DBL invFixp(FIXP_DBL op_m, int *op_e) {
  union {
    double d;
    UINT64 i;
  } result;
  if ((op_m == (FIXP_DBL)0) || (op_m == (FIXP_DBL)1)) {
    *op_e = 31 - *op_e;
    return ((LONG)0x7fffffff);
  }
  // Use 2 Newton-Ralphson iterations to reach 36-bit precision
  float64x1_t d = {(double)op_m};
  float64x1_t x = vrecpe_f64(d);
  x *= vrecps_f64(d, x);
  x *= vrecps_f64(d, x);
  result.d = x[0];
  INT mant = ((INT)(result.i >> 22) & 0x3FFFFFFF) | 0x40000000;
  *op_e = ((INT)(result.i >> 52) & 0x7FF) - 1023 + 32 - *op_e;
  return (INT64)result.i < 0 ? -mant : mant;
}

#elif defined(__ARM_ARCH_5TE__)

#ifdef __GNUC__
#define lrint(v) lrint_fast(v)
inline int lrint_fast(double v) {
  int x;
  __asm__ (
    "vcvtr.s32.f64 %0, %1\n\t"
    :"=t"(x) :"w"(v)
  );
  return x;
}

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
    "rsb %1, %1, #32\n\t"
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

#define FUNCTION_sqrtFixp
inline FIXP_DBL sqrtFixp(const FIXP_DBL op) {
  FIXP_DBL result = lrint(sqrt(op) * sqrt(1UL<<31));
  return result;
}

#define FUNCTION_invSqrtNorm2
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
  UINT mant = ((UINT)(result.i >> 22) & 0x3FFFFFFF) | 0x40000000;
  *result_e = (INT)(result.i >> 52) - 1023 + 16;
  return (FIXP_DBL)mant;
}

#define FUNCTION_invFixp
inline FIXP_DBL invFixp(FIXP_DBL op) {
  if ((op == (FIXP_DBL)0) || (op == (FIXP_DBL)1)) {
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
  if ((op_m == (FIXP_DBL)0) || (op_m == (FIXP_DBL)1)) {
    *op_e = 31 - *op_e;
    return ((LONG)0x7fffffff);
  }
  result.d = 1.0 / op_m;
  INT mant = ((INT)(result.i >> 22) & 0x3FFFFFFF) | 0x40000000;
  *op_e = ((result.i32[1] >> 20) & 0x7FF) - 1023 + 32 - *op_e;
  return (INT)result.i32[1] < 0 ? -mant : mant;
}

#endif // __ARM_ARCH_8__

#endif /* !defined(FIXPOINT_MATH_ARM_H) */
