#include <stdint.h>
#include <pthread.h>
#include <stdbool.h>

#define CHIP8_SCREEN_WIDTH 64
#define CHIP8_SCREEN_HEIGHT 32

typedef enum {
	CHIP8_BEEP = 0,
	CHIP8_CYCLE = 1,
	CHIP8_REDRAW = 2
} chip8_callback_type_t;

struct chip8_t;
typedef struct chip8_t chip8_t;

typedef void(*chip8_callback_t)(chip8_t *, chip8_callback_type_t);

struct chip8_t {
	// 1 represents a white pixel and 0 represents a black pixel. Use this for drawing.
	bool framebuffer[CHIP8_SCREEN_WIDTH][CHIP8_SCREEN_HEIGHT];

	// Access these directly if you want to have more control over the input.
	uint16_t keyboard_mask; // Example (check key F): self->keyboard_mask & 0xF
	bool dont_auto_update_keyboard_mask;

	// Do not access these directly unless you know what you are doing.
	uint8_t memory[0x1000];
	chip8_callback_t callbacks[3];
	uint8_t registers[0x10];
	uint16_t program_counter; // PC
	struct {
		uint8_t delay;
		uint8_t sound;
	} timers;
	uint16_t stack[0x10];
	uint8_t stack_pt; // SP
	uint16_t mem_pt; // I
	pthread_t emulation_thread;
};

chip8_t *chip8_init(void);
void chip8_keypress(chip8_t *self, uint8_t key);
void chip8_set_callback(chip8_t *self, chip8_callback_type_t type, chip8_callback_t callback);
size_t chip8_load_rom(chip8_t *self, const char *path);
bool chip8_start(chip8_t *self);