#include "ir_uart.h"
#include "tinygl.h"

tinygl_point_t attemptShot(uint8_t x, uint8_t y)
{
    char str[10];

    ir_uart_puts(sprintf(str, "%d%d", x, y));
}