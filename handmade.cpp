#include "handmade.h"

void gameUpdateAndRender(Game_offscreen_buffer *gameBuffer) {
    gameBuffer->data = (uint32 *)malloc(gameBuffer->width*gameBuffer->height*4); // 4 bytes for a pixel RR GG BB XX
    uint32 *ptr = gameBuffer->data;
    for (int y = 0; y < gameBuffer->height; ++y) {

        for (int x = 0; x < gameBuffer->width; ++x) {
            uint8 blue = (uint8)(x + gameBuffer->XOffset);
            uint8 green = (uint8)(y + gameBuffer->YOffset);
            *ptr++ = (green << 8) | blue;
        }
    }
}
