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

   Author(s):   Zongyi Zhou

   Description: Scaling operations for x86

*******************************************************************************/

/* prevent multiple inclusion with re-definitions */
#ifndef __INCLUDE_SCALE_X86__
#define __INCLUDE_SCALE_X86__

#ifdef __GNUC__
inline __m128d CVTDQ2PD(const void *x) {
  const __m64 *p = (const __m64 *)x;
  __m128d r;
#ifdef __AVX__
  __asm__("vcvtdq2pd %1, %0" : "=x"(r) : "m"(*p));
#else
  __asm__("cvtdq2pd %1, %0" : "=x"(r) : "m"(*p));
#endif
  return r;
}

inline __m128i PMOVSXDQ(const void *x) {
  const __m64 *p = (const __m64 *)x;
  __m128i r;
#ifdef __AVX__
  __asm__("vpmovsxdq %1, %0" : "=x"(r) : "m"(*p));
#else
  __asm__("pmovsxdq %1, %0" : "=x"(r) : "m"(*p));
#endif
  return r;
}
#else
#define CVTDQ2PD(x) _mm_cvtepi32_pd(*(__m128i *)(x))
#define PMOVSXDQ(x) _mm_cvtepi32_epi64(*(__m128i *)(x))
#endif

#define FUNCTION_scaleValuesSaturate_DBL
void scaleValuesSaturate(FIXP_DBL *vector, INT len, INT scalefactor) {
  /* Return if scalefactor is Zero */
  if (scalefactor == 0) return;

  if (scalefactor > 0) {
    __m128i exp = _mm_cvtsi32_si128(scalefactor);
    __m128d maxint = _mm_set1_pd((double)FDK_INT_MAX);
    exp = _mm_shuffle_epi32(exp, 0x11);
    exp = _mm_slli_epi32(exp, 20);
    __m128d d;
    __m128i i64;
    for (INT i = len >> 1; i; i--) {
      d = CVTDQ2PD(vector);
      d = _mm_castsi128_pd(_mm_add_epi32(_mm_castpd_si128(d), exp));
      d = _mm_min_pd(d, maxint);
      i64 = _mm_cvtpd_epi32(d);
      _mm_storel_pi((__m64 *)vector, _mm_castsi128_ps(i64));
      vector += 2;
    }
    if (len & 1) {
      INT v = *vector;
      d = _mm_cvtsi32_sd(d, v);
      d = _mm_castsi128_pd(_mm_add_epi32(_mm_castpd_si128(d), exp));
      d = _mm_min_sd(d, maxint);
      *vector = _mm_cvtsd_si32(_mm_castsi128_pd(i64));
    }
  } else {
    for (INT i = 0; i < len; i++) {
      vector[i] = vector[i] >> -scalefactor;
    }
  }
}

#define FUNCTION_scaleValuesWithFactor_DBL
void scaleValuesWithFactor(FIXP_DBL *vector, FIXP_DBL factor, INT len,
                           INT scalefactor) {
  /* This code combines the fMult with the scaling             */
  /* It performs a fMultDiv2 and increments shift by 1         */
#ifdef __SSE4_1__
  if (scalefactor >= 0) {
#endif
    // Use fp for multiply and saturation
    __m128i sh = _mm_cvtsi32_si128(scalefactor - 31);
    __m128d fd = _mm_cvtsi32_sd(_mm_setzero_pd(), factor);
    __m128d maxint = _mm_set1_pd((double)0x7fffffff);
    sh = _mm_slli_epi64(sh, 52);
    fd = _mm_castsi128_pd(_mm_add_epi32(_mm_castpd_si128(fd), sh));
    __m128d d;
    for (int i = len >> 1; i; i--) {
      d = CVTDQ2PD(vector);
      d = _mm_mul_pd(d, fd);
      d = _mm_min_pd(d, maxint);
      __m128i v = _mm_cvtpd_epi32(d);
      _mm_storel_pi((__m64 *)vector, _mm_castsi128_ps(v));
      vector += 2;
    }
    if (len & 1) {
      d = _mm_cvtsi32_sd(d, *vector);
      d = _mm_mul_sd(d, fd);
      d = _mm_min_pd(d, maxint);
      *vector = _mm_cvtsd_si32(d);
    }
#ifdef __SSE4_1__
  } else {
    __m128i fi = _mm_set1_epi32(factor);
    __m128i shift = _mm_cvtsi32_si128(-1 - scalefactor);
    shift = _mm_unpacklo_epi64(shift, shift);
    for (int i = len >> 1; i; i--) {
      __m128i v = PMOVSXDQ(vector);
      v = _mm_mul_epi32(v, fi);
      v = _mm_sra_epi32(v, shift);
      vector[0] = _mm_extract_epi32(v, 1);
      vector[1] = _mm_extract_epi32(v, 3);
      vector += 2;
    }
    if (len & 1) {
#ifdef __LP64__
      *vector = ((INT64)*vector * factor) >> (31 - scalefactor);
#else
      *vector = fMult(*vector, factor) >> -scalefactor;
#endif
    }
  }
#endif  // __SSE4_1__
}

#endif /* #ifndef __INCLUDE_SCALE_X86__ */
