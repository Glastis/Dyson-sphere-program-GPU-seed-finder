#ifndef DSP_VECTOR3_H
#define DSP_VECTOR3_H

#include "hd.h"
#include <math.h>

typedef struct
{
    double x;
    double y;
    double z;
}
vec3;

HD static inline vec3 vec3_make(double x, double y, double z)
{
    vec3 result;

    result.x = x;
    result.y = y;
    result.z = z;
    return result;
}

HD static inline vec3 vec3_zero(void)
{
    return vec3_make(0.0, 0.0, 0.0);
}

HD static inline double vec3_distance_sq(const vec3 *a, const vec3 *b)
{
    double dx;
    double dy;
    double dz;

    dx = b->x - a->x;
    dy = b->y - a->y;
    dz = b->z - a->z;
    return dx * dx + dy * dy + dz * dz;
}

HD static inline double vec3_distance(const vec3 *a, const vec3 *b)
{
    return sqrt(vec3_distance_sq(a, b));
}

HD static inline double vec3_magnitude_sq(const vec3 *a)
{
    return a->x * a->x + a->y * a->y + a->z * a->z;
}

HD static inline double vec3_magnitude(const vec3 *a)
{
    return sqrt(vec3_magnitude_sq(a));
}

#endif
