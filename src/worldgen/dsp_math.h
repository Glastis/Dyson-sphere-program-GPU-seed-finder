#ifndef DSP_MATH_H
#define DSP_MATH_H

#include "hd.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
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

HD static inline float lerp_f32(float a, float b, float t)
{
    return a + (b - a) * clampf(t, 0.0f, 1.0f);
}

HD static inline float rand_normal(float avg, float sd, double r1, double r2)
{
    double inner;

    inner = sqrt(-2.0 * log(1.0 - r1)) * sin(2.0 * M_PI * r2);
    return avg + sd * (float)inner;
}

HD static inline double log_base(double value, double base)
{
    return log(value) / log(base);
}

#endif
