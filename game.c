#include "../fonts/font5x7_1.h"
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
#define MAP_WIDTH 15
#define MAP_HEIGHT 11
#define RENDER_WIDTH 5
#define RENDER_HEIGHT 7


//0 = placingships, 1 = players turn, 3 = player waiting TODO: use it
//uint8_t gameState = 0;

// below is a simple "map", 1's indicating LED(on), 0's indicating LED(off). If
// the MAP_WIDTH and MAP_HEIGHT values  are changed, the layout array must be
// updated
static uint8_t layout[MAP_HEIGHT][MAP_WIDTH] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
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
struct ship  {
    uint8_t height;
    uint8_t width;
    uint8_t direction; // 0 = horz 1 = vert
    uint8_t **area;
};
typedef struct ship ship_t;

struct navy {
    uint8_t noOfLargeShips;
    uint8_t noOfMediumShips;
    uint8_t noOfSmallShips;
};
typedef  struct navy navy_t;

struct position {
    uint8_t x;
    uint8_t y;
    direction_t direction;
};
typedef struct position position_t;

/*struct safezone // When the player position is at "x3 - x11 or y2 - y8", dont
                // pan camera, move sprite instead
{
    position_t position;
};
typedef struct safezone safezone_t;*/

struct player {
    ship_t currentShip;
    int spriteRotation;
    position_t position;
    position_t spritePosition;
    //safezone_t safezone;
    navy_t units;
};
typedef struct player player_t;

// TODO: fix this terrible struct naming
struct map {
    // heres where the display frame is stored
    uint8_t displayArea[RENDER_HEIGHT][RENDER_WIDTH];
    int spritePos;
    player_t player;
};
typedef struct map map_t;

void updateDisplayArea(map_t *map)
{
    //This function checks the state of each cell in the map, relative to the player, as many cells as can be displayed
    //On the LED matix and stores these states within the "displayArea" array, to be pushed to the led matrix.
    //"Magic" numbers here are used start the for loop at LED0, relative to the player position(center)
    int start_pos_x = map->player.position.x - 3;
    int start_pos_y = map->player.position.y - 2;

    //Mapping the main map to a section the size of the display
    for (int i = 0; i < RENDER_HEIGHT && start_pos_x + i < MAP_HEIGHT; i++) {
        for (int k = 0; k < RENDER_WIDTH && start_pos_y + k < MAP_WIDTH; k++) {
            //TODO: maybe modify this section to pass a tinygl_coord_t type instead
            map->displayArea[i][k] = layout[start_pos_x + i][start_pos_y + k];
        }
    }
}

void drawPlayer(map_t *map)
{
    //This function is tasked with rendering player, usually center of the screen. Unless safezone is set, then we
    //move the player in direction the camera otherwise would have.
    //if (map->player.safezone.position.x || map->player.safezone.position.y) {
    //tinygl_pixel_set({map->player.position.x, map->player.position.y}, 0);

    //start from left hand led
    int start_pos_x = map->player.position.x - 1;
    int start_pos_y = map->player.position.y - 2;
    //width of ship
    int width = map->player.currentShip.height;
    int height = map->player.currentShip.width;

    for (int i = 0; i < height; i++) {
        for (int k = 0; k < width; k++ ) {
            tinygl_point_t tmp = {1 + i, 2 + k};
            tinygl_pixel_set(tmp, 1);
            //TODO: out of bounds issues, allocate and store ship area better
            //map->player.currentShip.area[start_pos_x + i][start_pos_y + k] = 1;

        }
    }
}

void updateMap(map_t *map)
{
    //Here we take the frame stored by the updateDisplayArea function and push it to the LED matrix
    for (int i = 0; i < RENDER_HEIGHT; i++) {
        for (int k = 0; k < RENDER_WIDTH; k++) {
            tinygl_point_t tmp = {k,i};
            tinygl_pixel_set(tmp, map->displayArea[i][k]);
        }
    }
}

void movePlayer(map_t *map)
{
    //This function moves the player in the direction of the navswtich, really we're manipulating the "camera"

    if (navswitch_push_event_p (NAVSWITCH_WEST) && map->player.position.y > 0) {
        map->player.position.y--;
        updateDisplayArea(map);

        //Make sure to call "drawPlayer" after "updateMap", else the player sprite is clobbered with map state
        updateMap(map);
        drawPlayer(map);
    }

    if (navswitch_push_event_p (NAVSWITCH_EAST) && map->player.position.y < MAP_WIDTH - 1) {
        map->player.position.y++;
        updateDisplayArea(map);

        updateMap(map);
        drawPlayer(map);
    }

    if (navswitch_push_event_p (NAVSWITCH_NORTH) && map->player.position.x > 0) {
        map->player.position.x--;
        updateDisplayArea(map);

        updateMap(map);
        drawPlayer(map);
    }

    if (navswitch_push_event_p (NAVSWITCH_SOUTH) && map->player.position.x < MAP_HEIGHT - 1) {
        map->player.position.x++;
        updateDisplayArea(map);

        updateMap(map);
        drawPlayer(map);
    }

}

void shipSelection(map_t *map) {
    //move down the conditionals until all units are placed
    if (map->player.units.noOfLargeShips > 0) {
        map->player.units.noOfLargeShips - 1;
        map->player.currentShip.height = 2;
        map->player.currentShip.width = 4;
        //improve this
        //&map->player.currentShip.area[2][4];
    }

    else if (map->player.units.noOfMediumShips > 0) {
        map->player.units.noOfMediumShips - 1;
        map->player.currentShip.height = 1;
        map->player.currentShip.width = 3;
        //&map->player.currentShip.area[1][3];
    }

    else {
        map->player.units.noOfSmallShips - 1;
        map->player.currentShip.height = 1;
        map->player.currentShip.width = 2;
        //&map->player.currentShip.area[1][2];
    }
}

//TODO: below is a skeleton for a possible task.h implementation
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

// TODO: New function to add the spriteArea array into the parent "layout" array when a ship is in a VALID position AND the navswitch is pressed



int main (void)
{
    //TODO: cleanup
    int tick = 0;
    int ledStatus;

    map_t map;
    map.player.position.x = 4; //players initial position
    map.player.position.y = 6; //

    //ship quantities
    map.player.units.noOfLargeShips = 2;
    map.player.units.noOfMediumShips = 3;
    map.player.units.noOfSmallShips = 3;

    shipSelection(&map);
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
        if (tick >= LOOP_RATE) {
            if (ledStatus == 1) {
                led_set(LED1, 0);
                ledStatus = 0;
            } else {
                led_set(LED1, 1);
                ledStatus = 1;
            }
            tick = 0;
        }
        tick++;
        movePlayer(&map);
        tinygl_update ();

    }
}