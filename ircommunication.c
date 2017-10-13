#include "ir_uart.h"
#include "led.h"
#include "navswitch.h"
#include "pacer.h"
#include "system.h"

// Characters that will be sent with ir_uart_putc()
#define USER_IS_READY 'r'
#define DONE_WITH_ROUND 'd'
#define USER_HIT 'h'

/* Will block until both players have pressed the NAVSWITCH_PUSH button.
   Will return true if user pressed first otherwise it will return false*/
bool waitForBothPlayers(void)
{

    bool userReady = false;
    bool opponentReady = false;
    bool goesFirst;

    while (1) {
        pacer_wait();
        navswitch_update();
        // If button is pushed, then player is ready
        if (navswitch_push_event_p(NAVSWITCH_PUSH)) {
            userReady = false;

            // If opponent has not pressed, then user will go first
            if (opponentReady == false) {
                goesFirst = true;
            } else {
                goesFirst = false;
            }

            // Send messsage to opponent telling them that you have pressed the
            // button
            ir_uart_putc(USER_IS_READY);
        }

        // If messaged recieved, then opponent is ready
        if (ir_uart_read_ready_p()) {
            // Making sure that the write message was sent
            if (ir_uart_getc() == USER_IS_READY) {
                opponentReady = true;

                // If user pressed button first, then user will go first
                if (userReady == true) {
                    goesFirst = true;
                } else {
                    goesFirst = false;
                }
            }
        }

        // If both users are ready then return who pressed first
        if (userReady && opponentReady) {
            return goesFirst;
        }
    }
    return 1;
}

/* Call this after user has finished their round.
   Will notify the opponent that their turn has finished
*/
void userDoneWithRound(void)
{
    ir_uart_putc(DONE_WITH_ROUND);
}

/*
    Checks to see if the user has finished with their round
    Call this function while user is currently waiting for their round
*/
bool isUserDoneWithRound(void)
{
    if (ir_uart_read_ready_p()) {
        if (ir_uart_getc() == DONE_WITH_ROUND) {
            return true;
        }
    }

    return false;
}

/*

*/
bool sendMissile(uint8_t x, uint8_t y)
{

    char[5] coordinates;
    sprintf(coordinates, "%d%d", xy);
    ir_uart_puts(coordinates);

    while (1) {
        if (ir_uart_read_ready_p()) {
            if (ir_uart_getc() == USER_HIT) {

            } else {
            }
        }
    }
}
