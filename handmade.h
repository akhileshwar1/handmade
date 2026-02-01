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
} Game_offscreen_buffer;

void gameUpdateAndRender(Game_offscreen_buffer *gameBuffer);

/*
 Services the platform layer provides to the game play.
*/
#endif
