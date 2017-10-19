/** @file  ircommunication.h
    @author Rafael Goesmann Joshua Aitken
    @date   10th October 2017

    Purpose: Contains public functions signatures from ircommunication.c
*/

#ifndef IRCOMMUNICATION_H
#define IRCOMMUNICATION_H

#define MAP_WIDTH 15
#define MAP_HEIGHT 11

/*
    Will block until both players have pressed the NAVSWITCH_PUSH button.
    Will return true if user pressed first otherwise it will return false
*/
bool waitForBothPlayers(uint8_t usersMap[MAP_HEIGHT][MAP_WIDTH],
                        uint8_t opponentsMap[MAP_HEIGHT][MAP_WIDTH]);

/*
    Call this after user has finished their round.
    Will notify the opponent that their turn has finished
*/
void userDoneWithRound(void);

/*
    Checks to see if the user has finished with their round
    Call this function while user is currently waiting for their round
*/
bool isUserDoneWithRound(void);

#endif