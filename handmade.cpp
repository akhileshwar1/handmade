#include "handmade.h"

void gameUpdateAndRender(Offscreen_buffer *buffer) {
    buffer->data = (uint32 *)malloc(buffer->width*buffer->height*4); // 4 bytes for a pixel RR GG BB XX
    uint32 *ptr = buffer->data;
    for (int y = 0; y < buffer->height; ++y) {

        for (int x = 0; x < buffer->width; ++x) {
            uint8 blue = (uint8)(x + buffer->XOffset);
            uint8 green = (uint8)(y + buffer->YOffset);
            *ptr++ = (green << 8) | blue;
        }
    }
}
