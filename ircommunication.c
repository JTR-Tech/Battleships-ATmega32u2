#include "ir_uart.h"
#include "led.h"
#include "navswitch.h"
#include "pacer.h"
#include "system.h"
#include <stdbool.h>

#define USER_IS_READY 'r'
#define DONE_WITH_ROUND 'd'

uint8_t waitForBothPlayers(void)
{

    uint8_t userReady = 0;
    uint8_t opponentReady = 0;
    uint8_t goesFirst;

    while (1) {
        pacer_wait();
        navswitch_update();
        // If button is pushed, then player is ready
        if (navswitch_push_event_p(NAVSWITCH_PUSH)) {
            userReady = 1;

            if (opponentReady == 0) {
                goesFirst = 1;
            } else {
                goesFirst = 0;
            }
            ir_uart_putc(USER_IS_READY);
        }

        // If messaged recieved, then opponent is ready
        if (ir_uart_read_ready_p()) {
            if (ir_uart_getc() == USER_IS_READY) {
                if (userReady == 1) {
                    goesFirst = 1;
                } else {
                    goesFirst = 0;
                }
                opponentReady = 1;
            }
        }

        if (userReady && opponentReady) {
            return goesFirst;
        }
    }
    return 1;
}

void userDoneWithRound(void)
{
    ir_uart_putc(DONE_WITH_ROUND);
}

bool isUserDoneWithRound(void)
{
    if (ir_uart_read_ready_p()) {
        if (ir_uart_getc() == DONE_WITH_ROUND) {
            return true;
        }
    }

    return false;
}

bool sendBomb(uint8_t x, uint8_t y)
{

    char[5];
    ir_uart_puts(sprint);
}
