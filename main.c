#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "emulator.h"
 
static Window window;
static Display *display;
static int width, height;
static int screen_no;
static chip8_t *chip8;
static const int multiplier = 8;
static const long event_mask = (KeyPressMask | KeyReleaseMask);
static pthread_mutex_t event_lock = PTHREAD_MUTEX_INITIALIZER;

void redraw_game(chip8_t *caller, chip8_callback_type_t type) {
	for (int y=0; y<height-1; y+=multiplier) {
		for (int x=0; x<width-1; x+=multiplier) {
			_Bool is_white = caller->framebuffer[x/multiplier][y/multiplier];
			if (caller->timers.sound > 0) is_white = !is_white;
			pthread_mutex_lock(&event_lock);
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
			pthread_mutex_unlock(&event_lock);
		}
	}
}

int main(int argc, char **argv) {
	if (argc <= 1) {
		fprintf(stderr, "Usage: %s <program>\n", *argv);
		return 1;
	}
	display = XOpenDisplay(NULL);
	if (display == NULL) {
		fprintf(stderr, "Cannot open display\n");
		exit(1);
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
	while (1) {
		pthread_mutex_lock(&event_lock);
		if (XCheckWindowEvent(display, window, event_mask, &event)) {
			if (!XLookupString(&event.xkey, input_buffer, 1, &key_symbol, NULL)) input_buffer[0] = 0;
			pthread_mutex_unlock(&event_lock);
			char c = *input_buffer;
			if ((c >= 'a') && (c <= 'z')) c -= 0x20;
			unsigned char chip8_key = 0x10;
			// NOTE TO FUTURE READERS: This is not the right way to do this.
			switch (c) {
				#define case(a,b) case a: chip8_key = b; break
				case('1',0x1); case('2',0x2);
				case('3',0x3); case('4',0xC);
				case('Q',0x4); case('W',0x5);
				case('E',0x6); case('R',0xD);
				case('A',0x7); case('S',0x8);
				case('D',0x9); case('F',0xE);
				case('Z',0xA); case('X',0x0);
				case('C',0xB); case('V',0xF);
				#undef case
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
		else pthread_mutex_unlock(&event_lock);
	}
	XCloseDisplay(display);
	return 0;
}