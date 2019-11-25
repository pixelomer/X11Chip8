#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "emulator.h"
 
static Window window;
static Display *display;
static int width, height;
static int screen_no;
static chip8_t *chip8;
static const int multiplier = 8;
static const long event_mask = (KeyPressMask | KeyReleaseMask | ExposureMask);

void redraw_game(chip8_t *caller, chip8_callback_type_t type) {
	XExposeEvent expose_event;
	memset(&expose_event, 0, sizeof(XExposeEvent));
	expose_event.type = Expose;
	expose_event.window = window;
	expose_event.send_event = 1;
	XLockDisplay(display);
	Status status = XSendEvent(display, window, 0, 0, (XEvent *)&expose_event);
	XFlush(display);
	XUnlockDisplay(display);
}

int main(int argc, char **argv) {
	if (argc <= 1) {
		fprintf(stderr, "Usage: %s <program>\n", *argv);
		return 1;
	}
	if (!XInitThreads()) {
		fprintf(stderr, "Couldn't initialize threads\n");
		return 1;
	}
	display = XOpenDisplay(NULL);
	if (display == NULL) {
		fprintf(stderr, "Cannot open display\n");
		return 1;
	}

	// Create window
	screen_no = DefaultScreen(display);
	{
		window = XCreateSimpleWindow(
			display, // Display
			RootWindow(display, screen_no), // Parent
			50, // X
			50, // Y
			(width = (CHIP8_SCREEN_WIDTH * multiplier)), // Width
			(height = (CHIP8_SCREEN_HEIGHT * multiplier)), // Height
			1, // Border Width
			WhitePixel(display, screen_no), // Border
			BlackPixel(display, screen_no) // Background
		);
		XSelectInput(display, window, event_mask);
		XMapWindow(display, window);
	}

	// Start emulation
	chip8 = chip8_init();
	chip8->dont_auto_update_keyboard_mask = 1;
	chip8_load_rom(chip8, argv[1]);
	chip8_set_callback(chip8, CHIP8_REDRAW, &redraw_game);
	chip8_start(chip8);

	// Event loop
	XEvent event;
	char input_buffer[2];
	KeySym key_symbol;
	input_buffer[1] = 0;
	_Bool first_loop = 1;
	while (1) {
		XNextEvent(display, &event);
		if (event.type == Expose) {
			XLockDisplay(display);
			for (int y=0; y<height-1; y+=multiplier) {
				for (int x=0; x<width-1; x+=multiplier) {
					_Bool is_white = chip8->framebuffer[x/multiplier][y/multiplier];
					if (chip8->timers.sound > 0) is_white = !is_white;
					XSetForeground(display, DefaultGC(display, screen_no), (0xFFFFFF * !!is_white));
					XFillRectangle(
						display,
						window,
						DefaultGC(display, screen_no),
						x,
						y,
						multiplier,
						multiplier
					);
				}
			}
			XUnlockDisplay(display);
			continue;
		}
		if (!XLookupString(&event.xkey, input_buffer, 1, &key_symbol, NULL)) input_buffer[0] = 0;
		char c = *input_buffer;
		if ((c >= 'a') && (c <= 'z')) c -= 0x20;
		unsigned char chip8_key = 0x10;
		static const char *keymap = "X123QWEASDZC4RFV";
		for (unsigned char i=0; i<0x10; i++) {
			if (keymap[i] == c) {
				chip8_key = i;
				break;
			}
		}
		if (chip8_key == 0x10) continue;
		switch (event.type) {
			case KeyPress:
				chip8->keyboard_mask |= (1UL << chip8_key);
				break;
			case KeyRelease:
				chip8->keyboard_mask &= ~(1UL << chip8_key);
				break;
		}
	}
	XCloseDisplay(display);
	return 0;
}