#include "tinygl.h"

#ifndef IRCOMMUNICATION_H
#define IRCOMMUNICATION_H

/* Will attempt to hit the opponents boats
   If boat was hit, then tinygl point will be returned
   If not, NULL will be returned*/

uint8_t waitForBothPlayers(void);

// This will be called when a user is done with their round
void userDoneWithRound(void);

// This will be called to check if the user is done with their round
bool isUserDoneWithRound(void);

bool sendBomb();

#endif