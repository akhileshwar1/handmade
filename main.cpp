#include <stdio.h>
#include <stdlib.h>
#include <cstdint>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

typedef struct {
    uint32_t *data;
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
    buffer->data = (uint32_t *)malloc(buffer->window_width*buffer->window_height*4); // 4 bytes for a pixel RR GG BB XX
    uint32_t *ptr = buffer->data;
    for (int y = 0; y < buffer->window_height; ++y) {

        for (int x = 0; x < buffer->window_width; ++x) {
            uint8_t blue = (uint8_t)(x + buffer->XOffset);
            uint8_t green = (uint8_t)(y + buffer->YOffset);
            *ptr++ = (green << 8) | blue;
        }
    }
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
        case ConfigureNotify: {
            printf("structure changed\n");
            XUpdateBufferDims(display, window, buffer);
            renderweirdgradient(buffer);
            XDisplayBufferInWindow(display, window, gc, buffer);
        }
            break;
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
    
    Window window = XCreateWindow(display, parent, 0, 0, buffer.window_width, buffer.window_height, 0, CopyFromParent,
                       InputOutput, CopyFromParent, valuemask, &attrs);
    XMapWindow(display, window);
    XGCValues gc_values = {};
    GC gc = XCreateGC(display, window, 0, &gc_values);

    printf("Window created and mapped\n"); 
    bool Running = true;

    while (Running) {
        while (XPending(display)) {
            XEvent event;
            XNextEvent(display, &event);
            handleEvent(display, window, gc, event, &buffer);
        }
       
        XUpdateBufferDims(display, window, &buffer);
        renderweirdgradient(&buffer);
        XDisplayBufferInWindow(display, window, gc, &buffer);
        buffer.XOffset++;
        buffer.YOffset++;
    }

}
