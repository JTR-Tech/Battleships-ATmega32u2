/** @file  ircommunication.h
    @author Rafael Goesmann (rgo51) Joshua Aitken (ajo107)
    @date   10th October 2017
    @brief Contains public functions signatures from ircommunication.c
*/

#ifndef IRCOMMUNICATION_H
#define IRCOMMUNICATION_H

#define MAP_WIDTH 15
#define MAP_HEIGHT 11

/**
 * Will block until both players have pressed the NAVSWITCH_PUSH button.
 * Will return true if user pressed first otherwise it will return false
 * @param usersMap MAP_HEIGHT*MAP_WIDTH matrix showing the users battleship
 * layout. 1's signify borders, 0 signify empty space and 2 signifies a users
 * ship
 * @param opponentsMap An empty MAP_HEIGHT * MAP_WIDTH matrix. This matrix
 * will be populated from the opponents map.
 */
bool waitForBothPlayers(uint8_t usersMap[MAP_HEIGHT][MAP_WIDTH],
                        uint8_t opponentsMap[MAP_HEIGHT][MAP_WIDTH]);

/**
 * Call this after user has finished their round.
 * Will notify the opponent that their turn has finished
 */
void userDoneWithRound(void);

/**
 * Checks to see if the user has finished with their round
 * Call this function while user is currently waiting for their round
 * @return Will return true if user is done, false otherwise.
 */
bool isUserDoneWithRound(void);

#endif