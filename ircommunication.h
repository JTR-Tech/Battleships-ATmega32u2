#include "tinygl.h"

#ifndef IRCOMMUNICATION_H
#define IRCOMMUNICATION_H

#define MAP_WIDTH 15
#define MAP_HEIGHT 11

/* Will block until both players have pressed the NAVSWITCH_PUSH button.
   Will return true if user pressed first otherwise it will return false*/
uint8_t waitForBothPlayers(const uint8_t usersMap[MAP_HEIGHT][MAP_WIDTH],
                           uint8_t opponentsMap[MAP_HEIGHT][MAP_WIDTH]);

/* Call this after user has finished their round.
   Will notify the opponent that their turn has finished
*/
void userDoneWithRound(void);

/*
    Checks to see if the user has finished with their round
    Call this function while user is currently waiting for their round
*/
bool isUserDoneWithRound(void);

/*
    Send the contents of the the users layout to the opponent
*/
void sendMap(const int map[MAP_HEIGHT][MAP_WIDTH]);

/*
    Will block until it has retrieved the opponents map
*/
void getOpponentsMap(int opponentsMap[MAP_HEIGHT][MAP_WIDTH]);

#endif