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

   Author(s):   Arthur Tritthart

   Description: Scaling operations for ARM

*******************************************************************************/

/* prevent multiple inclusion with re-definitions */
#ifndef __INCLUDE_SCALE_ARM__
#define __INCLUDE_SCALE_ARM__

#if (defined(__ARM_ARCH_8__) || defined(__ARM_ARCH_7_A__)) && !defined(_MSC_VER)
#define FUNCTION_scaleValuesSaturate_DBL
void scaleValuesSaturate(FIXP_DBL *vector, INT len, INT scalefactor) {
  /* Return if scalefactor is Zero */
  if (scalefactor == 0) return;

  int32x4_t s = vdupq_n_s32(scalefactor);
  int32x4_t *v = (int32x4_t *)vector;
  for (int i = len >> 2; i; i--) {
    *v = vqrshlq_s32(*v, s);
    v++;
  }
  int32x4_t x = *v;
  switch (len & 3) {
  case 3:
    x = vqrshlq_s32(x, s);
    ((int32x2_t*)v)[0] = vget_low_s32(x);
    v[0][2] = x[2];
    break;
  case 2:
    ((int32x2_t*)v)[0] = vqrshl_s32(vget_low_s32(x), vget_low_s32(s));
    break;
  case 1:
#ifdef __ARM_ARCH_7_A__
    v[0][0] = vqrshl_s32(vget_low_s32(x), vget_low_s32(s))[0];
#else
    v[0][0] = vqrshls_s32(x[0], s[0]);
#endif
  }
}

#define FUNCTION_scaleValuesWithFactor_DBL
SCALE_INLINE
void scaleValuesWithFactor(FIXP_DBL *vector, FIXP_DBL factor, INT len, INT scalefactor) {
#ifdef __ARM_ARCH_7_A__
  int64x2_t s = (int64x2_t)vdupq_n_s32(scalefactor) >> 32;
#else
  int64x2_t s = vdupq_n_s64(scalefactor);
#endif
  int32x2_t f = {factor, 0};
  int32x2_t *v = (int32x2_t *)vector;
  for (int i = len >> 2; i; i--) {
#ifdef __ARM_ARCH_7_A__
    int64x2_t y0 = vmull_lane_s32(v[0], f, 0),
              y1 = vmull_lane_s32(v[1], f, 0);
#else
    int32x4_t x = *(int32x4_t*)v;
    int64x2_t y0 = vmull_lane_s32(vget_low_s32(x), f, 0),
              y1 = vmull_high_lane_s32(x, f, 0);
#endif
    y0 = vqrshlq_s64(y0, s);
    y1 = vqrshlq_s64(y1, s);
    v[0] = vqrshrn_n_s64(y0, 32);
    v[1] = vqrshrn_n_s64(y1, 32);
    v += 2;
  }
  int64x2_t y;
  switch (len & 3) {
  case 3:
    y = vmull_lane_s32(v[1], f, 0);
    y = vcombine_s64(vqrshl_s64(vget_low_s64(y), vget_low_s64(s)),
                     vget_high_s64(y));
    v[1][0] = vqrshrn_n_s64(y, 32)[0];
  case 2:
    y = vmull_lane_s32(v[0], f, 0);
    y = vqrshlq_s64(y, s);
    v[0] = vqrshrn_n_s64(y, 32);
    break;
  case 1:
    y = vmull_lane_s32(v[0], f, 0);
    y = vcombine_s64(vqrshl_s64(vget_low_s64(y), vget_low_s64(s)),
                     vget_high_s64(y));
    v[0][0] = vqrshrn_n_s64(y, 32)[0];
  }
}

#define FUNCTION_getScalefactorCplx_DBL
SCALE_INLINE
INT getScalefactorCplx(const FIXP_DBL *vectorRe, /*!< Pointer to real vector */
                       const FIXP_DBL *vectorIm, /*!< Pointer to image vector */
                       INT len) {                /*!< Length of input vector */
  int32x4_t maxValRe = {32, 32, 32, 32},
            maxValIm = {32, 32, 32, 32};
  do {
    int32x4_t tr = vclsq_s32(*(int32x4_t *)vectorRe),
              ti = vclsq_s32(*(int32x4_t *)vectorIm);
    vectorRe += 4;
    vectorIm += 4;
    maxValRe = vminq_s32(maxValRe, tr);
    maxValIm = vminq_s32(maxValIm, ti);
    len -= 4;
  } while (len > 0);
  maxValRe = vminq_s32(maxValRe, maxValIm);
#ifdef __ARM_ARCH_8__
  return vminvq_s32(maxValRe);
#else
  int32x2_t maxVal = vpmin_s32(vget_low_s32(maxValRe), vget_high_s32(maxValRe));
  maxVal = vpmin_s32(maxVal, maxVal);
  return maxVal[0];
#endif
}

#else
#define FUNCTION_scaleValuesWithFactor_DBL
SCALE_INLINE
void scaleValuesWithFactor(FIXP_DBL *vector, FIXP_DBL factor, INT len,
                           INT scalefactor) {
  /* This code combines the fMult with the scaling             */
  /* It performs a fMultDiv2 and increments shift by 1         */
  int shift = scalefactor + 1;
  FIXP_DBL *mySpec = vector;

  shift = fixmin_I(shift, (INT)DFRACT_BITS - 1);

  if (shift >= 0) {
    for (int i = 0; i < (len >> 2); i++) {
      FIXP_DBL tmp0 = mySpec[0];
      FIXP_DBL tmp1 = mySpec[1];
      FIXP_DBL tmp2 = mySpec[2];
      FIXP_DBL tmp3 = mySpec[3];
      tmp0 = fMultDiv2(tmp0, factor);
      tmp1 = fMultDiv2(tmp1, factor);
      tmp2 = fMultDiv2(tmp2, factor);
      tmp3 = fMultDiv2(tmp3, factor);
      tmp0 <<= shift;
      tmp1 <<= shift;
      tmp2 <<= shift;
      tmp3 <<= shift;
      *mySpec++ = tmp0;
      *mySpec++ = tmp1;
      *mySpec++ = tmp2;
      *mySpec++ = tmp3;
    }
    for (int i = len & 3; i--;) {
      FIXP_DBL tmp0 = mySpec[0];
      tmp0 = fMultDiv2(tmp0, factor);
      tmp0 <<= shift;
      *mySpec++ = tmp0;
    }
  } else {
    shift = -shift;
    for (int i = 0; i < (len >> 2); i++) {
      FIXP_DBL tmp0 = mySpec[0];
      FIXP_DBL tmp1 = mySpec[1];
      FIXP_DBL tmp2 = mySpec[2];
      FIXP_DBL tmp3 = mySpec[3];
      tmp0 = fMultDiv2(tmp0, factor);
      tmp1 = fMultDiv2(tmp1, factor);
      tmp2 = fMultDiv2(tmp2, factor);
      tmp3 = fMultDiv2(tmp3, factor);
      tmp0 >>= shift;
      tmp1 >>= shift;
      tmp2 >>= shift;
      tmp3 >>= shift;
      *mySpec++ = tmp0;
      *mySpec++ = tmp1;
      *mySpec++ = tmp2;
      *mySpec++ = tmp3;
    }
    for (int i = len & 3; i--;) {
      FIXP_DBL tmp0 = mySpec[0];
      tmp0 = fMultDiv2(tmp0, factor);
      tmp0 >>= shift;
      *mySpec++ = tmp0;
    }
  }
}

#endif // arch selection

#endif /* #ifndef __INCLUDE_SCALE_ARM__ */
