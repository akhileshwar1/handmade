#ifndef HANDMADE_H
#define HANDMADE_H

/*
 Services that the game provides to the platform layer.
*/
typedef struct {
    uint32 *data;
    int width;
    int height;
} Game_offscreen_buffer;

typedef struct {
    int XOffset;
    int YOffset; 
} Game_state;

typedef struct {
    uint64 permanentStorageSize;
    void *permanentStorage;
    uint64 transientStorageSize;
    void *transientStorage;
} Game_memory;

typedef struct {
    uint32 sample_rate; // we can't produce a continous wave, so we snapshot aka sample it.
    uint32 frames;
    int16 *samples;
    real32 amplitude;
    real32 frequency; // full cycles per second.
    real32 phase;
    real32 phase_increment;
} Game_sound_buffer;

typedef struct {
    bool wWasPressed;
    bool aWasPressed;
    bool sWasPressed;
    bool dWasPressed;
} Game_input;

void gameUpdateAndRender(Game_offscreen_buffer *gameBuffer, Game_sound_buffer *gameSoundBuffer,
                         Game_input *input);

/*
 Services the platform layer provides to the game play.
*/
#endif
