#include <stdio.h>
#include <stdlib.h>
#include <cstdint>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

uint32_t *data;
XImage *image;

void update_data(int window_width, int window_height) {
    data = (uint32_t *)malloc(window_width*window_height*4); // 4 bytes for a pixel RR GG BB XX
    uint32_t *ptr = data;
    for (int y = 0; y < window_height; ++y) {

        for (int x = 0; x < window_width; ++x) {
            uint32_t a = 0;
            uint32_t r = 0;
            uint32_t g = 0;
            uint32_t b = 255;

            // For RGBA format:
            uint32_t blue = (a << 24) | (r << 16) | (g << 8) | b;
            ptr[x] = blue;
        }
        ptr = ptr + window_width;
    }
}

void Xupdate_window(Display *display, Window window, GC gc) {
    if(image) {
        XDestroyImage(image); // also frees the inner data.
    }
    XWindowAttributes attrs = {};
    XGetWindowAttributes(display, window, &attrs);
    update_data(attrs.width, attrs.height);
    int screen = DefaultScreen(display);
    Visual *visual = DefaultVisual(display, screen);
    int depth = DefaultDepth(display, screen);
    
    image = XCreateImage(display, visual, depth, ZPixmap, 0, (char *)data, attrs.width, attrs.height, 8,
                                 attrs.width*4);
    
    XPutImage(display, window, gc, image, 0, 0, 0, 0, attrs.width, attrs.height);
}

void handleEvent(Display *display, Window window, GC gc, XEvent event) {
    switch (event.type) {
        case KeyPress:
            printf("key pressed \n");
            break;
        case ConfigureNotify: {
            printf("structure changed\n");
            Xupdate_window(display, window, gc);
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
    unsigned int width = 500;
    unsigned int height = 500;
    unsigned int border_width = 0;
    unsigned long background = 0;

    /* Values */
    XSetWindowAttributes attrs = {};
    attrs.event_mask = ExposureMask |
        KeyPressMask |
        KeyReleaseMask |
        StructureNotifyMask;

    unsigned long valuemask = CWEventMask;
    
    Window window = XCreateWindow(display, parent, 0, 0, width, height, border_width, CopyFromParent,
                       InputOutput, CopyFromParent, valuemask, &attrs);
    XMapWindow(display, window);
    XGCValues gc_values = {};
    GC gc = XCreateGC(display, window, 0, &gc_values);

    printf("Window created and mapped\n"); 

    for (;;){
        XEvent event;
        printf("Waiting for next event\n");
        XNextEvent(display, &event);
        handleEvent(display, window, gc, event);
    }

}
