#ifndef DSP_HD_H
#define DSP_HD_H

#ifndef HD
#if defined(__CUDACC__)
#define HD __host__ __device__
#else
#define HD
#endif
#endif

#ifndef DSP_CONST
#if defined(__CUDACC__)
#define DSP_CONST __device__
#else
#define DSP_CONST
#endif
#endif

#endif
