#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <alsa/asoundlib.h>
#include <math.h>
#include <time.h>
#include <x86intrin.h>
#include <dlfcn.h>
#include <sys/stat.h>

#include "handmade.h"

// we only write the next "game" frame's worth of audio
// i.e 1/60 th of a second which is the period size
// 48000 * 2 * 2 (bytes/sec) / 4800 * 2 * 2 * 6 (bytes/interrupt)
// is the interrupt time for the card i.e 1/60th 
// period size determines the interrupt time.
#define PERIOD_SIZE (4800 * 6)
typedef struct {
    XImage *image;
    int screen; 
    Visual *visual;
    int depth; 
} X_offscreen_buffer;

typedef struct {
    snd_pcm_t *pcm;
    snd_output_t *output;
    snd_pcm_status_t *status;
} X_sound_config;

typedef struct {
    bool isValid;
    gameUpdateAndRender *GameUpdateAndRender;
    getSoundSamples *GetSoundSamples;
    void *handle;
} X_game_code;

// stub
GAME_UPDATE_AND_RENDER(gameUpdateAndRenderStub) {
    printf("this is just a stub!\n");
    return;
}

// stub
GET_SOUND_SAMPLES(getSoundSamplesStub) {
    printf("this is just a stub!\n");
    return;
}


void loadGameCode(X_game_code *gameCode) {
    gameCode->handle = dlopen("./libhandmade.so", RTLD_NOW);
    if (gameCode->handle) {
        dlerror();
        gameCode->GameUpdateAndRender = (gameUpdateAndRender *)dlsym(gameCode->handle, "gameUpdateAndRenderMain");
        gameCode->GetSoundSamples = (getSoundSamples *)dlsym(gameCode->handle, "getSoundSamplesMain");
        char *err = dlerror();
        if (err != NULL) {
            printf("Error while loading, falling back to stub %s!\n", err);
            gameCode->GameUpdateAndRender = &gameUpdateAndRenderStub;
            gameCode->GetSoundSamples = &getSoundSamplesStub;
        } else {
            printf("Loaded the actual call, %s!\n", err);
            gameCode->isValid = true;
        }
    } else {
        gameCode->GameUpdateAndRender = &gameUpdateAndRenderStub;
    }
}

void unloadGameCode(X_game_code *gameCode) {
    if (gameCode->handle) {
        dlclose(gameCode->handle);
    }

    if (gameCode->isValid) {
        gameCode->GameUpdateAndRender = &gameUpdateAndRenderStub;
        gameCode->GetSoundSamples = &getSoundSamplesStub;
        gameCode->isValid = false;
    }
}

int XInitSound(X_sound_config *sound_config) {
    snd_pcm_state_t state;
    sound_config->output = NULL;
    sound_config->status = {};
    int err;
    err = snd_output_stdio_attach(&sound_config->output, stdout, 0);
    if (err < 0) {
        printf("Output failed: %s\n", snd_strerror(err));
        return 0;
    }
    int dir = 0; // basically chose a value almost equal to what was passed.
    int channels = 2;
    err = snd_pcm_open(&sound_config->pcm, "hw:0,0", SND_PCM_STREAM_PLAYBACK, 0);
    if (err < 0) {
        printf("failed at %s\n", snd_strerror(err));
    }
    state = snd_pcm_state(sound_config->pcm);
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_hw_params_alloca(&hw_params);
    snd_pcm_hw_params_any(sound_config->pcm, hw_params);
    err = snd_pcm_hw_params_set_access(sound_config->pcm, hw_params, SND_PCM_ACCESS_MMAP_INTERLEAVED);
    if (err < 0) {
        printf("failed at %s\n", snd_strerror(err));
    }
    err = snd_pcm_hw_params_set_format(sound_config->pcm, hw_params, SND_PCM_FORMAT_S16_LE);
    if (err < 0) {
        printf("failed at %s\n", snd_strerror(err));
    }
    err = snd_pcm_hw_params_set_channels(sound_config->pcm, hw_params, channels);
    if (err < 0) {
        printf("failed at %s\n", snd_strerror(err));
    }
    err = snd_pcm_hw_params_set_rate(sound_config->pcm, hw_params, 48000, 0);
    if (err < 0) {
        printf("failed at %s\n", snd_strerror(err));
    }
    err = snd_pcm_hw_params_set_buffer_size(sound_config->pcm, hw_params, PERIOD_SIZE * 2);
    if (err < 0) {
        printf("failed at %s\n", snd_strerror(err));
    }
    
    snd_pcm_uframes_t period_size = PERIOD_SIZE;
    err = snd_pcm_hw_params_set_period_size_near(sound_config->pcm, hw_params, &period_size, &dir);
    if (err < 0) {
        printf("failed at %s\n", snd_strerror(err));
    }

    err = snd_pcm_hw_params(sound_config->pcm, hw_params);
    if (err < 0) {
        printf("failed at %s\n", snd_strerror(err));
    }
    state = snd_pcm_state(sound_config->pcm);
    snd_pcm_dump(sound_config->pcm, sound_config->output);
    return err;
}

void XUpdateBufferDims(Display *display, Window window, Game_offscreen_buffer *gameBuffer) {
    XWindowAttributes attrs = {};
    XGetWindowAttributes(display, window, &attrs);
    gameBuffer->width = attrs.width;
    gameBuffer->height = attrs.height;
}

void XDisplayBufferInWindow(Display *display, Window window, GC gc,
                    X_offscreen_buffer *xbuffer, Game_offscreen_buffer *gameBuffer) {
    if(xbuffer->image) {
        XDestroyImage(xbuffer->image); // also frees the inner data.
    }
    
   xbuffer->image = XCreateImage(display, xbuffer->visual, xbuffer->depth, ZPixmap,
                                 0, (char *)gameBuffer->data, gameBuffer->width,
                                 gameBuffer->height, 8, gameBuffer->width*4);
    
    XPutImage(display, window, gc, xbuffer->image, 0, 0, 0, 0, gameBuffer->width, gameBuffer->height);
}


void handleEvent(XEvent event,
                 Game_input *input) {
    switch (event.type) {
        case KeyPress:
            printf("key pressed \n");
            break;
        case KeyRelease: {
            printf("key released\n");
            KeySym sym = XLookupKeysym(&event.xkey, 0);
            if (sym == XK_w) {
                input->wWasPressed = true;
            }
            if (sym == XK_a) {
                input->aWasPressed = true;
            }
            if (sym == XK_s) {
                input->sWasPressed = true;
            }
            if (sym == XK_d) {
                input->dWasPressed = true;
            }
        } break;
        case ConfigureNotify: {
            printf("structure changed\n");
        } break;
        case Expose:
            printf("expose\n");
            break;
        default:
            printf("ignore event\n");
    }
}

int XFillSoundBuffer(X_sound_config *sound_config,
                     Game_sound_buffer *gameSoundBuffer) {
    snd_pcm_uframes_t offset;
    snd_pcm_uframes_t framesToWrite = gameSoundBuffer->framesToWrite;
    const snd_pcm_channel_area_t *areas;
    int can_write = snd_pcm_mmap_begin(sound_config->pcm, &areas, &offset, &framesToWrite);
    if (can_write < 0) {
        printf("can't write %s\n", snd_strerror(can_write));
        return -1;
    }

    /*
         [L0 R0] [L1 R1] where l and r are 16 bits each.
         and they mean left and right speaker, 2 samples per frame.
    */
    int16 *ring_ptr= (int16 *)((uint8 *)areas[0].addr + (offset * areas[0].step / 8));
    int16 *sample_ptr = gameSoundBuffer->samples; 
    // copy from the samples array on the game's sound buffer to the actual ring buffer.
    for (uint32 i = 0; i < framesToWrite; i++) {
        *ring_ptr++ = *sample_ptr++; // LEFT
        *ring_ptr++ = *sample_ptr++; // RIGHT
    }
    snd_pcm_sframes_t framesWritten = snd_pcm_mmap_commit(sound_config->pcm, offset, framesToWrite);
    if (framesWritten <= 0) {
        return framesWritten;
    }
    snd_pcm_state_t state = snd_pcm_state(sound_config->pcm);   
    if (state == SND_PCM_STATE_PREPARED) {
        int err = snd_pcm_start(sound_config->pcm);
        if (err < 0) {
            printf("failed at uhuh%s\n", snd_strerror(err));
        }
    }
    return 0;
}

real64 XtimeElapsedMS (timespec lastTime, timespec endTime) {
    real64 timeElapsedSeconds = endTime.tv_sec - lastTime.tv_sec;
    real64 timeElapsedNanoSeconds = endTime.tv_nsec - lastTime.tv_nsec;
    real64 timeElapsedMS = (timeElapsedSeconds * 1000.0f)  + (timeElapsedNanoSeconds / (1000.0f * 1000.0f));
    return timeElapsedMS;
}

void *PlatformReadEntireFile(char *filename) {
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

void PlatformFreeFileMemory(void *BitmapMemory) {
    free(BitmapMemory);
}

int main() {
    X_game_code gameCode = {};
    gameCode.isValid = false;
    loadGameCode(&gameCode);
    X_sound_config sound_config = {};
    Game_sound_buffer gameSoundBuffer = {};
    X_offscreen_buffer xbuffer = {};
    Game_offscreen_buffer gameBuffer = {};
    gameSoundBuffer.sample_rate = 48000;
    gameSoundBuffer.amplitude = 10000.0f;
    gameSoundBuffer.frequency = 440.0f;
    gameSoundBuffer.phase = 0.0f;
    gameSoundBuffer.phase_increment = (2.0f * M_PI * gameSoundBuffer.frequency) / gameSoundBuffer.sample_rate;
    gameSoundBuffer.samples = (int16 *)malloc(48000 * 2 * 16);
    gameSoundBuffer.framesToWrite = PERIOD_SIZE; 
    Display *display = XOpenDisplay(NULL);
    if (!display) {
        printf("Display not found\n");
        return -1;
    }

    printf("X server connected to the display\n");
    Window parent = DefaultRootWindow(display);
    gameBuffer.width = 500;
    gameBuffer.height = 500;
    xbuffer.screen = DefaultScreen(display);
    xbuffer.visual = DefaultVisual(display, xbuffer.screen);
    xbuffer.depth = DefaultDepth(display, xbuffer.screen);

    /* Values */
    XSetWindowAttributes attrs = {};
    attrs.event_mask = ExposureMask |
        KeyPressMask |
        KeyReleaseMask |
        StructureNotifyMask;

    unsigned long valuemask = CWEventMask;
    
    Window window = XCreateWindow(display, parent, 0, 0, gameBuffer.width,
                                  gameBuffer.height, 0, CopyFromParent,
                                  InputOutput, CopyFromParent, valuemask, &attrs);
    XMapWindow(display, window);
    XGCValues gc_values = {};
    GC gc = XCreateGC(display, window, 0, &gc_values);

    printf("Window created and mapped\n"); 
    bool Running = true;
    XGrabKeyboard(display, window, False, GrabModeAsync, GrabModeAsync, CurrentTime);
    XAutoRepeatOn(display); 

    // int err = XInitSound(&sound_config); 
    timespec lastTime;
    timespec endTime;
    unsigned long long lastTimeClock = __rdtsc();
    Game_memory memory = {};
    uint64 size = megabytes(256);
    memory.permanentStorage = malloc(megabytes(256));
    memory.transientStorage = malloc(megabytes(256));
    memory.permanentStorageSize = 256;
    memory.transientStorageSize = 256;
    memory.readEntireFile = &PlatformReadEntireFile;
    memory.freeFileMemory = &PlatformFreeFileMemory;

    real32 MonitorRefreshHz = 120.0f;
    real32 GameUpdateHz = (MonitorRefreshHz/ 2.0f);
    real32 TargetMSPerFrame = (1000.0f / GameUpdateHz);

    Game_input input = {};
    clock_gettime(CLOCK_MONOTONIC_RAW, &lastTime);
    while (Running) {
        while (XPending(display)) {
            XEvent event;
            XNextEvent(display, &event);
            handleEvent(event, &input);
        }
     
#if HANDMADE_INTERNAL
        snd_pcm_dump(sound_config.pcm, sound_config.output);
        snd_pcm_status_alloca(&sound_config.status);
        snd_pcm_status_dump(sound_config.status, sound_config.output);
#endif
        // for the animation.
        // TODO: return the error codes from these functions as well.
        XUpdateBufferDims(display, window, &gameBuffer);
        // rendering means writing into the memory.
        gameCode.GameUpdateAndRender(&gameBuffer, &input, &memory);
        gameCode.GetSoundSamples(&gameSoundBuffer);
        
        clock_gettime(CLOCK_MONOTONIC_RAW, &endTime);
        real64 worktimeElapsedMS = XtimeElapsedMS(lastTime, endTime);
        real64 fps = 1000.0f / worktimeElapsedMS;
        printf("time taken : physics time %f, %f\n", worktimeElapsedMS, TargetMSPerFrame);
        if (worktimeElapsedMS < TargetMSPerFrame) {
            printf("Less time taken!\n");
            while (worktimeElapsedMS < TargetMSPerFrame) {
                // pass time till the target.
                sleep((TargetMSPerFrame - worktimeElapsedMS) / 1000.0f);
                clock_gettime(CLOCK_MONOTONIC_RAW, &endTime);
                worktimeElapsedMS = XtimeElapsedMS(lastTime, endTime);
            }
        } else {
            printf("More time taken!\n");
            // NOTE: what to do? log perhaps.
        }
      
        printf("time elapsed finally: physics time + render time %f\n", worktimeElapsedMS);
        // this is flipping what was in the memory to the front.
        XDisplayBufferInWindow(display, window, gc, &xbuffer, &gameBuffer);
        // snd_pcm_sframes_t available = snd_pcm_avail_update(sound_config.pcm); 
        // if (available < 0) {
        //     printf ("availability error %s\n", snd_strerror(available));
        //     snd_pcm_recover(sound_config.pcm, available, 0);
        //     available = snd_pcm_avail_update(sound_config.pcm);
        // }
        // printf("frames to write is %lu\n", available);
        // int err = XFillSoundBuffer(&sound_config, &gameSoundBuffer);
        // if (err < 0) {
        //     printf("Sound error \n");
        //     return err;
        // }
        clock_gettime(CLOCK_MONOTONIC_RAW, &endTime);
        worktimeElapsedMS = XtimeElapsedMS(lastTime, endTime);
        printf("time elapsed after flip %f\n", worktimeElapsedMS);
        input = {};
        clock_gettime(CLOCK_MONOTONIC_RAW, &endTime);
        lastTime = endTime;
    }

}
