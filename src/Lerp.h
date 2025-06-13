#ifndef LERP_H
#define LERP_H

#include <qtypes.h>

qreal repeat(qreal t, qreal length);
qreal lerp(qreal a, qreal b, qreal t);
qreal lerpAngle(qreal a, qreal b, qreal t);

#endif // LERP_H
