#include "handmade.h"

void renderweirdgradient(Game_offscreen_buffer *gameBuffer) {
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

void writeSound(Game_sound_buffer *gameSoundBuffer) {
    int16 *sample_ptr = gameSoundBuffer->samples;
    for (uint32 i = 0; i < gameSoundBuffer->frames; i++) {
        int16 sample_value = (int16)(gameSoundBuffer->amplitude * sinf(gameSoundBuffer->phase));
        *sample_ptr++ = sample_value; // left
        *sample_ptr++ = sample_value; // right

        gameSoundBuffer->phase += gameSoundBuffer->phase_increment;

        if (gameSoundBuffer->phase >= 2.0f * M_PI) {
            gameSoundBuffer->phase -= 2.0f * M_PI;
        }

    }

}

void gameUpdateAndRender(Game_offscreen_buffer *gameBuffer,
                         Game_sound_buffer *gameSoundBuffer,
                         Game_input *input) {
    if (input->wWasPressed) {
        gameBuffer->YOffset++;
    } else if (input->aWasPressed) {
        gameBuffer->XOffset--;
    } else if (input->sWasPressed) {
        gameBuffer->YOffset--;
    } else if (input->dWasPressed) {
        gameBuffer->XOffset++;
    }
    gameBuffer->XOffset++;
    renderweirdgradient(gameBuffer);
    writeSound(gameSoundBuffer);
}
