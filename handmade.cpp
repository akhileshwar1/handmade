#include "handmade.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

void renderweirdgradient(Game_offscreen_buffer *gameBuffer, Game_state *state) {
    gameBuffer->data = (uint32 *)malloc(gameBuffer->width*gameBuffer->height*4); // 4 bytes for a pixel RR GG BB XX
    uint32 *ptr = gameBuffer->data;
    for (int y = 0; y < gameBuffer->height; ++y) {

        for (int x = 0; x < gameBuffer->width; ++x) {
            uint8 blue = (uint8)(x + state->XOffset);
            uint8 green = (uint8)(y + state->YOffset);
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

void *DEBUGPlatformReadEntireFile(char *filename) {
    int fd = open(filename, O_RDWR | O_APPEND);
    if (fd < 0) {
        return NULL;
    }
    struct stat sb;
    uint32 fileSize;
    if (stat(filename, &sb) < 0) {
        return NULL;
    }

    Assert(sb.st_size < 0xffffffff) // my file size will be less than max(uint32)
    fileSize = sb.st_size;
    void *BitmapMemory = malloc(fileSize);
    ssize_t bytesRead = read(fd, BitmapMemory, fileSize);
    return BitmapMemory;
}

void DEBUGPlatformFreeFileMemory(void *BitmapMemory) {
    free(BitmapMemory);
}

void gameUpdateAndRender(Game_offscreen_buffer *gameBuffer,
                         Game_sound_buffer *gameSoundBuffer,
                         Game_input *input,
                         Game_memory *memory) {
    Game_state *state = (Game_state *)memory->permanentStorage;
    char *filename = "build.sh";
    void *BitmapMemory = DEBUGPlatformReadEntireFile(filename);
    if(BitmapMemory) {
        DEBUGPlatformFreeFileMemory(BitmapMemory);
    }

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
    writeSound(gameSoundBuffer);
}
