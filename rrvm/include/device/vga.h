#pragma once

#include <device.h>
#include <bus.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_pixels.h>

// roadrunner mentioned being able to configure the controller in the future...
#define VGA_DEFAULT_WIDTH 320
#define VGA_DEFAULT_HEIGHT 240

#define VGA_MAX_WIDTH 640
#define VGA_MAX_HEIGHT 480
#define VGA_MAX_BPP 1

#define VGA_MMIO_BASE (0x1a80 << 2)
#define VGA_MMIO_END (0x4000 << 2)

typedef struct {
	device_t device;
	bus_t * membus;
	SDL_Palette * palette;
	SDL_Window * window;
	SDL_Renderer * renderer;
	SDL_Surface * surface;
	uint8_t * fb;
	int counter_deadline;
	int width;
	int height;
	int bpp;
} vga_device_t;