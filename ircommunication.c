#include "ir_uart.h"
#include "tinygl.h"

int connectUsers()
{

    if (navswitch_push_event_p(NAVSWITCH_PUSH) {
        return 0;    
    }

    while (1) {
        pacer_wait();
        // Wait for a connection
        if (ir_uart_read_ready_p()) {
            return 1;
        }
    }
}

tinygl_point_t attemptShot(uint8_t x, uint8_t y)
{
    char str[10];

    ir_uart_puts(sprintf(str, "%d%d", x, y));
}