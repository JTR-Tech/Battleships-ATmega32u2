#include "../fonts/font5x7_1.h"
#include "button.h"
#include "ir_uart.h"
#include "led.h"
#include "navswitch.h"
#include "pacer.h"
#include "pio.h"
#include "system.h"
#include "tinygl.h"
#include <stdio.h>
//#include "task.h"

/* Define polling rate in Hz.  */
#define LOOP_RATE 300
#define LED_RATE 1
// Constants for map and display size
#define MAP_WIDTH 5
#define MAP_HEIGHT 7
#define HORZ 1
#define VERT 0

// 0 = placingships, 1 = players turn, 3 = player waiting TODO: use it
// uint8_t gameState = 0;

// below is a simple "map", 1's indicating LED(on), 0's indicating LED(off).
// If the MAP_WIDTH and MAP_HEIGHT values  are changed, the layout array
// must be updated

typedef struct ship_s {
    uint8_t startingPoint;
    uint8_t endingPoint;
    uint8_t direction;
    uint8_t otherAxis;
} Ship;

typedef struct num_of_ships_s {
    uint8_t fourPoints;
    uint8_t threePoints;
    uint8_t twoPoints;
    uint8_t onePoints;
} NumberOfShips;

uint8_t layout[MAP_HEIGHT][MAP_WIDTH] = {
    {0, 0, 0, 0, 0},

    {0, 0, 0, 0, 0},

    {0, 0, 0, 0, 0},

    {0, 0, 0, 0, 0},

    {0, 0, 0, 0, 0},

    {0, 0, 0, 0, 0},

    {0, 0, 0, 0, 1},
};

void printOrClearShip(Ship* ship, bool printShip)
{
    if (ship->direction == VERT) {
        for (uint8_t i = ship->startingPoint; i <= ship->endingPoint; i++) {
            tinygl_point_t p = {ship->otherAxis, i};
            tinygl_draw_point(p, printShip ? 1 : 0);
        }
    } else {
        for (uint8_t i = ship->startingPoint; i <= ship->endingPoint; i++) {
            tinygl_point_t p = {i, ship->otherAxis};
            tinygl_draw_point(p, printShip ? 1 : 0);
        }
    }
}

void moveShip(Ship* ship)
{
    if (navswitch_push_event_p(NAVSWITCH_NORTH)) {
        printOrClearShip(ship, 0);
        if (ship->direction == VERT) {
            if (ship->startingPoint > 0) {
                ship->startingPoint -= 1;
                ship->endingPoint -= 1;
            }
        } else {
            if (ship->otherAxis > 0) {
                ship->otherAxis -= 1;
            }
        }
    } else if (navswitch_push_event_p(NAVSWITCH_EAST)) {
        printOrClearShip(ship, 0);

        if (ship->direction == HORZ) {
            if (ship->endingPoint < 4) {
                ship->startingPoint += 1;
                ship->endingPoint += 1;
            }
        } else {
            if (ship->otherAxis < 4) {
                ship->otherAxis += 1;
            }
        }

    } else if (navswitch_push_event_p(NAVSWITCH_SOUTH)) {
        printOrClearShip(ship, 0);
        if (ship->direction == VERT) {
            if (ship->endingPoint < 6) {
                ship->startingPoint += 1;
                ship->endingPoint += 1;
            }
        } else {
            if (ship->otherAxis < 6) {
                ship->otherAxis += 1;
            }
        }

    } else if (navswitch_push_event_p(NAVSWITCH_WEST)) {
        printOrClearShip(ship, 0);

        if (ship->direction == HORZ) {
            if (ship->startingPoint > 0) {
                ship->startingPoint -= 1;
                ship->endingPoint -= 1;
            }
        } else {
            if (ship->otherAxis > 0) {
                ship->otherAxis -= 1;
            }
        }
    }
}

void saveShipToMap(Ship* ship)
{
    if (ship->direction == VERT) {
        for (int i = ship->startingPoint; i <= ship->endingPoint; i++) {
            layout[i][ship->otherAxis] = 1;
        }
    } else {
        for (int i = ship->startingPoint; i <= ship->endingPoint; i++) {
            layout[ship->otherAxis][i] = 1;
        }
    }
}

// For now, it will just rotate in the
void rotateShip(Ship* ship)
{
    printOrClearShip(ship, 0);
    if (ship->direction == VERT) {
        ship->direction = HORZ;
        ship->otherAxis = 3;
        ship->endingPoint = ship->endingPoint - ship->startingPoint;
        ship->startingPoint = 0;
    } else {
        ship->direction = VERT;
        ship->otherAxis = 2;
        ship->endingPoint = ship->endingPoint - ship->startingPoint;
        ship->startingPoint = 0;
    }
}

void printLayout(void)
{
    for (int i = 0; i < MAP_WIDTH; i++) {
        for (int j = 0; j < MAP_HEIGHT; j++) {
            tinygl_point_t p = {i, j};
            tinygl_draw_point(p, layout[j][i]);
        }
    }
}

void clearMap()
{
    led_set(LED1, 1);
    tinygl_clear();
}

bool chooseCurrentShip(NumberOfShips* numberOfShips, Ship* currentShip)

{
    if (numberOfShips->fourPoints > 0) {
        currentShip->endingPoint = 4;
        numberOfShips->fourPoints -= 1;
        return true;
    } else if (numberOfShips->threePoints > 0) {
        currentShip->endingPoint = 3;
        numberOfShips->threePoints -= 1;
        return true;
    } else if (numberOfShips->twoPoints > 0) {
        currentShip->endingPoint = 2;
        numberOfShips->twoPoints -= 1;
        return true;
    } else if (numberOfShips->onePoints > 0) {
        currentShip->endingPoint = 1;
        numberOfShips->onePoints -= 1;
        return true;
    }

    return false;
}

int main(void)
{
    // library init
    system_init();
    navswitch_init();
    led_init();
    tinygl_init(LOOP_RATE);
    pacer_init(LOOP_RATE);
    button_init();
    ir_uart_init();

    uint8_t opponentsMap[MAP_HEIGHT][MAP_WIDTH];

    NumberOfShips numberOfShips;

    numberOfShips.fourPoints = 2;
    numberOfShips.threePoints = 0;
    numberOfShips.twoPoints = 1;
    numberOfShips.onePoints = 0;

    led_set(LED1, 0);

    Ship currentShip;

    currentShip.startingPoint = 1;
    currentShip.otherAxis = 2;
    currentShip.direction = VERT;

    chooseCurrentShip(&numberOfShips, &currentShip);

    /* Paced loop.  */
    while (1) {
        pacer_wait();
        navswitch_update();
        button_update();

        if (button_push_event_p(BUTTON1)) {
            rotateShip(&currentShip);
        }

        printLayout();

        moveShip(&currentShip);
        printOrClearShip(&currentShip, 1);

        if (navswitch_push_event_p(NAVSWITCH_PUSH)) {
            saveShipToMap(&currentShip);
            if (!chooseCurrentShip(&numberOfShips, &currentShip)) {
                tinygl_clear();
                tinygl_update();

                waitForBothPlayers(layout, opponentsMap);

                if (opponentsMap[6][4] == 1) {
                    led_set(LED1, 1);
                }

                while (1) {
                    pacer_wait();
                }
            }
        }

        tinygl_update();
    }
}