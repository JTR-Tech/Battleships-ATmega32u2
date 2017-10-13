#include "../fonts/font5x7_1.h"
#include "ir_uart.h"
#include "ircommunication.h"
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
const int layout[MAP_HEIGHT][MAP_WIDTH] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, // NORTH
    {1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
    // EAST
};

// 1 Marks a border, 0 marks unmarked territory, "h" marks hit, "m" marks miss
const int yourLayout[MAP_HEIGHT][MAP_WIDTH] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, // NORTH
    {1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
    // EAST
};

// 1 Marks a border, 0 marks unmarked territory, "h" marks hit, "m" marks miss
const int opponentsLayout[MAP_HEIGHT][MAP_WIDTH] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, // NORTH
    {1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
    // EAST
};

typedef enum { NORTH, EAST, SOUTH, WEST } direction_t;

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
    position_t position;
    position_t spritePosition;
    safezone_t safezone;
};
typedef struct player player_t;

// TODO: fix this terrible struct naming
struct map {
    // heres where the display frame is stored
    int displayArea[RENDER_HEIGHT][RENDER_WIDTH];
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
    // camera otherwise would have.  if (map->player.safezone.position.x ||
    // map->player.safezone.position.y) {
    // tinygl_pixel_set({map->player.position.x, map->player.position.y}, 0);
    // TODO: finish implementing safezone movement
    // if (map->player.position.direction == WEST) {

    //}
    //}
    // TODO: create a proper sprite
    int k = 3;
    int i = 2;
    tinygl_point_t pos = {i, k};
    tinygl_pixel_set(pos, 1);
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

void movePlayer(map_t* map)
{
    // TODO: finish implementing safezone movement
    // This function moves the player in the direction of the navswtich, really
    // we're manipulating the "camera",  unless the players movement would move
    // the camera outside the map area, in which case, set the "safezone"
    // causing the player to move, instead of the camera

    updateDisplayArea(map);

    if (navswitch_push_event_p(NAVSWITCH_WEST) && map->player.position.y > 0) {
        map->player.position.y--;
        map->player.position.direction = WEST;

        if (map->player.position.y <= 2 || map->player.position.y <= 8) {
            map->player.safezone.position.y = 1;
        } else {
            map->player.safezone.position.y = 0;
        }
        // Make sure to call "drawPlayer" after "updateMap", else the player
        // sprite is clobbered with map state
        updateMap(map);
        drawPlayer(map);
    }

    if (navswitch_push_event_p(NAVSWITCH_EAST) &&
        map->player.position.y < MAP_WIDTH - 1) {
        map->player.position.y++;
        map->player.position.direction = EAST;

        if (map->player.position.y <= 2 || map->player.position.y <= 8) {
            map->player.safezone.position.y = 1;
        } else {
            map->player.safezone.position.y = 0;
        }

        updateMap(map);
        drawPlayer(map);
    }

    if (navswitch_push_event_p(NAVSWITCH_NORTH) && map->player.position.x > 0) {
        map->player.position.x--;
        map->player.position.direction = NORTH;

        if (map->player.position.x <= 3 || map->player.position.x <= 11) {
            map->player.safezone.position.x = 1;
        } else {
            map->player.safezone.position.x = 0;
        }

        updateMap(map);
        drawPlayer(map);
    }

    if (navswitch_push_event_p(NAVSWITCH_SOUTH) &&
        map->player.position.x < MAP_HEIGHT - 1) {
        map->player.position.x++;
        map->player.position.direction = SOUTH;

        if (map->player.position.x <= 3 || map->player.position.x <= 11) {
            map->player.safezone.position.x = 1;
        } else {
            map->player.safezone.position.x = 0;
        }

        updateMap(map);
        drawPlayer(map);
    }
}

void* mapInit()
{
    // create the map outside the main() scope as to not interfer with tinygl's
    // text. Returns a pointer
    map_t* map;
    map->player.position.x = 4; // players initial position
    map->player.position.y = 6; //
    map->player.position.direction = NORTH;
    return map;
}

int main(void)
{
    system_init();
    led_init();
    navswitch_init();
    ir_uart_init();
    pacer_init(LOOP_RATE);


    tinygl_init(LOOP_RATE);
    tinygl_font_set(&font5x7_1);
    tinygl_text_speed_set(20);
    tinygl_text_mode_set(TINYGL_TEXT_MODE_SCROLL);


    led_set(LED1, 0);


    uint8_t isYourTurn = waitForBothPlayers();
    
    led_set(LED1, 1);

    if (isYourTurn == 1) {
        //Do init stuff for loading empty background
        tinygl_point_t p = {1, 1};
        tinygl_draw_point(p, 1);
    } else {
        //display tinygl text
        tinygl_point_t p = {2, 2};
        tinygl_draw_point(p, 1);
        
    }
    
    //Emulates a battleships game without grahpics (push command for end of round)
    while (1) {
        pacer_wait();
        tinygl_update();
        navswitch_update();

        if (isYourTurn == 1) {
            if (navswitch_push_event_p(NAVSWITCH_PUSH)) {
              userDoneWithRound();  
              tinygl_clear();
              tinygl_point_t p = {2, 2};
              tinygl_draw_point(p, 1);
              isYourTurn = 0;
            }
        }
        else {
            if (isUserDoneWithRound()) {
                tinygl_clear(); 
                isYourTurn = 1;
                tinygl_point_t p = {1, 1};
                tinygl_draw_point(p, 1);
            }
        }
   }

        

    /*
    bool currentTurn = connectUsers();
    srand(time(NULL));
    */
    /*
        int num = connectUsers();

        while (1) {
            pacer_wait();
            led_set(LED1, 1);
        }



    /*

        int tick = 0;
        int ledStatus;
        int introtick = 0;
        int state = 1;

        system_init();
        navswitch_init();
        led_init();
        pacer_init(LOOP_RATE);

        led_set(LED1, 0);
        ledStatus = 0;

        while (1) {
            pacer_wait();
            navswitch_update();

            // Our pacer is set to 300HZ, so the "tick" variable will increase
    from
            // 0 - 300 in exactly one second The "introtick" variable denotes
    the
            // amount of seconds the message has displayed. Once "introtick"
    reaches
            // 10(seconds), progess the "state"
            if (tick >= LOOP_RATE) {
                if (ledStatus == 1) { // re-added the led cycle to debug
    segfaults,
                                      // helpful to know if we have reached this
                    ledStatus = 0;    // point and are still processing
                } else {
                    led_set(LED1, 1);
                    ledStatus = 1;
                }

                introtick++;
                if (introtick == 10 && state == 1) {
                    state = 2;
                    tinygl_clear();
                }
                tick = 0;
            }

            if (state == 2) {
                // I think the problem was, mapInit() calls tinygl functions
    using
                // variables decleared within the the same context as the
                // "tinygl_text", when theyre initalized, they conflict and the
                // program doesnt run. Trying to get around this, ive initialize
                // "map" in a separate function and returned a pointer to keep
    it
                // out of the main() scope. Not quite working yet, but at least
    it
                // doesnt blow up :)

                map_t* map = mapInit();
                movePlayer(&map);
            }

            tinygl_update();
            tick = tick + 1;
        }

        */
}