#include <stdio.h>
#include <X11/Xlib.h>

int main() {
    Display *display = XOpenDisplay(NULL);
    if (!display) {
        printf("Display not found\n");
    }

    printf("X server connected to the display\n");
    Window parent = DefaultRootWindow(display);
    unsigned int width = 500;
    unsigned int height = 500;
    unsigned int border_width = 500;
    unsigned long border = 500;
    unsigned long background = 500;

    /* Values */
    XSetWindowAttributes attrs = {};
    attrs.event_mask = ExposureMask |
        KeyPressMask |
        KeyReleaseMask |
        StructureNotifyMask;

    unsigned long valuemask = CWEventMask;
    
    Window window = XCreateWindow(display, parent, 4, 4, width, height, border_width, CopyFromParent,
                       InputOutput, CopyFromParent, valuemask, &attrs);
    XMapWindow(display, window);

    printf("Window created and mapped\n"); 
    for (;;){
        XEvent event;
        printf("Waiting for next event\n");
        XNextEvent(display, &event);
    }
}
