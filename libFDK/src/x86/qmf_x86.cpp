/* -----------------------------------------------------------------------------
Software License for The Fraunhofer FDK AAC Codec Library for Android

© Copyright  1995 - 2020 Fraunhofer-Gesellschaft zur Förderung der angewandten
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

   Description: SIMD QMF filterbank for x86

*******************************************************************************/

/* prevent multiple inclusion with re-definitions */
#ifndef __INCLUDE_QMF_X86__
#define __INCLUDE_QMF_X86__

#if defined(QMF_COEFF_16BIT) && QAS_BITS == 16
#ifndef _mm_loadu_si64
#define _mm_loadu_si64(p) _mm_loadl_epi64((__m128i const *)(p))
#endif

#ifdef __SSE4_1__
#define PMOVZXWD(x) _mm_cvtepu16_epi32(*(__m128i *)(x))
#else
#define PMOVZXWD(x) _mm_unpacklo_epi16(_mm_loadu_si64(x), _mm_set1_epi16(0))
#endif

#define ACCU_QMF(s_01, f_01, s_23, f_23, s_4x, f_4x) \
  s_01 = _mm_madd_epi16(s_01, f_01);\
  s_4x = _mm_madd_epi16(s_4x, f_4x);\
  s_23 = _mm_madd_epi16(s_23, f_23);\
  s_01 = _mm_add_epi32(s_01, s_4x);\
  s_01 = _mm_add_epi32(s_01, s_23);\
  s_01 = _mm_add_epi32(s_01, s_01);\
  s_01 = _mm_shuffle_epi32(s_01, 0x1B)

static void qmfAnaPrototypeFirSlot(
    FIXP_DBL *analysisBuffer,
    INT no_channels, /*!< Number channels of analysis filter */
    const FIXP_PFT *p_filter, INT p_stride, /*!< Stride of analysis filter    */
    FIXP_QAS *RESTRICT pFilterStates) {

  INT pfltStep = QMF_NO_POLY * p_stride;
  INT staStep1 = no_channels * 2 * sizeof(FIXP_QAS);
  FIXP_DBL *RESTRICT pData_1 = analysisBuffer;
  SCHAR *sta_1 = (SCHAR *)(pFilterStates + (2 * QMF_NO_POLY * no_channels) - 4);

  if (p_filter == qmf_pfilt640_vector) {
    // Take advantage of vectorized filter coefficients
    const FIXP_PFT *RESTRICT p_flt = qmf_pfilt640_vector + 640 - 4;
#ifdef __SSE4_1__
    const __m128i shuf_mask0 = _mm_set_epi8(15, 14, 7, 6, 13, 12, 5, 4,
                                            11, 10, 3, 2, 9, 8, 1, 0),
                  shuf_mask1 = _mm_set_epi8(7, 6, 15, 14, 5, 4, 13, 12,
                                            3, 2, 11, 10, 1, 0, 9, 8);
#endif
    INT k = no_channels;
    do {
#ifdef __SSE4_1__
      __m128i f_01 = _mm_loadu_si128((__m128i *)(p_flt - 4)),      // 1111 0000
              f_23 = _mm_loadu_si128((__m128i *)(p_flt - 12)),     // 3333 2222
              f_4x = _mm_cvtepu16_epi32(*(__m128i *)(p_flt - 16)); // 4- 4- 4- 4-
      f_01 = _mm_shuffle_epi8(f_01, shuf_mask1); // 01 01 01 01
      f_23 = _mm_shuffle_epi8(f_23, shuf_mask1); // 23 23 23 23
#else
      __m128i f_0 = _mm_loadu_si64(p_flt),
              f_1 = _mm_loadu_si64(p_flt - 4),
              f_2 = _mm_loadu_si64(p_flt - 8),
              f_3 = _mm_loadu_si64(p_flt - 12),
              f_4 = _mm_loadu_si64(p_flt - 16);
      __m128i f_01 = _mm_unpacklo_epi16(f_0, f_1),  // 01 01 01 01
              f_23 = _mm_unpacklo_epi16(f_2, f_3),  // 23 23 23 23
              f_4x = _mm_unpacklo_epi16(f_4, f_4);  // 44 44 44 44
#endif
      p_flt -= pfltStep * 4;
      __m128i s_0 = _mm_loadu_si64(sta_1),
              s_1 = _mm_loadu_si64(sta_1 - staStep1),
              s_2 = _mm_loadu_si64(sta_1 - staStep1 * 2),
              s_3 = _mm_loadu_si64(sta_1 - staStep1 * 3);
      __m128i s_01 = _mm_unpacklo_epi16(s_0, s_1),   // 01 01 01 01
              s_23 = _mm_unpacklo_epi16(s_2, s_3),   // 23 23 23 23
              s_4x = PMOVZXWD(sta_1 - staStep1 * 4); // 4- 4- 4- 4-
      sta_1 -= 4 * sizeof(FIXP_QAS);
      ACCU_QMF(s_01, f_01, s_23, f_23, s_4x, f_4x); // Reverse order
      _mm_storeu_si128((__m128i *)pData_1, s_01);
      pData_1 += 4;
      k -= 4;
    } while (k);

    SCHAR *sta_0 = (SCHAR *)(pFilterStates + no_channels - 4);
    k = no_channels;
    do {
#ifdef __SSE4_1__
      __m128i f_01 = _mm_loadu_si128((__m128i *)(p_flt - 16)), // 0000 1111
              f_23 = _mm_loadu_si128((__m128i *)(p_flt - 8)),  // 2222 3333
              f_4x = _mm_cvtepu16_epi32(*(__m128i *)p_flt);    // 4- 4- 4- 4-
      f_01 = _mm_shuffle_epi8(f_01, shuf_mask0); // 01 01 01 01
      f_23 = _mm_shuffle_epi8(f_23, shuf_mask0); // 23 23 23 23
#else
      __m128i f_0 = _mm_loadu_si64(p_flt - 16),
              f_1 = _mm_loadu_si64(p_flt - 12),
              f_2 = _mm_loadu_si64(p_flt - 8),
              f_3 = _mm_loadu_si64(p_flt - 4),
              f_4 = _mm_loadu_si64(p_flt);
      __m128i f_01 = _mm_unpacklo_epi16(f_0, f_1),  // 01 01 01 01
              f_23 = _mm_unpacklo_epi16(f_2, f_3),  // 23 23 23 23
              f_4x = _mm_unpacklo_epi16(f_4, f_4);  // 44 44 44 44
#endif
      p_flt -= pfltStep * 4;
      __m128i s_0 = _mm_loadu_si64(sta_0),
              s_1 = _mm_loadu_si64(sta_0 + staStep1),
              s_2 = _mm_loadu_si64(sta_0 + staStep1 * 2),
              s_3 = _mm_loadu_si64(sta_0 + staStep1 * 3);
      __m128i s_01 = _mm_unpacklo_epi16(s_0, s_1),  // 01 01 01 01
              s_23 = _mm_unpacklo_epi16(s_2, s_3),  // 23 23 23 23
              s_4x = PMOVZXWD(sta_0 + staStep1 * 4);
      sta_0 -= 4 * sizeof(FIXP_QAS);
      ACCU_QMF(s_01, f_01, s_23, f_23, s_4x, f_4x); // Reverse order
      _mm_storeu_si128((__m128i *)pData_1, s_01);
      pData_1 += 4;
      k -= 4;
    } while (k);
    return;
  }

  const FIXP_PFT *RESTRICT p_flt = p_filter;
  INT k = no_channels;
#ifdef __LP64__ // 64-bit
  // Use single loop to reduce unaligned loads of filter coefficients
  FIXP_DBL *RESTRICT pData_0 = analysisBuffer + 2 * no_channels - 4;
  SCHAR *sta_0 = (SCHAR *)pFilterStates;
  __m128i f_a = _mm_loadu_si128((__m128i *)p_flt);
  p_flt += pfltStep;
  do {
    __m128i f_b = _mm_loadu_si128((__m128i *)(p_flt)),
            f_c = _mm_loadu_si128((__m128i *)(p_flt + pfltStep)),
            f_d = _mm_loadu_si128((__m128i *)(p_flt + pfltStep * 2)),
            f_e = _mm_loadu_si128((__m128i *)(p_flt + pfltStep * 3));
    p_flt += pfltStep * 4;

    __m128i f_dc03 = _mm_unpacklo_epi32(f_d, f_c),  // 01 01 23 23
            f_dc4x = _mm_unpackhi_epi32(f_d, f_c),  // 4x 4x xx xx
            f_ba03 = _mm_unpacklo_epi32(f_b, f_a),  // 01 01 23 23
            f_ba4x = _mm_unpackhi_epi32(f_b, f_a),  // 4x 4x xx xx
            f_bc03 = _mm_unpacklo_epi32(f_b, f_c),  // 01 01 23 23
            f_bc4x = _mm_unpackhi_epi32(f_b, f_c),  // 4x 4x xx xx
            f_de03 = _mm_unpacklo_epi32(f_d, f_e),  // 01 01 23 23
            f_de4x = _mm_unpackhi_epi32(f_d, f_e);  // 4x 4x xx xx
    f_a = f_e;

    __m128i f_01 = _mm_unpacklo_epi64(f_dc03, f_ba03),  // 01 01 01 01
            f_23 = _mm_unpackhi_epi64(f_dc03, f_ba03),  // 23 23 23 23
            f_4x = _mm_unpacklo_epi64(f_dc4x, f_ba4x);  // 4x 4x 4x 4x
    __m128i s_0 = _mm_loadu_si64(sta_1),
            s_1 = _mm_loadu_si64(sta_1 - staStep1),
            s_2 = _mm_loadu_si64(sta_1 - staStep1 * 2),
            s_3 = _mm_loadu_si64(sta_1 - staStep1 * 3);
    __m128i s_01 = _mm_unpacklo_epi16(s_0, s_1),   // 01 01 01 01
            s_23 = _mm_unpacklo_epi16(s_2, s_3),   // 23 23 23 23
            s_4x = PMOVZXWD(sta_1 - staStep1 * 4); // 4- 4- 4- 4-
    sta_1 -= 4 * sizeof(FIXP_QAS);
    ACCU_QMF(s_01, f_01, s_23, f_23, s_4x, f_4x); // Reverse order
    _mm_storeu_si128((__m128i *)pData_1, s_01);
    pData_1 += 4;

    f_01 = _mm_unpacklo_epi64(f_bc03, f_de03); // 01 01 01 01
    f_23 = _mm_unpackhi_epi64(f_bc03, f_de03); // 23 23 23 23
    f_4x = _mm_unpacklo_epi64(f_bc4x, f_de4x); // 4x 4x 4x 4x
    s_0 = _mm_loadu_si64(sta_0);
    s_1 = _mm_loadu_si64(sta_0 + staStep1);
    s_2 = _mm_loadu_si64(sta_0 + staStep1 * 2);
    s_3 = _mm_loadu_si64(sta_0 + staStep1 * 3);
    s_01 = _mm_unpacklo_epi16(s_0, s_1);   // 01 01 01 01
    s_23 = _mm_unpacklo_epi16(s_2, s_3);   // 23 23 23 23
    s_4x = PMOVZXWD(sta_0 + staStep1 * 4); // 4- 4- 4- 4-
    sta_0 += 4 * sizeof(FIXP_QAS);
    ACCU_QMF(s_01, f_01, s_23, f_23, s_4x, f_4x); // Reverse order
    _mm_storeu_si128((__m128i *)pData_0, s_01);
    pData_0 -= 4;
    k -= 4;
  } while (k);
#else // 32-bit
  // Split loop into two since there are only 8 SSE registers on 32-bit
  do {
    __m128i f_a = _mm_loadu_si128((__m128i *)(p_flt + pfltStep * 3)),
            f_b = _mm_loadu_si128((__m128i *)(p_flt + pfltStep * 2)),
            f_c = _mm_loadu_si128((__m128i *)(p_flt + pfltStep)),
            f_d = _mm_loadu_si128((__m128i *)p_flt);    // 01234xxx
    __m128i f_ab03 = _mm_unpacklo_epi32(f_a, f_b),      // 01 01 23 23
            f_ab4x = _mm_unpackhi_epi32(f_a, f_b),      // 4x 4x xx xx
            f_cd03 = _mm_unpacklo_epi32(f_c, f_d),      // 01 01 23 23
            f_cd4x = _mm_unpackhi_epi32(f_c, f_d);      // 4x 4x xx xx
    __m128i f_01 = _mm_unpacklo_epi64(f_ab03, f_cd03),  // 01 01 01 01
            f_23 = _mm_unpackhi_epi64(f_ab03, f_cd03),  // 23 23 23 23
            f_4x = _mm_unpacklo_epi64(f_ab4x, f_cd4x);  // 4x 4x 4x 4x
    p_flt += pfltStep * 4;

    __m128i s_0 = _mm_loadu_si64(sta_1),
            s_1 = _mm_loadu_si64(sta_1 - staStep1),
            s_2 = _mm_loadu_si64(sta_1 - staStep1 * 2),
            s_3 = _mm_loadu_si64(sta_1 - staStep1 * 3);
    __m128i s_01 = _mm_unpacklo_epi16(s_0, s_1), // 01 01 01 01
            s_23 = _mm_unpacklo_epi16(s_2, s_3), // 23 23 23 23
            s_4x = PMOVZXWD(sta_1 - staStep1 * 4);
    sta_1 -= 4 * sizeof(FIXP_QAS);
    ACCU_QMF(s_01, f_01, s_23, f_23, s_4x, f_4x); // Reverse order
    _mm_storeu_si128((__m128i *)pData_1, s_01);
    pData_1 += 4;
    k -= 4;
  } while (k);

  k = no_channels;
  
  p_flt -= pfltStep * 3;
  SCHAR *sta_0 = (SCHAR *)(pFilterStates + no_channels - 4);
  do {
    __m128i f_a = _mm_loadu_si128((__m128i *)p_flt),
            f_b = _mm_loadu_si128((__m128i *)(p_flt + pfltStep)),
            f_c = _mm_loadu_si128((__m128i *)(p_flt + pfltStep * 2)),
            f_d = _mm_loadu_si128((__m128i *)(p_flt + pfltStep * 3));
    __m128i f_ab03 = _mm_unpacklo_epi32(f_a, f_b),      // 01 01 23 23
            f_ab4x = _mm_unpackhi_epi32(f_a, f_b),      // 4x 4x xx xx
            f_cd03 = _mm_unpacklo_epi32(f_c, f_d),      // 01 01 23 23
            f_cd4x = _mm_unpackhi_epi32(f_c, f_d);      // 4x 4x xx xx
    __m128i f_01 = _mm_unpacklo_epi64(f_ab03, f_cd03),  // 01 01 01 01
            f_23 = _mm_unpackhi_epi64(f_ab03, f_cd03),  // 23 23 23 23
            f_4x = _mm_unpacklo_epi64(f_ab4x, f_cd4x);  // 4x 4x 4x 4x
    p_flt -= pfltStep * 4;

    __m128i s_0 = _mm_loadu_si64(sta_0),
            s_1 = _mm_loadu_si64(sta_0 + staStep1),
            s_2 = _mm_loadu_si64(sta_0 + staStep1 * 2),
            s_3 = _mm_loadu_si64(sta_0 + staStep1 * 3);
    __m128i s_01 = _mm_unpacklo_epi16(s_0, s_1),  // 01 01 01 01
            s_23 = _mm_unpacklo_epi16(s_2, s_3),  // 23 23 23 23
            s_4x = PMOVZXWD(sta_0 + staStep1 * 4);
    sta_0 -= 4 * sizeof(FIXP_QAS);
    ACCU_QMF(s_01, f_01, s_23, f_23, s_4x, f_4x); // Reverse order
    _mm_storeu_si128((__m128i *)pData_1, s_01);
    pData_1 += 4;
    k -= 4;
  } while (k);
#endif // __LP64__
}
#define FUNCTION_qmfAnaPrototypeFirSlot
#define qmf_pfilt640 qmf_pfilt640_vector
#endif // defined(QMF_COEFF_16BIT) && QAS_BITS == 16

#endif /* #ifndef __INCLUDE_QMF_X86__ */
