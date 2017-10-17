#include "../fonts/font5x7_1.h"
#include "button.h"
#include "led.h"
#include "navswitch.h"
#include "pacer.h"
#include "pio.h"
#include "system.h"
#include "tinygl.h"
#include <stdio.h>
#include "ircommunication.h"

/* Define polling rate in Hz.  */
#define LOOP_RATE 300
#define LED_RATE 1
// Constants for map and display size
#define MAP_WIDTH 15
#define MAP_HEIGHT 11
#define RENDER_WIDTH 5
#define RENDER_HEIGHT 7

// 0 = placingships, 1 = players turn, 3 = player waiting TODO: use it
uint8_t gameState = 0;

// below is a simple "map", 1's indicating LED(on), 0's indicating LED(off). If
// the MAP_WIDTH and MAP_HEIGHT values  are changed, the layout array must be
// updated
uint8_t layout[MAP_HEIGHT][MAP_WIDTH] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}, // NORTH
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
    // EAST
};

uint8_t opponentsMap[MAP_HEIGHT][MAP_WIDTH];

typedef enum { NORTH = 1, SOUTH = 2, EAST = 3, WEST = 4 } direction_t;

struct opponent {
    uint8_t health;
};
typedef struct opponent opponent_t;


// defining ship size
struct ship {
    uint8_t height;
    uint8_t width;
    uint8_t direction;   // 0 = horz 1 = vert
    uint8_t* area[4][4]; // this should be allocated dynamically/ or drawPlayer
                         // breaks on null terminator
};
typedef struct ship ship_t;

struct navy {
    uint8_t noOfLargeShips;
    uint8_t noOfMediumShips;
    uint8_t noOfSmallShips;
    uint8_t health;
};
typedef struct navy navy_t;

struct position {
    uint8_t x;
    uint8_t y;
    direction_t direction;
};
typedef struct position position_t;

struct player {
    ship_t currentShip;
    position_t position;
    position_t spritePosition;
    navy_t units;
};
typedef struct player player_t;

// TODO: fix this terrible struct naming
struct map {
    // heres where the display frame is stored
    uint8_t displayArea[RENDER_HEIGHT][RENDER_WIDTH];
    player_t player;
    opponent_t opponent;
};
typedef struct map map_t;

void calculateOpponentsTotalHealth(map_t *map)
{
    uint8_t *health = map->opponent.health;

    for (uint8_t x = 0; x < MAP_HEIGHT; x++) {
        for (uint8_t y = 0; y < MAP_WIDTH; y++) {
            if (opponentsMap[x][y] == 2) {
                health = health + 1;
            }
        }
    }
}

void updateDisplayArea(map_t* map)
{
    // This function checks the state of each cell in the map, relative to the
    // player, as many cells as can be displayed  On the LED matix and stores
    // these states within the "displayArea" array, to be pushed to the led
    // matrix. "Magic" numbers here are used start the for loop at LED0, relative
    // to the player position(center)
    int start_pos_x = map->player.position.x - 3;
    int start_pos_y = map->player.position.y - 2;

    // Mapping the main map to a section the size of the display
    for (int i = 0; i < RENDER_HEIGHT && start_pos_x + i < MAP_HEIGHT; i++) {
        for (int k = 0; k < RENDER_WIDTH && start_pos_y + k < MAP_WIDTH; k++) {
            // Either send our map or opponents map to the framebuffer depending on gameState
            if (gameState == 0) {
                map->displayArea[i][k] = layout[start_pos_x + i][start_pos_y + k];
            } else {
                map->displayArea[i][k] = opponentsMap[start_pos_x + i][start_pos_y + k];
            }

        }
    }
}

void drawPlayer(map_t* map)
{
    // This function is tasked with rendering player, usually center of the
    // screen. Unless safezone is set, then we move the player in direction the
    // camera otherwise would have.

    if (gameState == 0) { // placement mode
        static uint8_t offset_x = 2;
        static uint8_t offset_y = 2;

        // start from left
        uint8_t start_pos_x = map->player.position.x - 1;
        uint8_t start_pos_y = map->player.position.y;

        // width/height of ship, essentially defining the rows/ columns the
        // sprite will consume
        int width = map->player.currentShip.height;
        int height = map->player.currentShip.width;

        for (int x = 0; x < height && start_pos_x + x < MAP_HEIGHT - 1 && start_pos_x + x >= 0; x++) {
            for (int y = 0; y < width && start_pos_y + y < MAP_WIDTH - 1 && start_pos_y + y >= 0; y++) {
                // dumb drawing, magic numbers here are to align with the
                // players position
                tinygl_point_t tmp = {y + offset_y, x + offset_x};
                tinygl_pixel_set(tmp, 1);
                // TODO: ensure that placement happens within the boarders of
                // the map here we basically store the addresses of the map
                // cells of which our ship is placed overtop. to be used later.
                map->player.currentShip.area[x][y] = &layout[start_pos_x + x][start_pos_y + y];
            }
        }
        updateDisplayArea(map);
    }

    else if (gameState == 1) {

        static uint8_t offset_x = 2;
        static uint8_t offset_y = 1;

        for (uint8_t x = 0; x < 3; x++) {
            tinygl_point_t tmp = {offset_y + 1, x + offset_x};
            tinygl_pixel_set(tmp, 1);
        }
        for (uint8_t y = 0; y < 3; y++) {
            tinygl_point_t tmp = {y + offset_y, offset_x + 1};
            tinygl_pixel_set(tmp, 1);
        }
    }
}

void framebuffer(map_t* map)
{
    // Here we take the frame stored by the updateDisplayArea function and push
    // it to the LED matrix
    for (int i = 0; i < RENDER_HEIGHT; i++) {
        for (int k = 0; k < RENDER_WIDTH; k++) {
            tinygl_point_t tmp = {k, i};
            tinygl_pixel_set(tmp, map->displayArea[i][k]);
        }
    }
}

void placeShip(map_t* map)
{
    // on navswtich press, use the pointers stored in currentShip.area to modify
    // the map

    uint8_t width = map->player.currentShip.height;
    uint8_t height = map->player.currentShip.width;

    for (int i = 0; i < height; i++) {
        for (int k = 0; k < width; k++) {
            int* ptr = map->player.currentShip.area[i][k];
            *ptr = 2; // this is the value ulimately drawn on the map, should be
                      // a "2"
        }
    }
    shipSelection(map);
    // refresh the screen immediately
    updateDisplayArea(map);
    framebuffer(map);
    drawPlayer(map);
}

void rotateShip(map_t* map)
{
    // Simply swap currentShip.height and currentShip.width
    uint8_t tmp = map->player.currentShip.width;
    map->player.currentShip.width = map->player.currentShip.height;
    map->player.currentShip.height = tmp;

    // refresh the screen immediately
    updateDisplayArea(map);
    framebuffer(map);
    drawPlayer(map);
}

void playerFire(map_t *map) {


    if (opponentsMap[map->player.position.x][map->player.position.y] == 2) {
        opponentsMap[map->player.position.x][map->player.position.y] = 3;
        map->opponent.health--;
    }
}

void movePlayer(map_t* map)
{
    // This function moves the player in the direction of the navswtich, really
    // we're manipulating the "camera"

    if (navswitch_push_event_p(NAVSWITCH_WEST) && map->player.position.y > 0) {
        map->player.position.y--;
        updateDisplayArea(map);

        // Make sure to call "drawPlayer" after "updateMap", else the player
        // sprite is clobbered with map state
        framebuffer(map);
        drawPlayer(map);
    }

    if (navswitch_push_event_p(NAVSWITCH_EAST) && map->player.position.y < MAP_WIDTH - 1) {
        map->player.position.y++;
        updateDisplayArea(map);

        framebuffer(map);
        drawPlayer(map);
    }

    if (navswitch_push_event_p(NAVSWITCH_NORTH) && map->player.position.x > 0) {
        map->player.position.x--;
        updateDisplayArea(map);

        framebuffer(map);
        drawPlayer(map);
    }

    if (navswitch_push_event_p(NAVSWITCH_SOUTH) && map->player.position.x < MAP_HEIGHT - 1) {
        map->player.position.x++;
        updateDisplayArea(map);

        framebuffer(map);
        drawPlayer(map);
    }

    // if gameState = 0 AND navswitch is pressed in, confirm ship position
    if (navswitch_push_event_p(NAVSWITCH_PUSH) && gameState == 0) {
        placeShip(map);
    }

    // if gameState = 1 AND navswitch is pressed in, confirm attack
    if (navswitch_push_event_p(NAVSWITCH_PUSH) && gameState == 1) {
        placeShip(map);
    }

    // if button(near IR) is pressed, rotate the currentShip
    if (button_push_event_p(0)) {
        rotateShip(map);
    }
}

void shipSelection(map_t* map)
{
    //int *health = map->player.units.health;
    //int *ship = map->player.currentShip;

    // move down the conditionals until all units are placed
    if (map->player.units.noOfLargeShips > 0) {
        map->player.units.noOfLargeShips--;
        map->player.currentShip.height = 2;
        map->player.currentShip.width = 4;
        //health = health + (ship.height * ship.width);
    }

    else if (map->player.units.noOfMediumShips > 0) {
        map->player.units.noOfMediumShips--;
        map->player.currentShip.height = 1;
        map->player.currentShip.width = 3;
        //health = health + (ship.height * ship.width);
    }

    else if (map->player.units.noOfSmallShips > 0) {
        map->player.units.noOfSmallShips--;
        map->player.currentShip.height = 1;
        map->player.currentShip.width = 2;
        //health = health + (ship.height * ship.width);
    }
    // At this point all ships are placed, start game
    else {
        intermission(map, layout, opponentsMap);
    }
}

// TODO: below is a skeleton for a possible task.h implementation
/*static void display_task (map_t *map)
{
    tinygl_update();
}

static void send_task (map_t *map)
{
    //sendData
}

static void recieve_task (map_t *map)
{
    //getData
}*/

void intermission(map_t *map, uint8_t layout, uint8_t opponentsMap) {
    waitForBothPlayers(layout, opponentsMap);
    calculateOpponentsTotalHealth(map);
}

int main(void)
{
    // TODO: cleanup
    //int tick = 0;
    uint8_t ledStatus;

    map_t map;
    map.player.position.x = 4; // players initial position
    map.player.position.y = 6; //

    // ship quantities
    map.player.units.noOfLargeShips = 2;
    map.player.units.noOfMediumShips = 2;
    map.player.units.noOfSmallShips = 3;

    shipSelection(&map);
    updateDisplayArea(&map);

    ledStatus = 0;

    // library init
    system_init();
    navswitch_init();
    led_init();
    button_init();
    tinygl_init(LOOP_RATE);
    drawPlayer(&map);
    pacer_init(LOOP_RATE);
    ir_uart_init();

    led_set(LED1, 0);

    /* Paced loop.  */
    while (1) {
        pacer_wait();
        navswitch_update();
        button_update();

        // Keeps blue led cycling at 1 second intervals, useful for debugging.
        // //Moved into movePlayer to debug safezone
        /*        if (tick >= LOOP_RATE) {
                    if (ledStatus == 1) {
                        led_set(LED1, 0);
                        ledStatus = 0;
                    } else {
                        led_set(LED1, 1);
                        ledStatus = 1;
                    }
                    tick = 0;
                }
                tick++;*/
        movePlayer(&map);
        tinygl_update();
    }
}