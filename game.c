#include "../fonts/font5x7_1.h"
#include "led.h"
#include "navswitch.h"
#include "pacer.h"
#include "pio.h"
#include "system.h"
#include "tinygl.h"

/* Define polling rate in Hz.  */
#define LOOP_RATE 300
#define LED_RATE 1
// Constants for map and display size
#define MAP_WIDTH 15
#define MAP_HEIGHT 11
#define RENDER_WIDTH 5
#define RENDER_HEIGHT 7

// below is a simple "map", 1's indicating LED(on), 0's indicating LED(off). If
// the MAP_WIDTH and MAP_HEIGHT values  are changed, the layout array must be
// updated
int layout[MAP_HEIGHT][MAP_WIDTH] = {
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
typedef enum { NORTH=1, SOUTH=2, EAST=3, WEST=4 } direction_t;

//defining ship size
const int largeShip[2][5] = {
        {2, 2, 2, 2, 2},
        {2, 2, 2, 2, 2}
};

const int mediumShip[1][4] = {
        {2, 2, 2, 2}
};

const int smallShip[1][2] = {
        {2, 2}
};

struct navy {
    int noOfLargeShips;
    int noOfMediumShips;
    int noOfSmallShips;
};
typedef  struct navy navy_t;

struct position {
    int x;
    int y;
    direction_t direction;
};
typedef struct position position_t;

struct safezone // When the player position is at "x3 - x11 or y2 - y8", dont
                // pan camera, move sprite instead
{
    position_t position;
};
typedef struct safezone safezone_t;

struct player {
    int **sprite;
    int spriteRotation;
    position_t position;
    position_t spritePosition;
    safezone_t safezone;
    navy_t units;
};
typedef struct player player_t;

// TODO: fix this terrible struct naming
struct map {
    // heres where the display frame is stored
    int displayArea[RENDER_HEIGHT][RENDER_WIDTH];
    int **spriteArea;
    player_t player;
};
typedef struct map map_t;

void updateDisplayArea(map_t* map)
{
    // This function checks the state of each cell in the map, relative to the
    // player, as many cells as can be displayed  On the LED matix and stores
    // these states within the "displayArea" array, to be pushed to the led
    // matrix. "Magic" numbers here are used start the for loop at LED0,
    // relative
    // to the player position(center)
    int start_pos_x = map->player.position.x - 3;
    int start_pos_y = map->player.position.y - 2;

    // Mapping the main map to a section the size of the display
    for (int i = 0; i < RENDER_HEIGHT && start_pos_x + i < MAP_HEIGHT; i++) {
        for (int k = 0; k < RENDER_WIDTH && start_pos_y + k < MAP_WIDTH; k++) {
            // TODO: maybe modify this section to pass a tinygl_coord_t type
            // instead
            map->displayArea[i][k] = layout[start_pos_x + i][start_pos_y + k];
        }
    }
}

void drawPlayer(map_t* map)
{
    // This function is tasked with rendering player, usually center of the
    // screen. Unless safezone is set, then we  move the player in direction the
    // camera otherwise would have.

    if (map->player.safezone.position.x || map->player.safezone.position.y) {
        //TODO: curser not travelling in proper direction
        tinygl_point_t pos = {map->player.position.x, map->player.position.y};
        tinygl_pixel_set(pos, 1);
    } else {
        //"else if we're not in the save zone, just draw the curser in the center"
        int k = 3;
        int i = 2;
        tinygl_point_t pos = {i, k};
        tinygl_pixel_set(pos, 1);
    }
    // After we have drawn the simple curser, then we add the sprite "around" it, so its position is such that
    // the curser is centered
    // TODO: render sprite
    int spriteHeight = sizeof map->player.sprite[0];
    int spriteWidth = sizeof map->player.sprite[0][0];
    int start_pos_x = map->player.position.x - 3;
    int start_pos_y = map->player.position.y;

    for (int i = 0; i < spriteHeight && start_pos_x + i < spriteHeight; i++) {
        for (int k = 0; k < spriteWidth && start_pos_y + k < spriteWidth; k++) {

            map->spriteArea[i][k] = layout[start_pos_x + i][start_pos_y + k];
        }
    }
}

void updateMap(map_t* map)
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

void movePlayer(map_t *map) {
    //TODO: finish implementing safezone movement
    //This function moves the player in the direction of the navswtich, really we're manipulating the "camera",
    //unless the players movement would move the camera outside the map area, in which case, set the "safezone"
    //causing the player to move, instead of the camera

    updateDisplayArea(map);

    if (navswitch_push_event_p(NAVSWITCH_WEST) && map->player.position.y > 0) {
        map->player.position.y--;
        map->player.position.direction = WEST;

        if (map->player.position.y <= 2 || map->player.position.y >= 8) {
            map->player.safezone.position.y = 1;
            led_set(LED1, 1);

            updateMap(map);
            drawPlayer(map);

        } else {
            map->player.safezone.position.y = 0;
            led_set(LED1, 0);

            updateMap(map);
            drawPlayer(map);
        }
    }

    if (navswitch_push_event_p(NAVSWITCH_EAST) && map->player.position.y < MAP_WIDTH - 1) {
        map->player.position.y++;
        map->player.position.direction = EAST;

        if (map->player.position.y <= 2 || map->player.position.y >= 8) {
            map->player.safezone.position.y = 1;
            led_set(LED1, 1);

            updateMap(map);
            drawPlayer(map);
        } else {
            map->player.safezone.position.y = 0;
            led_set(LED1, 0);

            updateMap(map);
            drawPlayer(map);
        }
    }

    if (navswitch_push_event_p(NAVSWITCH_NORTH) && map->player.position.x > 0) {
        map->player.position.x--;
        map->player.position.direction = NORTH;

        if (map->player.position.x <= 3 || map->player.position.x >= 11) {
            map->player.safezone.position.y = 1;
            led_set(LED1, 1);

            updateMap(map);
            drawPlayer(map);
        } else {
            map->player.safezone.position.y = 0;
            led_set(LED1, 0);

            updateMap(map);
            drawPlayer(map);
        }
    }

    if (navswitch_push_event_p(NAVSWITCH_SOUTH) && map->player.position.x < MAP_HEIGHT - 1) {
        map->player.position.x++;
        map->player.position.direction = SOUTH;

        if (map->player.position.x <= 3 || map->player.position.x >= 11) {
            map->player.safezone.position.y = 1;
            led_set(LED1, 1);

            updateMap(map);
            drawPlayer(map);
        } else {
            map->player.safezone.position.y = 0;
            led_set(LED1, 0);

            updateMap(map);
            drawPlayer(map);
        }
    }
}

int shipSelection(map_t *map) {
    //move down the conditionals until all units are placed
    if (map->player.units.noOfLargeShips > 0) {
        map->player.units.noOfLargeShips - 1;
        return &largeShip;
    }

    else if (map->player.units.noOfMediumShips > 0) {
        map->player.units.noOfMediumShips - 1;
        return &mediumShip;
    }

    else {
        map->player.units.noOfSmallShips - 1;
        return &smallShip;
    }
}

// TODO: New function to add the spriteArea array into the parent "layout" array when a ship is in a VALID position AND the navswitch is pressed

int main (void)
{
    //TODO: cleanup
    int tick = 0;
    int ledStatus;

    map_t map;
    map.player.position.x = 6; //players initial position
    map.player.position.y = 7; //
    map.player.position.direction = NORTH;

    map.player.sprite = shipSelection(&map);
    updateDisplayArea(&map);
    led_set(LED1, 0);
    ledStatus = 0;

    //library init
    system_init ();
    navswitch_init();
    led_init();
    tinygl_init(LOOP_RATE);
    drawPlayer(&map);
    pacer_init(LOOP_RATE);

    /* Paced loop.  */
    while (1) {
        pacer_wait();
        navswitch_update();

        //Keeps blue led cycling at 1 second intervals, useful for debugging. //Moved into movePlayer to debug safezone
/*        if (tick >= LOOP_RATE) {
            if (ledStatus == 1) {
                led_set(LED1, 0);
                ledStatus = 0;
            } else {
                led_set(LED1, 1);
                ledStatus = 1;
            }
            tick = 0;
        }*/

        tick = tick + 1;
        movePlayer(&map);
        tinygl_update ();

    }
}