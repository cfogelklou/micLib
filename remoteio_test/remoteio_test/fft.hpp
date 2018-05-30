#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

#ifdef WIN32
#define FFT_DBG
#endif

#ifdef FFT_DBG
#include <stdio.h>
#endif

#ifndef MREQ_PI
#define MREQ_PI       3.14159265358979323846264338327
#endif

#ifndef MREQ_TWO_PI
#define MREQ_TWO_PI   (2 * MREQ_PI)
#endif

typedef unsigned int uint_t;
#define AS_ASSERT(x) assert(x)
#define AS_ASSERT_FN(x) assert(x)

class Fft {
private:
  typedef struct
  {
    double sm2;
    double sm1;
    double cm2;
    double cm1;
  } FftLut_t;

  const uint_t mFftSize;
  const uint_t mBitsNeeded;
  uint_t * const mpReverseBitsLUT;
  FftLut_t * mpFftLUT;
  FftLut_t * mpIfftLUT;
#ifdef FFT_DBG
  FILE * pf;
#endif


public:
  //---------------------------------------------------------------------------
  // Initializes or re-initializes the FFT.
  Fft(uint_t nSize)  
  : mFftSize(nSize)
  , mBitsNeeded(NumberOfBitsNeeded(nSize))
  , mpReverseBitsLUT(new uint_t[mFftSize])
  , mpFftLUT(nullptr)
  , mpIfftLUT(nullptr)
  {
    AS_ASSERT(IsPowerOfTwo(mFftSize));
    AS_ASSERT(NumberOfBitsNeeded(mFftSize) > 0);
    
    fft_InitTwiddles();
    AS_ASSERT(mpReverseBitsLUT);

    for (uint_t i = 0; i < nSize; i++) {
      mpReverseBitsLUT[i] = ReverseBits(i, mBitsNeeded);
    }
#ifdef FFT_DBG
    if (pf == nullptr) {
      pf = fopen("fft_dbg.csv", "w");
    }
    if (pf != nullptr) {
      fprintf(pf, "tIdx, bIdx, pAdRealOut[tIdx], pAdImagOut[tIdx], pAdRealOut[bIdx], pAdImagOut[bIdx]\n");
    }
#endif

  }


  //---------------------------------------------------------------------------
  ~Fft()
  {
    delete mpReverseBitsLUT;
    delete mpFftLUT;
    delete mpIfftLUT;
#ifdef FFT_DBG
    if (pf != nullptr) {
      fclose(pf);
      pf = nullptr;
    }
#endif
  }


  //---------------------------------------------------------------------------
  static bool IsPowerOfTwo(const int nX)
  {
    return ((nX & -nX) == nX);
  }


  //---------------------------------------------------------------------------
  static int NumberOfBitsNeeded(
    const uint_t nPowerOfTwo
  )
  {
    
    bool success = false;    
    int bitsNeeded = -1;
    if (nPowerOfTwo < 2) {
      bitsNeeded = 0;
    }

    uint_t i = 0;
    while ((i < 32) && (success == false)) {
      if (nPowerOfTwo & (1 << i))  {
        bitsNeeded = i;
        success = true;
      }
      ++i;
    }
    return (success) ? bitsNeeded : 0;
  }

  //---------------------------------------------------------------------------
  static uint_t ReverseBits(
    uint_t nIndex, 
    const uint_t nNumBits)
  {

    uint_t rev = 0;
    if (nIndex != 0) {
      for (uint_t i = 0; i < nNumBits; i++) {
        rev = (rev << 1) | (nIndex & 1);
        nIndex >>= 1;
      }
    }

    return rev;
  }

  //---------------------------------------------------------------------------
  double  IndexToFrequency(
    const uint_t nNumSamples, 
    const uint_t nIndex, 
    const double fs)
  {
    double rval = 0.0;
    if (nIndex < (nNumSamples / 2)) {
      rval = (double)nIndex / (double)nNumSamples;
    }
    else {
      rval = -((double)(nNumSamples - nIndex) / (double)nNumSamples);
    }
    rval *= fs;
    return rval;
  }


  //---------------------------------------------------------------------------
  void DoFFT(
    const double adRealIn[],
    const double adImagIn[],
    double adRealOut[],
    double adImagOut[],
    uint_t const nSize)

  {
    fft_ifft(false, adRealIn, adImagIn, adRealOut, adImagOut, nSize);
#ifdef FFT_DBG
    if (pf != nullptr) {
      for (uint_t i = 0; i < nSize; i++) {
        fprintf(pf, "xr[ i=%d ], xi[ i=%d ] = %d, %d\n", i, i, (int32_t)adRealOut[i], (int32_t)adImagOut[i]);
      }
    }
#endif    
  }


  //---------------------------------------------------------------------------
  void DoIFFT(
    const double adRealIn[],
    const double adImagIn[],
    double adRealOut[],
    double adImagOut[],
    uint_t const nSize)
  {
    fft_ifft(true, adRealIn, adImagIn, adRealOut, adImagOut, nSize);
  }


  //---------------------------------------------------------------------------
  bool   GetPhase(
    double pAdRealIn[],
    double pAdImagIn[],
    double adPhase[],
    const uint_t nSize)
  {

    bool status = false;
    AS_ASSERT((pAdRealIn != nullptr) && (adPhase != nullptr));
    if (pAdImagIn == nullptr) {
      for (uint_t i = 0; i < nSize; i++) {
        adPhase[i] = 0.0;
      }
      status = true;
    }
    else {
      for (uint_t i = 0; i < nSize; i++)  {
        adPhase[i] = (pAdRealIn[i] == 0) ? MREQ_PI / 2 : atan(pAdImagIn[i] / pAdRealIn[i]);
      }
      status = true;
    }
    return status;
  }


  //---------------------------------------------------------------------------
  bool GetMagnitude(
    double pAdRealIn[],
    double pAdImagIn[],
    double adMagnitude[],
    const uint_t nSize)
  {
    AS_ASSERT((pAdRealIn != nullptr) && (adMagnitude != nullptr));
    for (uint_t i = 0; i < nSize; i++) {
      adMagnitude[i] = sqrt(pow(pAdRealIn[i], 2) + pow(pAdImagIn[i], 2));
    }
    return true;
  }

  //---------------------------------------------------------------------------
  bool GetMagnitudeAndPhase(
    double pAdRealIn[], 
    double pAdImagIn[],
    double pAdMagnitude[],
    double pAdPhase[],
    const uint_t nSize)
  {
    uint_t i;
    AS_ASSERT((pAdRealIn != nullptr) && (pAdMagnitude != nullptr) && (pAdPhase != nullptr));
    for (i = 0; i < nSize; i++)
    {
      pAdMagnitude[i] = sqrt(pow(pAdRealIn[i], 2) + pow(pAdImagIn[i], 2));
      pAdPhase[i] = (pAdRealIn[i] == 0) ? MREQ_PI / 2 : atan(pAdImagIn[i] / pAdRealIn[i]);
    }
    return true;
  }

private:
  
  //---------------------------------------------------------------------------
  void fft_ifft(
    const bool bInverseTransform,
    const double * const pAdRealIn,
    const double * const pAdImagIn,
    double * const xr,
    double * const xi,
    const uint_t nNumSamples)
  {
    AS_ASSERT((nNumSamples == mFftSize) && (pAdRealIn != nullptr) && (xr != nullptr) && (xi != nullptr));

    // Reverse ordering of samples so FFT can be done in place.
    if (pAdImagIn == nullptr) {
      for (uint_t i = 0; i < nNumSamples; i++) {
        const uint_t rev = mpReverseBitsLUT[i];
        xr[rev] = pAdRealIn[i];
        xi[rev] = 0.0;
      }
    }
    else {
      for (uint_t i = 0; i < nNumSamples; i++) {
        const uint_t rev = mpReverseBitsLUT[i];
        xr[rev] = pAdRealIn[i];
        xi[rev] = pAdImagIn[i];
      }
    }

    // Declare some local variables and start the FFT.
    {
      uint_t nBlockSize;

      const FftLut_t * const pLut = (!bInverseTransform) ? mpFftLUT : mpIfftLUT;
      uint_t iter = 0;
      uint_t nBlockEnd = 1;

#ifdef FFT_DBG
      uint_t innerloops = 0;
#endif

      for (nBlockSize = 2; nBlockSize <= nNumSamples; nBlockSize <<= 1) {
        const double sm2 = pLut[iter].sm2;
        const double sm1 = pLut[iter].sm1;
        const double cm2 = pLut[iter].cm2;
        const double cm1 = pLut[iter].cm1;
        const double w = 2 * cm1;
        double ar[3], ai[3];
        ++iter;

        for (uint_t i = 0; i < nNumSamples; i += nBlockSize) {
          uint_t j, n;

          ar[2] = cm2;
          ar[1] = cm1;

          ai[2] = sm2;
          ai[1] = sm1;

          for (j = i, n = 0; n < nBlockEnd; j++, n++) {

            double tr, ti;     /* temp real, temp imaginary */
            const uint_t k = j + nBlockEnd;

            ar[0] = w * ar[1] - ar[2];
            ar[2] = ar[1];
            ar[1] = ar[0];

            ai[0] = w * ai[1] - ai[2];
            ai[2] = ai[1];
            ai[1] = ai[0];


            tr = ar[0] * xr[k] - ai[0] * xi[k];
            ti = ar[0] * xi[k] + ai[0] * xr[k];

#ifdef FFT_DBG
            if (pf != nullptr) {
              fprintf(pf, "Innerloops: %d\n", innerloops++);
              fprintf(pf, "tr = ar * xr[ k=%d ] - ai * xi[ k=%d ] = (%f * %f) - (%f * %f) = %f\n", k, k, ar[0], xr[k], ai[0], xi[k], tr);
              fprintf(pf, "ti = ar * xi[ k=%d ] + ai * xr[ k=%d ] = (%f * %f) + (%f * %f) = %f\n", k, k, ar[0], xi[k], ai[0], xr[k], ti);
            }
#endif

            xr[k] = xr[j] - tr;
            xi[k] = xi[j] - ti;

#ifdef FFT_DBG
            if (pf != nullptr) {
              fprintf(pf, "xr[ k=%d ] = xr[ j=%d ] - tr = %f - (%f) = %f\n", k, j, xr[j], tr, xr[k]);
              fprintf(pf, "xi[ k=%d ] = xi[ j=%d ] - ti = %f - (%f) = %f\n", k, j, xi[j], ti, xi[k]);
            }
#endif

            {
              const double oldxrj = xr[j];
              const double oldxij = xi[j];
              xr[j] = xr[j] + tr;
              xi[j] = xi[j] + ti;

#ifdef FFT_DBG
              if (pf != nullptr) {
                fprintf(pf, "xr[ j=%d ] = xr[ j=%d ] + tr = %f + %f = %f\n", j, j, oldxrj, tr, xr[j]);
                fprintf(pf, "xi[ j=%d ] = xi[ j=%d ] + ti = %f + %f = %f\n", j, j, oldxij, ti, xi[j]);
              }
#endif
            }
          }
        }

        nBlockEnd = nBlockSize;
      }
    }

    // Normalize
    if (bInverseTransform) {
      double dDenom = 1.0 / (double)nNumSamples;

      for (uint_t i = 0; i < nNumSamples; i++) {
        xr[i] *= dDenom;
        xi[i] *= dDenom;
      }
    }

  }

  //---------------------------------------------------------------------------
  void fft_InitTwiddles() 
  {

    uint_t numSamples = mFftSize;
    uint_t lutSize = 0;
    for (uint_t nBlockSize = 2; nBlockSize <= numSamples; nBlockSize <<= 1) {
      ++lutSize;
    }

    mpFftLUT = new FftLut_t[lutSize];
    mpIfftLUT = new FftLut_t[lutSize]; 

    // Do once for regular transform, once for inverse
    for (uint_t i = 0; i < 2; i++)
    {
      uint_t j = 0;
      // lookup tables for sm and cm.
      FftLut_t *pLut = (i == 0) ? mpFftLUT : mpIfftLUT;
      // Inverse, not inverse
      double dAngleNumerator = (i == 0) ? -MREQ_TWO_PI : MREQ_TWO_PI;
      for (uint_t nBlockSize = 2; nBlockSize <= numSamples; nBlockSize <<= 1)
      {
        const double dDeltaAngle = dAngleNumerator / (double)nBlockSize;
        pLut[j].sm2 = sin(-2 * dDeltaAngle);
        pLut[j].sm1 = sin(-dDeltaAngle);
        pLut[j].cm2 = cos(-2 * dDeltaAngle);
        pLut[j].cm1 = cos(-dDeltaAngle);
        ++j;
      }
    }

  }
};
