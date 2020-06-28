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

   Description: SIMD QMF filterbank for ARM

*******************************************************************************/

/* prevent multiple inclusion with re-definitions */
#ifndef __INCLUDE_QMF_ARM__
#define __INCLUDE_QMF_ARM__

#if defined(__ARM_ARCH_7_A__) || defined(__ARM_ARCH_8__)
#if defined(QMF_COEFF_16BIT) && QAS_BITS == 16 && !defined(_MSC_VER)

static void qmfAnaPrototypeFirSlot(
    FIXP_DBL *analysisBuffer,
    INT no_channels, /*!< Number channels of analysis filter */
    const FIXP_PFT *p_filter, INT p_stride, /*!< Stride of analysis filter    */
    FIXP_QAS *RESTRICT pFilterStates) {

  INT pfltStep = QMF_NO_POLY * p_stride;
  INT staStep1 = no_channels * 2 * sizeof(FIXP_QAS);
  FIXP_DBL *RESTRICT pData_1 = analysisBuffer;
  SCHAR *sta_1 = (SCHAR *)(pFilterStates + (2 * QMF_NO_POLY * no_channels) - 4);

#ifdef __ARM_ARCH_8__
  const uint8x16_t index = {12, 13, 14, 15, 8, 9, 10, 11,
                           4,  5,  6,  7,  0, 1, 2,  3};
#define REV32_128(p, v) \
  *(int8x16_t *)p = vqtbl1q_s8((int8x16_t)v, index)
#else
#define REV32_128(p, v) \
  v = vrev64q_s32(v);\
  ((int64_t *)p)[0] = ((int64x2_t)v)[1];\
  ((int64_t *)p)[1] = ((int64x2_t)v)[0]
#endif
  if (p_filter == qmf_pfilt640) {
    // Take advantage of vectorized filter coefficients
    const FIXP_PFT *RESTRICT p_flt = qmf_pfilt640_vector + 640 - 4;
    INT k = no_channels;
    do {
      int16x4_t s_0 = *(int16x4_t *)sta_1,
                s_1 = *(int16x4_t *)(sta_1 - staStep1),
                s_2 = *(int16x4_t *)(sta_1 - staStep1 * 2),
                s_3 = *(int16x4_t *)(sta_1 - staStep1 * 3),
                s_4 = *(int16x4_t *)(sta_1 - staStep1 * 4);

      int16x4_t *f = (int16x4_t *)p_flt;
      int32x4_t accu = vmull_s16(s_0, f[0]);
      accu = vmlal_s16(accu, s_1, f[-1]);
      accu = vmlal_s16(accu, s_2, f[-2]);
      accu = vmlal_s16(accu, s_3, f[-3]);
      accu = vmlal_s16(accu, s_4, f[-4]);
      accu += accu;
      REV32_128(pData_1, accu);

      p_flt -= pfltStep * 4;
      sta_1 -= 4 * sizeof(FIXP_QAS);
      pData_1 += 4;
      k -= 4;
    } while (k);

    SCHAR *sta_0 = (SCHAR *)(pFilterStates + no_channels - 4);
    k = no_channels;
    do {
      int16x4_t s_0 = *(int16x4_t *)sta_0,
                s_1 = *(int16x4_t *)(sta_0 + staStep1),
                s_2 = *(int16x4_t *)(sta_0 + staStep1 * 2),
                s_3 = *(int16x4_t *)(sta_0 + staStep1 * 3),
                s_4 = *(int16x4_t *)(sta_0 + staStep1 * 4);

      int16x4_t *f = (int16x4_t *)p_flt;
      int32x4_t accu = vmull_s16(s_0, f[-4]);
      accu = vmlal_s16(accu, s_1, f[-3]);
      accu = vmlal_s16(accu, s_2, f[-2]);
      accu = vmlal_s16(accu, s_3, f[-1]);
      accu = vmlal_s16(accu, s_4, f[0]);
      accu += accu;
      REV32_128(pData_1, accu);

      p_flt -= pfltStep * 4;
      sta_0 -= 4 * sizeof(FIXP_QAS);
      pData_1 += 4;
      k -= 4;
    } while (k);
    return;
  }

  const FIXP_PFT *RESTRICT p_flt = p_filter;
  INT k = no_channels;
  int16x4x4_t f = {};
  int16x4_t f_4 = {};

  do {
    f = vld4_lane_s16(p_flt + pfltStep * 3, f, 0); f_4[0] = p_flt[pfltStep * 3 + 4];
    f = vld4_lane_s16(p_flt + pfltStep * 2, f, 1); f_4[1] = p_flt[pfltStep * 2 + 4];
    f = vld4_lane_s16(p_flt + pfltStep, f, 2); f_4[2] = p_flt[pfltStep + 4];
    f = vld4_lane_s16(p_flt, f, 3); f_4[3] = p_flt[4];
    p_flt += pfltStep * 4;

    int16x4_t s_0 = *(int16x4_t *)sta_1,
              s_1 = *(int16x4_t *)(sta_1 - staStep1),
              s_2 = *(int16x4_t *)(sta_1 - staStep1 * 2),
              s_3 = *(int16x4_t *)(sta_1 - staStep1 * 3),
              s_4 = *(int16x4_t *)(sta_1 - staStep1 * 4);
    int32x4_t accu = vmull_s16(s_0, f.val[0]);
    accu = vmlal_s16(accu, s_1, f.val[1]);
    accu = vmlal_s16(accu, s_2, f.val[2]);
    accu = vmlal_s16(accu, s_3, f.val[3]);
    accu = vmlal_s16(accu, s_4, f_4);
    accu += accu;
    REV32_128(pData_1, accu);

    sta_1 -= 4 * sizeof(FIXP_QAS);
    pData_1 += 4;
    k -= 4;
  } while (k);

  p_flt -= pfltStep * 3;
  SCHAR *sta_0 = (SCHAR *)(pFilterStates + no_channels - 4);
  k = no_channels;
  do {
    f = vld4_lane_s16(p_flt, f, 0); f_4[0] = p_flt[4];
    f = vld4_lane_s16(p_flt + pfltStep, f, 1); f_4[1] = p_flt[pfltStep + 4];
    f = vld4_lane_s16(p_flt + pfltStep * 2, f, 2); f_4[2] = p_flt[pfltStep * 2 + 4];
    f = vld4_lane_s16(p_flt + pfltStep * 3, f, 3); f_4[3] = p_flt[pfltStep * 3 + 4];
    p_flt -= pfltStep * 4;

    int16x4_t s_0 = *(int16x4_t *)sta_0,
              s_1 = *(int16x4_t *)(sta_0 + staStep1),
              s_2 = *(int16x4_t *)(sta_0 + staStep1 * 2),
              s_3 = *(int16x4_t *)(sta_0 + staStep1 * 3),
              s_4 = *(int16x4_t *)(sta_0 + staStep1 * 4);
    int32x4_t accu = vmull_s16(s_0, f.val[0]);
    accu = vmlal_s16(accu, s_1, f.val[1]);
    accu = vmlal_s16(accu, s_2, f.val[2]);
    accu = vmlal_s16(accu, s_3, f.val[3]);
    accu = vmlal_s16(accu, s_4, f_4);
    accu += accu;
    REV32_128(pData_1, accu);

    sta_0 -= 4 * sizeof(FIXP_QAS);
    pData_1 += 4;
    k -= 4;
  } while (k);
}

#define FUNCTION_qmfAnaPrototypeFirSlot
#endif // defined(QMF_COEFF_16BIT) && QAS_BITS == 16
#endif // defined(__ARM_ARCH_7_A__) || defined(__ARM_ARCH_8__)

#endif /* #ifndef __INCLUDE_QMF_ARM__ */
