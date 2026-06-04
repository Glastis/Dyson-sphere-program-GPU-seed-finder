#ifndef DSP_MATH_H
#define DSP_MATH_H

#include "hd.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* --- FP32-on-device: the `wreal` ("worldgen real") type --------------------
 * `wreal` carries the heavy floating-point transcendentals of the hot path
 * (the star/planet pow/log/sqrt/sin in star.h + planet_props.h). On the GPU it
 * is `float` so the kernel uses the fast single-precision transcendental units
 * (FP64 runs at ~1/64 FP32 on consumer Blackwell); on the host it stays
 * `double` so the CPU reference engine remains bit-exact ground truth.
 *
 * IMPORTANT scope: only the transcendentals migrate. vec3 positions/distances
 * (vector3.h), star/pose placement (galaxy_gen.h) and the PRNG (prng.h) all
 * stay FP64 unconditionally -- keeping distances exact is what drives the GPU
 * false-negative rate to ~0 while still capturing the ~5x kernel speedup.
 *
 * Define DSP_FORCE_FP64_DEVICE to compile the *device* in FP64 too: used by the
 * GPU==CPU exactness test so it can assert bit-equality of the kernel logic
 * independently of precision. The host is unaffected by that macro. */
#if defined(__CUDA_ARCH__) && !defined(DSP_FORCE_FP64_DEVICE)
typedef float wreal;    /* device GPU: FP32 (production) */
#define WREAL_IS_FLOAT 1
#else
typedef double wreal;   /* host CPU, or device in FP64 test mode: FP64 exact */
#define WREAL_IS_FLOAT 0
#endif

#if WREAL_IS_FLOAT
HD static inline wreal wpow(wreal a, wreal b) { return powf(a, b); }
HD static inline wreal wlog(wreal a) { return logf(a); }
HD static inline wreal wlog10(wreal a) { return log10f(a); }
HD static inline wreal wsqrt(wreal a) { return sqrtf(a); }
HD static inline wreal wsin(wreal a) { return sinf(a); }
HD static inline wreal wfabs(wreal a) { return fabsf(a); }
#else
HD static inline wreal wpow(wreal a, wreal b) { return pow(a, b); }
HD static inline wreal wlog(wreal a) { return log(a); }
HD static inline wreal wlog10(wreal a) { return log10(a); }
HD static inline wreal wsqrt(wreal a) { return sqrt(a); }
HD static inline wreal wsin(wreal a) { return sin(a); }
HD static inline wreal wfabs(wreal a) { return fabs(a); }
#endif

HD static inline float clampf(float value, float lo, float hi)
{
    if (value < lo)
    {
        return lo;
    }
    if (value > hi)
    {
        return hi;
    }
    return value;
}

HD static inline double clampd(double value, double lo, double hi)
{
    if (value < lo)
    {
        return lo;
    }
    if (value > hi)
    {
        return hi;
    }
    return value;
}

HD static inline wreal clampw(wreal value, wreal lo, wreal hi)
{
    if (value < lo)
    {
        return lo;
    }
    if (value > hi)
    {
        return hi;
    }
    return value;
}

HD static inline float lerp_f32(float a, float b, float t)
{
    return a + (b - a) * clampf(t, 0.0f, 1.0f);
}

HD static inline float rand_normal(float avg, float sd, double r1, double r2)
{
    wreal inner;

    inner = wsqrt((wreal)(-2.0) * wlog((wreal)(1.0 - r1))) * wsin((wreal)(2.0 * M_PI * r2));
    return avg + sd * (float)inner;
}

HD static inline wreal log_base(wreal value, wreal base)
{
    return wlog(value) / wlog(base);
}

#endif
