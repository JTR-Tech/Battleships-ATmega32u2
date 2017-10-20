/** @file  ircommunication.c
    @author Rafael Goesmann (rgo51) Joshua Aitken (ajo107)
    @date   11th October 2017
    @brief This module handles everything to do with ir sending and receiving.
*/

#include "ir_uart.h"
#include "led.h"
#include "navswitch.h"
#include "pacer.h"
#include "system.h"

#define MAP_WIDTH 15
#define MAP_HEIGHT 11
// 'Magic Number' characters that will be sent with ir_uart_putc()
#define USER_IS_READY 'r'
#define DONE_MESSAGE 'd'
#define USER_HIT 'h'
#define MAP_EMPTY '0'
#define MAP_BORDER '1'
#define MAP_USER_SHIP '2'

/**
 * Will send the 'USER_IS_READY' message along with the map contents to the
 * opponent
 *
 * @param usersMap The layout of the placed battleships from the user.
 * '0' signifies empty space, '1' signifies terrain and '2' signifies
 * part of a users ship
 */
static void sendReadyAndMap(uint8_t usersMap[MAP_HEIGHT][MAP_WIDTH])
{

    /*
    Send messsage to opponent telling them that you have pressed the
    button
    */

    ir_uart_putc(USER_IS_READY);

    for (int i = 0; i < MAP_HEIGHT; i++) {
        for (int j = 0; j < MAP_WIDTH; j++) {
            // Send the map coordinate converted to char representation
            ir_uart_putc(usersMap[i][j] + '0');
        }
    }
}

/**
 * Inserts the returned char into the opponentsMap as an integer
 * @param opponentsMap Will insert the `returnedChar` as an integer into this
 * matrix
 * @param returnedChar Received char from the opponents map
 * @param currentWidth Pointer to the current width in the matrix
 * @param currentHeight Pointer to the current height in the matrix
 * @param notProcessing Pointer to show if user has finish processing
 * with the opponents map.
 */
static void insertIntoOpponentsMap(uint8_t opponentsMap[MAP_HEIGHT][MAP_WIDTH],
                                   char returnedChar, uint8_t *currentWidth,
                                   uint8_t *currentHeight, bool *notProcessing)
{
    // Insert the returnedChar at the current coordinate an an int
    // representation of the char
    opponentsMap[*currentHeight][*currentWidth] = returnedChar - '0';

    // If currently at end of row, then increment counter
    if (*currentWidth == 14) {
        *currentHeight += 1;
        *currentWidth = 0;

        // If the currentHeight is 11, then transmission is complete
        if (*currentHeight == 11) {
            led_set(LED1, 1);
            // Notify function that user is not processing opponentsMap
            *notProcessing = true;
        }
    } else {
        *currentWidth += 1;
    }
}

/**
 * Will block until both players have pressed the NAVSWITCH_PUSH button.
 * @param usersMap Contains the users battleship layout '0' for empty terrain
 * '1' for border and '2' for users ship
 * @param opponentsMap Empty opponentsMap matrix that will be populated when
 * user sends their battleship layout
 * @return Will return true if user pressed first otherwise it will return false
 */

bool waitForBothPlayers(uint8_t usersMap[MAP_HEIGHT][MAP_WIDTH],
                        uint8_t opponentsMap[MAP_HEIGHT][MAP_WIDTH])
{
    bool userReady = false;
    bool opponentReady = false;
    bool goesFirst;

    uint8_t currentWidth = 0;
    uint8_t currentHeight = 0;

    bool notProcessing = false;

    while (1) {
        pacer_wait();
        navswitch_update();
        // If button is pushed, then player is ready
        if (navswitch_push_event_p(NAVSWITCH_PUSH)) {
            userReady = true;

            // If opponent has not pressed, then user will go first
            if (opponentReady == false) {
                goesFirst = true;
            } else {
                goesFirst = false;
            }

            // Send messages to opponent
            sendReadyAndMap(usersMap);
        }

        // If messaged recieved, then opponent is ready
        if (ir_uart_read_ready_p()) {
            // Making sure that the write message was sent
            char returnedChar = ir_uart_getc();

            if (returnedChar == USER_IS_READY) {
                opponentReady = true;

                // If user pressed button first, then user will go first
                if (userReady == true) {
                    goesFirst = true;
                } else {
                    goesFirst = false;
                }
            } else if ((returnedChar == MAP_EMPTY ||
                        returnedChar == MAP_BORDER ||
                        returnedChar == MAP_USER_SHIP) &&
                       currentHeight != MAP_HEIGHT) {

                // Insert into opponents map and update indexes accordingly
                insertIntoOpponentsMap(opponentsMap, returnedChar,
                                       &currentWidth, &currentHeight,
                                       &notProcessing);
            }
        }

        // If user and opponent have pressed navswitch and user is not
        // processing opponents map, then return

        if (userReady && opponentReady && notProcessing) {
            return goesFirst;
        }
    }
    return 1;
}

/** Call this after user has finished their round.
 *  Will notify the opponent that their turn has finished
 */
void userDoneWithRound(void)
{
    ir_uart_putc(DONE_MESSAGE);
}

/*
    Checks to see if the user has finished with their round
    Call this function while user is currently waiting for their round

    @return Will return true if user is done with round, false otherwise
*/
bool isUserDoneWithRound(void)
{
    if (ir_uart_read_ready_p()) {
        if (ir_uart_getc() == DONE_MESSAGE) {
            return true;
        }
    }

    return false;
}
