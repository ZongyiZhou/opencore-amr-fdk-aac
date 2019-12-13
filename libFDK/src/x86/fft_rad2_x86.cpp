/* -----------------------------------------------------------------------------
Software License for The Fraunhofer FDK AAC Codec Library for Android

© Copyright 2019 Fraunhofer-Gesellschaft zur Förderung der angewandten
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

   Author(s):

   Description: dit_fft x86 intrinsic replacements.

*******************************************************************************/

#ifndef __FFT_RAD2_CPP__
#error \
    "Do not compile this file separately. It is included on demand from fft_rad2.cpp"
#endif

#ifndef FUNCTION_dit_fft
#define FUNCTION_dit_fft

void dit_fft_impl(FIXP_DBL *x, const INT ldn, const FIXP_STP *trigdata,
                  const INT trigDataSize) {
  const UINT n = 1 << ldn;
  INT trigstep, ldm;
  UINT i, mh;

  /*
   * 1+2 stage radix 4
   */

  for (i = 0; i < n * 2; i += 8) {
    __m128i x0123 = _mm_castps_si128(_mm_loadu_ps((float *)(x + i))),
            x4567 = _mm_castps_si128(_mm_loadu_ps((float *)(x + i + 4)));
    __m128i x0415 = _mm_unpacklo_epi32(x0123, x4567),
            x2637 = _mm_unpackhi_epi32(x0123, x4567);
    __m128i a0123 = _mm_srai_epi32(_mm_add_epi32(x0415, x2637), 1);
    __m128i b0123 = _mm_sub_epi32(a0123, x2637),
            a0213 = _mm_shuffle_epi32(a0123, 0xd8);
    __m128i b0231 = _mm_shuffle_epi32(b0123, 0x78);
    __m128i a02b02 = _mm_unpacklo_epi64(a0213, b0231),
            a13b31 = _mm_unpackhi_epi64(a0213, b0231);
    __m128i x0127 = _mm_add_epi32(a02b02, a13b31),
            x4563 = _mm_sub_epi32(a02b02, a13b31);
#ifdef __SSE4_1__
    x0123 = _mm_blend_epi16(x0127, x4563, 0xC0);
    x4567 = _mm_blend_epi16(x4563, x0127, 0xC0);
    _mm_storeu_ps((float *)(x + i), _mm_castsi128_ps(x0123));
    _mm_storeu_ps((float *)(x + i + 4), _mm_castsi128_ps(x4567));
#else
    _mm_storel_pi((__m64 *)(x + i), _mm_castsi128_ps(x0127));
    _mm_storel_pi((__m64 *)(x + i + 4), _mm_castsi128_ps(x4563));
    __m128 r = _mm_castsi128_ps(_mm_unpackhi_epi32(x0127, x4563));  // 2673
    r = _mm_shuffle_ps(r, r, 0x9c);                                 // 2367
    _mm_storel_pi((__m64 *)(x + i + 2), r);
    _mm_storeh_pi((__m64 *)(x + i + 6), r);
#endif
  }

  mh = (1 << 2);
  for (ldm = 3; ldm <= ldn; ++ldm) {
    const UINT m = mh * 2;
    UINT j, r;

    trigstep = ((trigDataSize << 2) >> ldm);

    FDK_ASSERT(trigstep > 0);

    /* Do first iteration with c=1.0 and s=0.0 separately to avoid loosing to
       much precision. Beware: The impact on the overal FFT precision is
       rather large. */
    { /* block 1 */
      for (r = 0; r < n; r += m) {
        INT t1 = r * 2;
        INT t2 = t1 + m;
        __m128i u = _mm_loadl_epi64((__m128i *)(x + t1)),
                v = _mm_loadl_epi64((__m128i *)(x + t2));
        u = _mm_castps_si128(
            _mm_loadh_pi(_mm_castsi128_ps(u), (__m64 *)(x + t1 + mh)));
        v = _mm_castps_si128(
            _mm_loadh_pi(_mm_castsi128_ps(v), (__m64 *)(x + t2 + mh)));
        u = _mm_srai_epi32(u, 1);
        v = _mm_srai_epi32(v, 1);
        v = _mm_shufflehi_epi16(v, 0x4e);
        __m128 r1 = _mm_castsi128_ps(_mm_add_epi32(u, v)),
               r2 = _mm_castsi128_ps(_mm_sub_epi32(u, v));
        _mm_storel_pi((__m64 *)(x + t1), r1);
        _mm_storel_pi((__m64 *)(x + t2), r2);
        r1 = _mm_unpackhi_ps(r1, r2);
        r1 = _mm_shuffle_ps(r1, r1, 0x9c);
        _mm_storel_pi((__m64 *)(x + t1 + mh), r1);
        _mm_storeh_pi((__m64 *)(x + t2 + mh), r1);
      }
    } /* end of  block 1 */

#ifdef __SSE4_1__  // SSE4.1 required for block 2
    for (j = 1; j < mh / 4; ++j) {
      const FIXP_STP *cs = trigdata + j * trigstep;
      // load coef
#ifdef SINETABLE_16BIT
      __m128i cs0 =
          _mm_cvtepu16_epi32(_mm_cvtsi32_si128(cs->w));  // [Cr 0 Ci 0 0 0 0 0]
      __m128i cs1 = _mm_shuffle_epi32(cs0, 0xdf);        // [0 0 0 0 Ci 0 0 0]
      cs0 = _mm_subs_epi16(cs0, cs1);      // [Cr 0 Ci 0 -Ci 0 0 0]
      cs0 = _mm_slli_epi32(cs0, 16);       // [0 Cr 0 Ci 0 -Ci 0 0]
#else
      __m128i cs0 = _mm_cvtsi64_si128(cs->w);     // [Cr Ci 0 0]
      __m128i cs1 = _mm_shuffle_epi32(cs0, 0xdf); // [0 0 Ci 0]
      cs0 = _mm_subs_epi16(cs0, cs1);      // [Cr Ci -Ci 0]
#endif
      cs1 = _mm_shuffle_epi32(cs0, 0xec);  // [Cr<<16 0 -Ci<<16 0]
      cs0 = _mm_shuffle_epi32(cs0, 0xcd);  // [Ci<<16 0  Cr<<16 0]
      for (r = 0; r < n; r += m) {
        INT t1 = (r + j) * 2;
        INT t2 = t1 + m;

        // vi = x1Cr - x0Ci, vr = x0Cr + x1Ci
        __m128i v0 = _mm_loadl_epi64((__m128i *)(x + t2));  // [x0 x1 0 0]
        __m128i v1 = _mm_shuffle_epi32(v0, 0x99);           // [x1 0 x1 0]
        v0 = _mm_shuffle_epi32(v0, 0x88);                   // [x0 0 x0 0]
        v1 = _mm_mul_epi32(v1, cs0);                        // [x1Ci x1Cr]
        v0 = _mm_mul_epi32(v0, cs1);                        // [x0Cr -x0Ci]
        v0 = _mm_add_epi64(v0, v1);                         // [vr<<32 vi<<32]
        v1 = _mm_srli_epi64(v0, 32);                        // [vr 0 vi 0]
        v0 = _mm_shuffle_epi32(v1, 0x58);                   // [vr vi 0 0]
        v1 = _mm_shuffle_epi32(v1, 0x85);                   // [0 0 vr vi]

        __m128i u = _mm_castpd_si128(_mm_movedup_pd(
            *(__m128d *)(x + t1)));  // [x[t1] x[t1+1] x[t1] x[t1+1]]
        u = _mm_srai_epi32(u, 1);    // [ur ui ur ui]
        v0 = _mm_sub_epi32(_mm_add_epi32(u, v0),
                           v1);  // [ur+vr ui+vi ur-vr ui-vi]
        _mm_storel_pi((__m64 *)(x + t1), _mm_castsi128_ps(v0));
        _mm_storeh_pi((__m64 *)(x + t2), _mm_castsi128_ps(v0));

        t1 += mh;
        t2 += mh;

        // vr = x1Cr - x0Ci, vi = x0Cr + x1Ci
        v0 = _mm_loadl_epi64((__m128i *)(x + t2));  // [x0 x1 0 0]
        v1 = _mm_shuffle_epi32(v0, 0x99);           // [x1 0 x1 0]
        v0 = _mm_shuffle_epi32(v0, 0x88);           // [x0 0 x0 0]
        v1 = _mm_mul_epi32(v1, cs0);                // [x1Ci x1Cr]
        v0 = _mm_mul_epi32(v0, cs1);                // [x0Cr -x0Ci]
        v0 = _mm_add_epi64(v0, v1);                 // [vi<<32 vr<<32]
        v1 = _mm_srli_epi64(v0, 32);                // [vi 0 vr 0]
        v0 = _mm_shuffle_epi32(v1, 0x1e);           // [vr 0 0 vi]
        v1 = _mm_shuffle_epi32(v1, 0xe1);           // [0 vi vr 0]

        u = _mm_castpd_si128(_mm_movedup_pd(
            *(__m128d *)(x + t1)));  // [x[t1] x[t1+1] x[t1] x[t1+1]]
        u = _mm_srai_epi32(u, 1);    // [ur ui ur ui]
        v0 = _mm_sub_epi32(_mm_add_epi32(u, v0),
                           v1);  // [ur+vr ui-vi ur-vr ui+vi]
        _mm_storel_pi((__m64 *)(x + t1), _mm_castsi128_ps(v0));
        _mm_storeh_pi((__m64 *)(x + t2), _mm_castsi128_ps(v0));

        /* Same as above but for t1,t2 with j>mh/4 and thus cs swapped */
        t1 -= j * 4;
        t2 -= j * 4;

        // vi = x0Cr - x1Ci, vr = x1Cr + x0Ci
        v0 = _mm_loadl_epi64((__m128i *)(x + t2));  // [x0 x1 0 0]
        v1 = _mm_shuffle_epi32(v0, 0x88);           // [x0 0 x0 0]
        v0 = _mm_shuffle_epi32(v0, 0x99);           // [x1 0 x1 0]
        v1 = _mm_mul_epi32(v1, cs0);                // [x0Ci x0Cr]
        v0 = _mm_mul_epi32(v0, cs1);                // [x1Cr -x1Ci]
        v0 = _mm_add_epi64(v0, v1);                 // [vr<<32 vi<<32]
        v1 = _mm_srli_epi64(v0, 32);                // [vr 0 vi 0]
        v0 = _mm_shuffle_epi32(v1, 0x94);           // [vr 0 0 vi]
        v1 = _mm_shuffle_epi32(v1, 0x49);           // [0 vi vr 0]

        u = _mm_castpd_si128(_mm_movedup_pd(
            *(__m128d *)(x + t1)));  // [x[t1] x[t1+1] x[t1] x[t1+1]]
        u = _mm_srai_epi32(u, 1);    // [ur ui ur ui]
        v0 = _mm_sub_epi32(_mm_add_epi32(u, v0),
                           v1);  // [ur+vr ui-vi ur-vr ui+vi]
        _mm_storel_pi((__m64 *)(x + t1), _mm_castsi128_ps(v0));
        _mm_storeh_pi((__m64 *)(x + t2), _mm_castsi128_ps(v0));

        t1 += mh;
        t2 += mh;

        // vr = x0Cr - x1Ci, vi = x1Cr + x0Ci
        v0 = _mm_loadl_epi64((__m128i *)(x + t2));  // [x0 x1 0 0]
        v1 = _mm_shuffle_epi32(v0, 0x88);           // [x0 0 x0 0]
        v0 = _mm_shuffle_epi32(v0, 0x99);           // [x1 0 x1 0]
        v1 = _mm_mul_epi32(v1, cs0);                // [x0Ci x0Cr]
        v0 = _mm_mul_epi32(v0, cs1);                // [x1Cr -x1Ci]
        v0 = _mm_add_epi64(v0, v1);                 // [vi<<32 vr<<32]
        v1 = _mm_srli_epi64(v0, 32);                // [vi 0 vr 0]
        v0 = _mm_shuffle_epi32(v1, 0x25);           // [0 0 vr vi]
        v1 = _mm_shuffle_epi32(v1, 0x52);           // [vr vi 0 0]

        u = _mm_castpd_si128(_mm_movedup_pd(
            *(__m128d *)(x + t1)));  // [x[t1] x[t1+1] x[t1] x[t1+1]]
        u = _mm_srai_epi32(u, 1);    // [ur ui ur ui]
        v0 = _mm_sub_epi32(_mm_add_epi32(u, v0),
                           v1);  // [ur-vr ui-vi ur+vr ui+vi]
        _mm_storel_pi((__m64 *)(x + t1), _mm_castsi128_ps(v0));
        _mm_storeh_pi((__m64 *)(x + t2), _mm_castsi128_ps(v0));
      }
    }

    { /* block 2 */
      // load coef
      __m128i cs = _mm_set_epi32(0, -0x5a82799a, 0, 0x5a82799a);

      for (r = 0; r < n; r += m) {
        INT t1 = r * 2 + mh / 2;
        INT t2 = t1 + m;

        // vi = -Cr(x0-x1), vr = Cr(x0 + x1)
        __m128i v0 = _mm_castpd_si128(
            _mm_movedup_pd(*(__m128d *)(x + t2)));     // [x0 x1 x0 x1]
        __m128i v1 = _mm_srli_si128(v0, 12);           // [x1 0 0 0]
        __m128i u = _mm_shuffle_epi32(v1, 1);          // [0 x1 x1 x1]
        v0 = _mm_sub_epi32(_mm_add_epi32(v0, v1), u);  // [x0+x1 0 x0-x1 0]
        v0 = _mm_mul_epi32(v0, cs);                    // [vr<<32 vi<<32]
        v1 = _mm_srli_epi64(v0, 32);                   // [vr 0 vi 0]
        v0 = _mm_shuffle_epi32(v1, 0x58);              // [vr vi 0 0]
        v1 = _mm_shuffle_epi32(v1, 0x85);              // [0 0 vr vi]

        u = _mm_castpd_si128(_mm_movedup_pd(
            *(__m128d *)(x + t1)));  // [x[t1] x[t1+1] x[t1] x[t1+1]]
        u = _mm_srai_epi32(u, 1);    // [ur ui ur ui]
        v0 = _mm_sub_epi32(_mm_add_epi32(u, v0),
                           v1);  // [ur+vr ui+vi ur-vr ui-vi]
        _mm_storel_pi((__m64 *)(x + t1), _mm_castsi128_ps(v0));
        _mm_storeh_pi((__m64 *)(x + t2), _mm_castsi128_ps(v0));

        t1 += mh;
        t2 += mh;

        // vr = -Cr(x0-x1), vi = Cr(x0 + x1)
        v0 = _mm_castpd_si128(
            _mm_movedup_pd(*(__m128d *)(x + t2)));     // [x0 x1 x0 x1]
        v1 = _mm_srli_si128(v0, 12);                   // [x1 0 0 0]
        u = _mm_shuffle_epi32(v1, 1);                  // [0 x1 x1 x1]
        v0 = _mm_sub_epi32(_mm_add_epi32(v0, v1), u);  // [x0+x1 0 x0-x1 0]
        v0 = _mm_mul_epi32(v0, cs);                    // [vi<<32 vr<<32]
        v1 = _mm_srli_epi64(v0, 32);                   // [vi 0 vr 0]
        v0 = _mm_shuffle_epi32(v1, 0x1e);              // [vr 0 0 vi]
        v1 = _mm_shuffle_epi32(v1, 0xe1);              // [0 vi vr 0]

        u = _mm_castpd_si128(_mm_movedup_pd(
            *(__m128d *)(x + t1)));  // [x[t1] x[t1+1] x[t1] x[t1+1]]
        u = _mm_srai_epi32(u, 1);    // [ur ui ur ui]
        v0 = _mm_sub_epi32(_mm_add_epi32(u, v0),
                           v1);  // [ur+vr ui-vi ur-vr ui+vi]
        _mm_storel_pi((__m64 *)(x + t1), _mm_castsi128_ps(v0));
        _mm_storeh_pi((__m64 *)(x + t2), _mm_castsi128_ps(v0));
      }
    } /* end of block 2 */
#else
      for (j = 1; j < mh / 4; ++j) {
        FIXP_STP cs;

        cs = trigdata[j * trigstep];

        for (r = 0; r < n; r += m) {
          INT t1 = (r + j) << 1;
          INT t2 = t1 + m;
          FIXP_DBL vr, vi, ur, ui;

          cplxMultDiv2(&vi, &vr, x[t2 + 1], x[t2], cs);

          ur = x[t1] >> 1;
          ui = x[t1 + 1] >> 1;

          x[t1] = ur + vr;
          x[t1 + 1] = ui + vi;

          x[t2] = ur - vr;
          x[t2 + 1] = ui - vi;

          t1 += mh;
          t2 += mh;

          cplxMultDiv2(&vr, &vi, x[t2 + 1], x[t2], cs);

          ur = x[t1] >> 1;
          ui = x[t1 + 1] >> 1;

          x[t1] = ur + vr;
          x[t1 + 1] = ui - vi;

          x[t2] = ur - vr;
          x[t2 + 1] = ui + vi;

          /* Same as above but for t1,t2 with j>mh/4 and thus cs swapped */
          t1 -= j * 4;
          t2 -= j * 4;

          cplxMultDiv2(&vi, &vr, x[t2], x[t2 + 1], cs);

          ur = x[t1] >> 1;
          ui = x[t1 + 1] >> 1;

          x[t1] = ur + vr;
          x[t1 + 1] = ui - vi;

          x[t2] = ur - vr;
          x[t2 + 1] = ui + vi;

          t1 += mh;
          t2 += mh;

          cplxMultDiv2(&vr, &vi, x[t2], x[t2 + 1], cs);

          ur = x[t1] >> 1;
          ui = x[t1 + 1] >> 1;

          x[t1] = ur - vr;
          x[t1 + 1] = ui - vi;

          x[t2] = ur + vr;
          x[t2 + 1] = ui + vi;
        }
      }

      { /* block 2 */
        for (r = 0; r < n; r += m) {
          INT t1 = r * 2 + mh / 2;
          INT t2 = t1 + m;
          FIXP_DBL vr, vi, ur, ui;

          cplxMultDiv2(&vi, &vr, x[t2 + 1], x[t2], (FIXP_DBL)0x5a82799a,
                       (FIXP_DBL)0x5a82799a);

          ur = x[t1] >> 1;
          ui = x[t1 + 1] >> 1;

          x[t1] = ur + vr;
          x[t1 + 1] = ui + vi;

          x[t2] = ur - vr;
          x[t2 + 1] = ui - vi;

          t1 += mh;
          t2 += mh;

          cplxMultDiv2(&vr, &vi, x[t2 + 1], x[t2], (FIXP_DBL)0x5a82799a,
                       (FIXP_DBL)0x5a82799a);

          ur = x[t1] >> 1;
          ui = x[t1 + 1] >> 1;

          x[t1] = ur + vr;
          x[t1 + 1] = ui - vi;

          x[t2] = ur - vr;
          x[t2 + 1] = ui + vi;
        }
      } /* end of block 2 */
#endif
    mh = m;
  }
}

#endif /* ifndef FUNCTION_dit_fft */
