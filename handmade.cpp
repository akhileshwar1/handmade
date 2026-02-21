#include "handmade.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>

void renderweirdgradient(Game_offscreen_buffer *gameBuffer, Game_state *state) {
    gameBuffer->data = (uint32 *)malloc(gameBuffer->width*gameBuffer->height*4); // 4 bytes for a pixel RR GG BB XX
    uint32 *ptr = gameBuffer->data;
    for (int y = 0; y < gameBuffer->height; ++y) {

        for (int x = 0; x < gameBuffer->width; ++x) {
            uint8 blue = (uint8)(x + state->XOffset);
            uint8 green = (uint8)(y + state->YOffset);
            *ptr++ = (green << 16) | blue;
        }
    }
}

void writeSound(Game_sound_buffer *gameSoundBuffer) {
    int16 *sample_ptr = gameSoundBuffer->samples;
    for (uint32 i = 0; i < gameSoundBuffer->framesToWrite; i++) {
        int16 sample_value = (int16)(gameSoundBuffer->amplitude *
                                     sinf(gameSoundBuffer->phase));
        *sample_ptr++ = sample_value; // left
        *sample_ptr++ = sample_value; // right

        gameSoundBuffer->phase += gameSoundBuffer->phase_increment;

        if (gameSoundBuffer->phase >= 2.0f * M_PI) {
            gameSoundBuffer->phase -= 2.0f * M_PI;
        }

    }

}

extern "C"
GET_SOUND_SAMPLES(getSoundSamplesMain) {
    writeSound(gameSoundBuffer);
}

extern "C"
GAME_UPDATE_AND_RENDER(gameUpdateAndRenderMain) {
    Game_state *state = (Game_state *)memory->permanentStorage;
    // char *filename = "random_notes.md";
    // void *BitmapMemory = memory->readEntireFile(filename);
    // if(BitmapMemory) {
    //     memory->freeFileMemory(BitmapMemory);
    // }

    if (input->wWasPressed) {
       state->YOffset++;
    } else if (input->aWasPressed) {
        state->XOffset--;
    } else if (input->sWasPressed) {
        state->YOffset--;
    } else if (input->dWasPressed) {
        state->XOffset++;
    }
    state->XOffset++;
    renderweirdgradient(gameBuffer, state);
}
