#ifndef HANDMADE_H
#define HANDMADE_H

/*
 Services that the game provides to the platform layer.
*/
typedef struct {
    uint32 *data;
    int XOffset;
    int YOffset;
    int width;
    int height;
} Offscreen_buffer;

void gameUpdateAndRender(Offscreen_buffer *buffer);

/*
 Services the platform layer provides to the game play.
*/
#endif
