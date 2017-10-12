#include "tinygl.h"

#ifndef IRCOMMUNICATION_H
#define IRCOMMUNICATION_H

/* Will attempt to hit the opponents boats
   If boat was hit, then tinygl point will be returned
   If not, NULL will be returned*/
tinygl_point_t attemptShot(uint8_t x, uint8_t y);

#endif