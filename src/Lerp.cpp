#include <cmath>
#include <algorithm>
#include "Lerp.h"

#define M_2PI 6.283185307179586

qreal repeat(qreal t, qreal length)
{
    return std::clamp(t - std::floor(t / length) * length, 0.0, length);
}

template <typename T>
T clamp01(T a)
{
    if (a > 1.0) return 1.0;
    if (a < 0.0) return 0.0;
    return a;
}

qreal lerp(qreal a, qreal b, qreal t)
{
    return (b - a) * t + a;
}

qreal lerpAngle(qreal a, qreal b, qreal t)
{
    qreal delta = repeat(b - a, 360.0);

    if (delta > 180.0)
    {
        delta -= 360.0;
    }

    return a + delta * clamp01(t);
}
