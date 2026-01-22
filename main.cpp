#include <stdio.h>
#include <stdlib.h>
#include <cstdint>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <alsa/asoundlib.h>
#include <math.h>

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef float real32;
typedef double real64;

enum { SAMPLE_RATE = 48000, };
typedef struct {
    uint32 *data;
    XImage *image;
    int window_width;
    int window_height;
    int XOffset;
    int YOffset;
    int screen; 
    Visual *visual;
    int depth; 
} X_offscreen_buffer;

X_offscreen_buffer buffer = {};

void renderweirdgradient(X_offscreen_buffer *buffer) {
    buffer->data = (uint32 *)malloc(buffer->window_width*buffer->window_height*4); // 4 bytes for a pixel RR GG BB XX
    uint32 *ptr = buffer->data;
    for (int y = 0; y < buffer->window_height; ++y) {

        for (int x = 0; x < buffer->window_width; ++x) {
            uint8 blue = (uint8)(x + buffer->XOffset);
            uint8 green = (uint8)(y + buffer->YOffset);
            *ptr++ = (green << 8) | blue;
        }
    }
}

void sine_wave_sound(short *buffer, size_t sample_count, int freq) {
    for (int i = 0; i < sample_count; i++) {
        buffer[i] = 10000 * sinf(2*M_PI*freq*((float)i/SAMPLE_RATE));
    }
}

void square_wave_sound(short *buffer, size_t sample_count) {
    short value = 16000;
    int period = 0;
    for (int i = 0; i < sample_count; i++) {
        if (period == 10) {
            period = 0;
            value = -value;
        }
        buffer[i] = value;
    }
}

void XPlaySound(short *samples_buffer) {
    snd_pcm_t *pcm;
    snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK, 0);

    snd_pcm_hw_params_t *hw_params;
    snd_pcm_hw_params_alloca(&hw_params);
    snd_pcm_hw_params_any(pcm, hw_params);
    snd_pcm_hw_params_set_access(pcm, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm, hw_params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pcm, hw_params, 1);
    snd_pcm_hw_params_set_rate(pcm, hw_params, 48000, 0);
    snd_pcm_hw_params_set_periods(pcm, hw_params, 10, 0);
    snd_pcm_hw_params_set_period_time(pcm, hw_params, 100000, 0); // 0.1 seconds
    snd_pcm_hw_params(pcm, hw_params);

    // write the samples.
    snd_pcm_writei(pcm, samples_buffer, 48000);
    // wait for the device to play all our samples.
    snd_pcm_drain(pcm);
    snd_pcm_close(pcm);
}

void XUpdateBufferDims(Display *display, Window window, X_offscreen_buffer *buffer) {
    XWindowAttributes attrs = {};
    XGetWindowAttributes(display, window, &attrs);
    buffer->window_width = attrs.width;
    buffer->window_height = attrs.height;
}

void XDisplayBufferInWindow(Display *display, Window window, GC gc,
                    X_offscreen_buffer *buffer) {
    if(buffer->image) {
        XDestroyImage(buffer->image); // also frees the inner data.
    }
    
    buffer->image = XCreateImage(display, buffer->visual, buffer->depth, ZPixmap,
                                 0, (char *)buffer->data, buffer->window_width,
                                 buffer->window_height, 8, buffer->window_width*4);
    
    XPutImage(display, window, gc, buffer->image, 0, 0, 0, 0, buffer->window_width, buffer->window_height);
}

void handleEvent(Display *display, Window window, GC gc, XEvent event,
                 X_offscreen_buffer *buffer) {
    switch (event.type) {
        case KeyPress:
            printf("key pressed \n");
            break;
        case KeyRelease: {
            printf("key released\n");
            KeySym sym = XLookupKeysym(&event.xkey, 0);
            if (sym == XK_w) {
                buffer->YOffset++;
            }
            if (sym == XK_a) {
                buffer->XOffset--;
            }
            if (sym == XK_s) {
                buffer->YOffset--;
            }
            if (sym == XK_d) {
                buffer->XOffset++;
            }
        } break;
        case ConfigureNotify: {
            printf("structure changed\n");
            XUpdateBufferDims(display, window, buffer);
            renderweirdgradient(buffer);
            XDisplayBufferInWindow(display, window, gc, buffer);
        } break;
        case Expose:
            printf("expose\n");
            break;
        default:
            printf("ignore event\n");
    }
}


int main() {
    Display *display = XOpenDisplay(NULL);
    if (!display) {
        printf("Display not found\n");
    }

    printf("X server connected to the display\n");
    Window parent = DefaultRootWindow(display);
    buffer.window_width = 500;
    buffer.window_height = 500;
    buffer.screen = DefaultScreen(display);
    buffer.visual = DefaultVisual(display, buffer.screen);
    buffer.depth = DefaultDepth(display, buffer.screen);

    /* Values */
    XSetWindowAttributes attrs = {};
    attrs.event_mask = ExposureMask |
        KeyPressMask |
        KeyReleaseMask |
        StructureNotifyMask;

    unsigned long valuemask = CWEventMask;
    
    Window window = XCreateWindow(display, parent, 0, 0, buffer.window_width,
                                  buffer.window_height, 0, CopyFromParent,
                                  InputOutput, CopyFromParent, valuemask, &attrs);
    XMapWindow(display, window);
    XGCValues gc_values = {};
    GC gc = XCreateGC(display, window, 0, &gc_values);

    printf("Window created and mapped\n"); 
    bool Running = true;
    XGrabKeyboard(display, window, False, GrabModeAsync, GrabModeAsync, CurrentTime);
    XAutoRepeatOn(display); 

    short samples_buffer[SAMPLE_RATE];
    // sine_wave_sound(&samples_buffer[0], SAMPLE_RATE, 200);
    square_wave_sound(&samples_buffer[0], SAMPLE_RATE);
    XPlaySound(samples_buffer);
    while (Running) {
        while (XPending(display)) {
            XEvent event;
            XNextEvent(display, &event);
            handleEvent(display, window, gc, event, &buffer);
        }
      
        // for the animation.
        XUpdateBufferDims(display, window, &buffer);
        renderweirdgradient(&buffer);
        XDisplayBufferInWindow(display, window, gc, &buffer);
        buffer.XOffset++;
        // buffer.YOffset++;
    }

}
