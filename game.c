/** @file game.c
 * @author Rafael Goesmann Joshua Aitken
 * @date 11th October 2017
 * @brief An interactive battleships game
 *
 **/

#include "../fonts/font5x7_1.h"
#include "button.h"
#include "ir_uart.h"
#include "ircommunication.h"
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

typedef struct cursor_s {
    uint8_t x;
    uint8_t y;
} Cursor;

uint8_t layout[MAP_HEIGHT][MAP_WIDTH] = {
    {0, 0, 0, 0, 0},

    {0, 0, 0, 0, 0},

    {0, 0, 0, 0, 0},

    {0, 0, 0, 0, 0},

    {0, 0, 0, 0, 0},

    {0, 0, 0, 0, 0},

    {0, 0, 0, 0, 0},
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

void clearMap(void)
{
    for (int i = 0; i < MAP_HEIGHT; i++) {
        for (int j = 0; j < MAP_WIDTH; j++) {
            layout[i][j] = 0;
        }
    }
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

void renderCursor(Cursor* cursor, bool isFull)
{

    tinygl_point_t p = {cursor->x, cursor->y};
    tinygl_draw_point(p, isFull);
}

void moveAndClickCursor(Cursor* cursor,
                        uint8_t opponentsMap[MAP_HEIGHT][MAP_WIDTH],
                        bool* userDone)
{

    if (navswitch_push_event_p(NAVSWITCH_PUSH) && *userDone == false) {
        if (opponentsMap[cursor->y][cursor->x]) {
            layout[cursor->y][cursor->x] = 1;
            *userDone = true;
            userDoneWithRound();
            led_set(LED1, 0);
        } else {
            *userDone = true;
            userDoneWithRound();
            led_set(LED1, 0);
        }
    }

    if (navswitch_push_event_p(NAVSWITCH_NORTH) && cursor->y > 0) {
        renderCursor(cursor, 0);
        cursor->y -= 1;
        renderCursor(cursor, 1);

    } else if (navswitch_push_event_p(NAVSWITCH_EAST) && cursor->x < 4) {
        renderCursor(cursor, 0);
        cursor->x += 1;
        renderCursor(cursor, 1);

    } else if (navswitch_push_event_p(NAVSWITCH_SOUTH) && cursor->y < 6) {
        renderCursor(cursor, 0);
        cursor->y += 1;
        renderCursor(cursor, 1);

    } else if (navswitch_push_event_p(NAVSWITCH_WEST) && cursor->x > 0) {
        renderCursor(cursor, 0);
        cursor->x -= 1;
        renderCursor(cursor, 1);
    }
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

    Cursor cursor = {2, 2};

    uint8_t gameState = 0;
    uint8_t opponentsMap[MAP_HEIGHT][MAP_WIDTH];

    bool userDone = false;

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

        if (gameState == 0) {
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

                    clearMap();

                    if (opponentsMap[0][0] == 1) {
                        led_set(LED1, 1);
                    }

                    gameState += 1;
                    renderCursor(&cursor, 1);
                }
            }
        } else if (gameState == 1) {

            moveAndClickCursor(&cursor, opponentsMap, &userDone);

            for (uint8_t i = 0; i < MAP_WIDTH; i++) {
                for (uint8_t j = 0; j < MAP_HEIGHT; j++) {
                    if (layout[j][i] == 1) {
                        tinygl_point_t p = {i, j};
                        tinygl_draw_point(p, 1);
                    }
                }
            }

            if (isUserDoneWithRound()) {
                led_set(LED1, 1);
                userDone = false;
            }
        }

        tinygl_update();
    }
}
