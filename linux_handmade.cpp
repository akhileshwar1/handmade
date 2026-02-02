#include <stdio.h>
#include <stdlib.h>
#include <cstdint>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <alsa/asoundlib.h>
#include <math.h>
#include <time.h>
#include <x86intrin.h>

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int16_t int16;
typedef float real32;
typedef double real64;
typedef struct timespec timespec;

#include "handmade.cpp"

typedef struct {
    XImage *image;
    int screen; 
    Visual *visual;
    int depth; 
} X_offscreen_buffer;

typedef struct {
    snd_pcm_t *pcm;
} X_sound_config;

int XInitSound(X_sound_config *sound_config) {
    snd_pcm_state_t state;
    int dir = 0; // basically chose a value almost equal to what was passed.
    int channels = 2;
    static unsigned int period_time = 100000;       /* period time in us */
    int err;
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
    // add one sec worth of buffer size i.e 48000 frames.
    // so that this one sec is always maintained between read and write pointer
    // of the audio ring buffer.
    err = snd_pcm_hw_params_set_buffer_size(sound_config->pcm, hw_params, 48000);
    if (err < 0) {
        printf("failed at %s\n", snd_strerror(err));
    }
    err = snd_pcm_hw_params_set_period_time_near(sound_config->pcm, hw_params, &period_time, &dir);
    if (err < 0) {
        printf("failed at %s\n", snd_strerror(err));
    }
    // 48 period size * 1000 (100ms period time) == 1 second worth of buffer.
    // period is the smallest unit of work read from sound buffer.
    snd_pcm_uframes_t period_size = 512;
    err = snd_pcm_hw_params_set_period_size_near(sound_config->pcm, hw_params, &period_size, &dir);
    if (err < 0) {
        printf("failed at %s\n", snd_strerror(err));
    }

    err = snd_pcm_hw_params(sound_config->pcm, hw_params);
    if (err < 0) {
        printf("failed at %s\n", snd_strerror(err));
    }
    state = snd_pcm_state(sound_config->pcm);
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

void handleEvent(Display *display, Window window, GC gc, XEvent event,
                 X_offscreen_buffer *xbuffer, Game_offscreen_buffer *gameBuffer,
                 Game_sound_buffer *gameSoundBuffer,Game_input *gameInput) {
    switch (event.type) {
        case KeyPress:
            printf("key pressed \n");
            break;
        case KeyRelease: {
            printf("key released\n");
            KeySym sym = XLookupKeysym(&event.xkey, 0);
            if (sym == XK_w) {
                gameInput->wWasPressed = true;
            }
            if (sym == XK_a) {
                gameInput->aWasPressed = true;
            }
            if (sym == XK_s) {
                gameInput->sWasPressed = true;
            }
            if (sym == XK_d) {
                gameInput->dWasPressed = true;
            }
        } break;
        case ConfigureNotify: {
            printf("structure changed\n");
            XUpdateBufferDims(display, window, gameBuffer);
            gameUpdateAndRender(gameBuffer, gameSoundBuffer, gameInput);
            *gameInput = {}; // Reset the gameInput because we send transitions here.
            XDisplayBufferInWindow(display, window, gc, xbuffer, gameBuffer);
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
    snd_pcm_uframes_t frames;
    const snd_pcm_channel_area_t *areas;
    int can_write = snd_pcm_mmap_begin(sound_config->pcm, &areas, &offset, &frames);
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
    for (uint32 i = 0; i < gameSoundBuffer->frames; i++) {
        int16 sample_value = gameSoundBuffer->samples[i];
        *ring_ptr++ = *sample_ptr++; // LEFT
        *ring_ptr++ = *sample_ptr++; // RIGHT
    }
    snd_pcm_mmap_commit(sound_config->pcm, offset, frames);
    snd_pcm_state_t state = snd_pcm_state(sound_config->pcm);   
    if (state == SND_PCM_STATE_PREPARED) {
        int err = snd_pcm_start(sound_config->pcm);
        if (err < 0) {
            printf("failed at uhuh%s\n", snd_strerror(err));
        }
    }
    return 0;
}


int main() {
    X_sound_config sound_config = {};
    Game_sound_buffer gameSoundBuffer = {};
    X_offscreen_buffer xbuffer = {};
    Game_offscreen_buffer gameBuffer = {};
    Game_input gameInput = {};
    gameSoundBuffer.sample_rate = 48000;
    gameSoundBuffer.amplitude = 10000.0f;
    gameSoundBuffer.frequency = 440.0f;
    gameSoundBuffer.phase = 0.0f;
    gameSoundBuffer.phase_increment = (2.0f * M_PI * gameSoundBuffer.frequency) / gameSoundBuffer.sample_rate;
    gameSoundBuffer.samples = (int16 *)malloc(48000 * 2 * 16);

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
    clock_gettime(CLOCK_MONOTONIC_RAW, &lastTime);
    unsigned long long lastTimeClock = __rdtsc();
    while (Running) {
        while (XPending(display)) {
            XEvent event;
            XNextEvent(display, &event);
            handleEvent(display, window, gc,
                        event, &xbuffer, &gameBuffer,
                        &gameSoundBuffer,&gameInput);
        }
      
        // for the animation.
        // TODO: return the error codes from these functions as well.
        XUpdateBufferDims(display, window, &gameBuffer);
        // snd_pcm_sframes_t available = snd_pcm_avail_update(sound_config.pcm); 
        // if (available < 0) {
        //     printf ("availability error %s\n", snd_strerror(available));
        //     snd_pcm_recover(sound_config.pcm, available, 0);
        //     available = snd_pcm_avail_update(sound_config.pcm);
        // }
        // gameSoundBuffer.frames = available;
        gameUpdateAndRender(&gameBuffer, &gameSoundBuffer, &gameInput);
        gameInput = {}; // Reset the gameInput because we send transitions here.
        XDisplayBufferInWindow(display, window, gc, &xbuffer, &gameBuffer);
        gameBuffer.XOffset++;
        // int err = XFillSoundBuffer(&sound_config, &gameSoundBuffer);
        // if (err < 0) {
        //     printf("Sound error \n");
        //     return err;
        // }

        timespec endTime;
        clock_gettime(CLOCK_MONOTONIC_RAW, &endTime);

        real64 timeElapsed = endTime.tv_nsec - lastTime.tv_nsec;
        real64 timeElapsedMS = timeElapsed / (1000.0f * 1000.0f); 
        real64 fps = 1000.0f / timeElapsedMS;
        printf("time elapsed in ms: %f\n", timeElapsedMS);
        printf("FPS: %f\n", fps);
        lastTime = endTime;

        unsigned long long endTimeClock = __rdtsc();
        real64 clockElapsed = endTimeClock - lastTimeClock; // in 10^13 order. 
        real64 clockTimeElapsedMS = (1000.0f * 10000.0f) / clockElapsed;
        printf("Clock time elapsed in ms : %f\n", clockTimeElapsedMS);
        lastTimeClock = endTimeClock;

        // gameBuffer.YOffset++;
    }

}
