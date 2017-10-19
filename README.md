"Battleships" game for the UCFK - ENCE260
An interactive game of battleships with a scrolling background.

For the assignment we have created the classic battleships game for the UCFK.
We believed the limiting factor of this device to be the rather small LED matrix,
to work around this we have implemented a 2d scrolling arena using a rudimentary
framebuffer we are able achieve a much more interesting game than the UCFK would
otherwise be limited to. 

## Getting Started

To run the program, simply type ```make program``` on two funkits. Once you have done this, you can then place your battleships. Once the battleships have been placed, all led's will go off and will wait for both players to press the NAV_PUSH button. What this does is send their map to their opponent. Once both players have pressed the NAV_PUSH button, then both of you will be able to try bomb the other player once at a time until someone else wins.
