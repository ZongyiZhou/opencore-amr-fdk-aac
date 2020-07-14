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

#ifndef _mm_loadu_si64
#define _mm_loadu_si64(p) _mm_loadl_epi64((__m128i const *)(p))
#endif
#define LOADH_64(v, p) \
  _mm_castps_si128(_mm_loadh_pi(_mm_castsi128_ps(v), (__m64 *)(p)))
#define SHUFPS(u, v, i) \
  _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(u), _mm_castsi128_ps(v), i))
#define STORELO(p, v) _mm_storel_pi((__m64 *)(p), _mm_castsi128_ps(v))
#define STOREHI(p, v) _mm_storeh_pi((__m64 *)(p), _mm_castsi128_ps(v))

#ifdef __GNUC__
inline __m128i PMOVSXDQ(const void* x) {
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
#define PMOVSXDQ(x) _mm_cvtepi32_epi64(*(__m128i *)(x))
#endif

#ifdef __LP64__
// Reduce rounding error on x86-64 
inline void cplxMult(INT64 *c_Re, INT64 *c_Im, const FIXP_DBL a_Re,
    const FIXP_DBL a_Im, const FIXP_DBL b_Re, const FIXP_DBL b_Im) {
  *c_Re = ((INT64)a_Re * b_Re - (INT64)a_Im * b_Im + 0x40000000) >> 31;
  *c_Im = ((INT64)a_Re * b_Im + (INT64)a_Im * b_Re + 0x40000000) >> 31;
}

inline void cplxMult(INT64 *c_Re, INT64 *c_Im, const FIXP_DBL a_Re,
                     const FIXP_DBL a_Im, const FIXP_STP cs) {
  return cplxMult(c_Re, c_Im, a_Re, a_Im, cs.v.re, cs.v.im);
}

inline FIXP_DBL addh(INT64 a, INT64 b) { return (FIXP_DBL)((a + b) >> 1); }
inline FIXP_DBL subh(INT64 a, INT64 b) { return (FIXP_DBL)((a - b) >> 1); }
#else
inline FIXP_DBL addh(FIXP_DBL a, FIXP_DBL b) {
  return (a & b) + ((a ^ b) >> 1);
}
inline FIXP_DBL subh(FIXP_DBL a, FIXP_DBL b) {
  return ((a ^ b) >> 1) - (~a & b);
}
#endif

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
    STORELO(x + i, x0127);
    STORELO(x + i + 4, x4563);
    __m128i r = _mm_unpackhi_epi32(x0127, x4563);  // 2673
    r = _mm_shuffle_epi32(r, 0x9c);                // 2367
    STORELO(x + i + 2, r);
    STOREHI(x + i + 6, r);
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
        __m128i u = _mm_loadu_si64(x + t1), v = _mm_loadu_si64(x + t2);
        u = LOADH_64(u, x + t1 + mh);
        v = LOADH_64(v, x + t2 + mh);
        v = _mm_shufflehi_epi16(v, 0x4e);
        __m128i r1 = _mm_and_si128(u, v), r2 = _mm_andnot_si128(u, v),
                r3 = _mm_srai_epi32(_mm_xor_si128(u, v), 1);
        r1 = _mm_add_epi32(r1, r3);
        r2 = _mm_sub_epi32(r3, r2);
        STORELO(x + t1, r1);
        STORELO(x + t2, r2);
        r1 = _mm_unpackhi_epi32(r1, r2);
        r1 = _mm_shuffle_epi32(r1, 0x9c);
        STORELO(x + t1 + mh, r1);
        STOREHI(x + t2 + mh, r1);
      }
    } /* end of  block 1 */

#ifdef __SSE4_1__  // SSE4.1 required for block 2
    const __m128i rnd_offset = _mm_set_epi32(0, 0x40000000, 0, 0x40000000);
    for (j = 1; j < mh / 4; ++j) {
      const FIXP_STP *cs = trigdata + j * trigstep;
      // load coef
#ifdef SINETABLE_16BIT
      __m128i cs0 =
          _mm_cvtepu16_epi32(_mm_cvtsi32_si128(cs->w));  // [Cr 0 Ci 0 0 0 0 0]
      cs0 = _mm_slli_epi32(cs0, 16);                     // [Cr<<16 Ci<<16 0 0]
#else
      __m128i cs0 = _mm_loadu_si64(&cs->w);  // [Cr Ci 0 0]
#endif
      __m128i cs1 = _mm_shuffle_epi32(cs0, 0x4E); // [0 0 Cr Ci]
      cs0 = _mm_sub_epi32(cs0, cs1);              // [Cr Ci -Cr -Ci]
      __m128i cs2 = _mm_shuffle_epi32(cs0, 0),    // [Cr x Cr x]
              cs3 = _mm_shuffle_epi32(cs0, 0xB1); //  [Ci x -Ci x]
      cs1 = _mm_shuffle_epi32(cs0, 0x55);         // [Ci x Ci x]
      for (r = 0; r < n; r += m) {
        INT t1 = (r + j) * 2;
        INT t2 = t1 + m;

        // vi = x1Cr - x0Ci, vr = x0Cr + x1Ci
        __m128i v0 = PMOVSXDQ(x + t2);            // [x0 x1]
        __m128i v1 = _mm_shuffle_epi32(v0, 0x4E); // [x1 x0]
        v0 = _mm_mul_epi32(v0, cs2);              // [x0Cr x1Cr]
        v1 = _mm_mul_epi32(v1, cs3);              // [x1Ci -x0Ci]
        v0 = _mm_add_epi64(v0, v1);               // [vr<<32 vi<<32]
        v0 = _mm_add_epi64(v0, rnd_offset);
        v1 = _mm_srli_epi64(v0, 31);              // [vr*2 vi*2]

        __m128i u = PMOVSXDQ(x + t1); // [x[t1] x[t1+1]]
        v0 = _mm_add_epi64(u, v1);    // [ur+vr ui+vi]
        v1 = _mm_sub_epi64(u, v1);    // [ur-vr ui-vi]
        v0 = _mm_srli_epi64(v0, 1);
        v1 = _mm_srli_epi64(v1, 1);
        v0 = SHUFPS(v0, v1, 0x88);

        STORELO(x + t1, v0);
        STOREHI(x + t2, v0);

        t1 += mh;
        t2 += mh;

        // -vr = x0Ci - x1Cr, vi = x0Cr + x1Ci
        v0 = PMOVSXDQ(x + t2);             // [x0 x1]
        v1 = _mm_shuffle_epi32(v0, 0x4E);  // [x1 x0]
        v0 = _mm_mul_epi32(v0, cs1);  // [x0Ci x1Ci]
        v1 = _mm_mul_epi32(v1, cs0);  // [x1Cr -x0Cr]
        v0 = _mm_sub_epi64(v0, v1);   // [-vr<<32 vi<<32]
        v0 = _mm_add_epi64(v0, rnd_offset);
        v1 = _mm_srli_epi64(v0, 31);  // [-vr*2 vi*2]

        u = PMOVSXDQ(x + t1);      // [x[t1] x[t1+1]]
        v0 = _mm_sub_epi64(u, v1); // [ur+vr ui-vi]
        v1 = _mm_add_epi64(u, v1); // [ur-vr ui+vi]
        v0 = _mm_srli_epi64(v0, 1);
        v1 = _mm_srli_epi64(v1, 1);
        v0 = SHUFPS(v0, v1, 0x88);
        STORELO(x + t1, v0);
        STOREHI(x + t2, v0);

        /* Same as above but for t1,t2 with j>mh/4 and thus cs swapped */
        t1 -= j * 4;
        t2 -= j * 4;

        // -vi = -x0Cr + x1Ci, vr = x1Cr + x0Ci
        v0 = PMOVSXDQ(x + t2);            // [x0 x1]
        v1 = _mm_shuffle_epi32(v0, 0x4E); // [x1 x0]
        v0 = _mm_mul_epi32(v0, cs1);      // [x0Ci x1Ci]
        v1 = _mm_mul_epi32(v1, cs0);      // [x1Cr -x0Cr]
        v0 = _mm_add_epi64(v0, v1);       // [vr<<32 -vi<<32]
        v0 = _mm_add_epi64(v0, rnd_offset);
        v1 = _mm_srli_epi64(v0, 31);      // [vr*2 -vi*2]

        u = PMOVSXDQ(x + t1);      // [x[t1] x[t1+1]]
        v0 = _mm_add_epi64(u, v1); // [ur+vr ui-vi]
        v1 = _mm_sub_epi64(u, v1); // [ur-vr ui+vi]
        v0 = _mm_srli_epi64(v0, 1);
        v1 = _mm_srli_epi64(v1, 1);
        v0 = SHUFPS(v0, v1, 0x88);
        STORELO(x + t1, v0);
        STOREHI(x + t2, v0);

        t1 += mh;
        t2 += mh;

        // vr = x0Cr - x1Ci, vi = x1Cr + x0Ci
        v0 = PMOVSXDQ(x + t2);            // [x0 x1]
        v1 = _mm_shuffle_epi32(v0, 0x4E); // [x1 x0]
        v0 = _mm_mul_epi32(v0, cs2);      // [x0Cr x1Cr]
        v1 = _mm_mul_epi32(v1, cs3);      // [x1Ci -x0Ci]
        v0 = _mm_sub_epi64(v0, v1);       // [vr<<32 vi<<32]
        v0 = _mm_add_epi64(v0, rnd_offset);
        v1 = _mm_srli_epi64(v0, 31);      // [vr*2 vi*2]

        u = PMOVSXDQ(x + t1);      // [x[t1] x[t1+1]]
        v0 = _mm_sub_epi64(u, v1); // [ur-vr ui-vi]
        v1 = _mm_add_epi64(u, v1); // [ur+vr ui+vi]
        v0 = _mm_srli_epi64(v0, 1);
        v1 = _mm_srli_epi64(v1, 1);
        v0 = SHUFPS(v0, v1, 0x88);
        STORELO(x + t1, v0);
        STOREHI(x + t2, v0);
      }
    }

#else // __SSE4_1__
    for (j = 1; j < mh / 4; ++j) {
      FIXP_STP cs;

      cs = trigdata[j * trigstep];

      for (r = 0; r < n; r += m) {
        INT t1 = (r + j) << 1;
        INT t2 = t1 + m;
#ifdef __LP64__
        INT64 vr, vi, ur, ui;
#else
        FIXP_DBL vr, vi, ur, ui;
#endif
        cplxMult(&vi, &vr, x[t2 + 1], x[t2], cs);
        ur = x[t1];
        ui = x[t1 + 1];

        x[t1] = addh(ur, vr);
        x[t1 + 1] = addh(ui, vi);
        x[t2] = subh(ur, vr);
        x[t2 + 1] = subh(ui, vi);

        t1 += mh;
        t2 += mh;

        cplxMult(&vr, &vi, x[t2 + 1], x[t2], cs);
        ur = x[t1];
        ui = x[t1 + 1];

        x[t1] = addh(ur, vr);
        x[t1 + 1] = subh(ui, vi);
        x[t2] = subh(ur, vr);
        x[t2 + 1] = addh(ui, vi);

        /* Same as above but for t1,t2 with j>mh/4 and thus cs swapped */
        t1 -= j * 4;
        t2 -= j * 4;

        cplxMult(&vi, &vr, x[t2], x[t2 + 1], cs);
        ur = x[t1];
        ui = x[t1 + 1];

        x[t1] = addh(ur, vr);
        x[t1 + 1] = subh(ui, vi);
        x[t2] = subh(ur, vr);
        x[t2 + 1] = addh(ui, vi);

        t1 += mh;
        t2 += mh;

        cplxMult(&vr, &vi, x[t2], x[t2 + 1], cs);
        ur = x[t1];
        ui = x[t1 + 1];

        x[t1] = subh(ur, vr);
        x[t1 + 1] = subh(ui, vi);
        x[t2] = addh(ur, vr);
        x[t2 + 1] = addh(ui, vi);
      }
    }
#endif

    { /* block 2 */
#ifdef __SSE4_1__
      // load coef
      const __m128i cs1 =
          _mm_set_epi32(0, -0x5a82799a, 0, 0x5a82799a);  // Cr -Cr
      const __m128i cs0 = _mm_shuffle_epi32(cs1, 0x44);  // Cr Cr

      for (r = 0; r < n; r += m) {
        INT t1 = r * 2 + mh / 2;
        INT t2 = t1 + m;

        // vi = Cr(x1 - x0), vr = Cr(x0 + x1)
        __m128i v0 = PMOVSXDQ(x + t2);            // [x0 x1]
        __m128i v1 = _mm_shuffle_epi32(v0, 0x4E); // [x1 x0]
        v0 = _mm_mul_epi32(v0, cs0);              // [Crx0 Crx1]
        v1 = _mm_mul_epi32(v1, cs1);              // [Crx1 -Crx0]
        v0 = _mm_add_epi64(v0, v1);  // [vr<<32 vi<<32]
        v0 = _mm_add_epi64(v0, rnd_offset);
        v1 = _mm_srli_epi64(v0, 31); // [vr*2 vi*2]

        __m128i u = PMOVSXDQ(x + t1); // [x[t1] x[t1+1]]
        v0 = _mm_add_epi64(u, v1);    // [ur+vr ui+vi]
        v1 = _mm_sub_epi64(u, v1);    // [ur-vr ui-vi]
        v0 = _mm_srli_epi64(v0, 1);
        v1 = _mm_srli_epi64(v1, 1);
        v0 = SHUFPS(v0, v1, 0x88);
        STORELO(x + t1, v0);
        STOREHI(x + t2, v0);

        t1 += mh;
        t2 += mh;

        // -vr = Cr(x0 - x1), vi = Cr(x0 + x1)
        v0 = PMOVSXDQ(x + t2);            // [x0 x1]
        v1 = _mm_shuffle_epi32(v0, 0x4E); // [x1 x0]
        v0 = _mm_mul_epi32(v0, cs0);      // [Crx0 Crx1]
        v1 = _mm_mul_epi32(v1, cs1);      // [Crx1 -Crx0]
        v0 = _mm_sub_epi64(v0, v1);       // [-vr<<32 vi<<32]
        v0 = _mm_add_epi64(v0, rnd_offset);
        v1 = _mm_srli_epi64(v0, 31);      // [-vr*2 vi*2]

        u = PMOVSXDQ(x + t1);      // [x[t1] x[t1+1]]
        v0 = _mm_sub_epi64(u, v1); // [ur+vr ui-vi]
        v1 = _mm_add_epi64(u, v1); // [ur+vr ui+vi]
        v0 = _mm_srli_epi64(v0, 1);
        v1 = _mm_srli_epi64(v1, 1);
        v0 = SHUFPS(v0, v1, 0x88);
        STORELO(x + t1, v0);
        STOREHI(x + t2, v0);
      }
#else
      for (r = 0; r < n; r += m) {
        INT t1 = r * 2 + mh / 2;
        INT t2 = t1 + m;
#ifdef __LP64__
        INT64 vr, vi, ur, ui;
#else
        FIXP_DBL vr, vi, ur, ui;
#endif
        cplxMult(&vi, &vr, x[t2 + 1], x[t2], (FIXP_DBL)0x5a82799a,
                 (FIXP_DBL)0x5a82799a);
        ur = x[t1];
        ui = x[t1 + 1];

        x[t1] = addh(ur, vr);
        x[t1 + 1] = addh(ui, vi);

        x[t2] = subh(ur, vr);
        x[t2 + 1] = subh(ui, vi);

        t1 += mh;
        t2 += mh;

        cplxMult(&vr, &vi, x[t2 + 1], x[t2], (FIXP_DBL)0x5a82799a,
                 (FIXP_DBL)0x5a82799a);
        ur = x[t1];
        ui = x[t1 + 1];

        x[t1] = addh(ur, vr);
        x[t1 + 1] = subh(ui, vi);

        x[t2] = subh(ur, vr);
        x[t2 + 1] = addh(ui, vi);
      }
#endif
    } /* end of block 2 */
    mh = m;
  }
}

#endif /* ifndef FUNCTION_dit_fft */
