#include <stdio.h>
#include <stdlib.h>
#include <cstdint>
#include <X11/Xlib.h>

uint32_t *data;

int get_width_of_window(Display *display, Window window) {
    XWindowAttributes attrs = {};
    XGetWindowAttributes(display, window, &attrs);
    return attrs.width;
}

void Xupdate_data(Display *display, Window window) {
    if (data) {
        free(data);
    }

    int width = get_width_of_window(display, window);
    data = (uint32_t *)malloc(width*4); // 4 bytes for a pixel RR GG BB XX
    for (int x = 0; x < width; x++) {
        uint32_t a = 0;
        uint32_t r = 0;
        uint32_t g = 0;
        uint32_t b = 255;

        // For RGBA format:
        uint32_t blue = (a << 24) | (r << 16) | (g << 8) | b;
        data[x] = blue;
    }
}

void Xupdate_window(Display *display, Window window) {
    int screen = DefaultScreen(display);
    Visual *visual = DefaultVisual(display, screen);
    int depth = DefaultDepth(display, screen);
    
    int width = get_width_of_window(display, window);
    XImage *image = XCreateImage(display, visual, depth, ZPixmap, 0, (char *)data, width, 1, 8,
                                 width*4);
    XGCValues gc_values = {};
    GC gc = XCreateGC(display, window, 0, &gc_values);
    XPutImage(display, window, gc, image, 0, 0, 0, 0, width, 1);
}

void handleEvent(Display *display, Window window, XEvent event) {
    switch (event.type) {
        case KeyPress:
            printf("key pressed \n");
            break;
        case ConfigureNotify: {
            printf("structure changed\n");
            Xupdate_data(display, window);
        }
            break;
        case Expose:
            printf("expose\n");
            break;
        default:
            printf("ignore event\n");
    }
    Xupdate_window(display, window);
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

    printf("Window created and mapped\n"); 

    for (;;){
        XEvent event;
        printf("Waiting for next event\n");
        XNextEvent(display, &event);
        handleEvent(display, window, event);
    }

}
