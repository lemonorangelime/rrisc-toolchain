#pragma once

#include <device.h>
#include <bus.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_pixels.h>

#define M3P_DEFAULT_WIDTH 320
#define M3P_DEFAULT_HEIGHT 240

#define M3P_MAX_WIDTH 640
#define M3P_MAX_HEIGHT 480
#define M3P_MAX_BPP 1

#define M3P_VRAM_SIZE 0x10000000

#define M3P_COMPAT_MMIO_BASE (0x1a80 << 2)
#define M3P_COMPAT_MMIO_END (0x4000 << 2)

#define M3P_VRAM_BASE (0x1a80 << 2)
#define M3P_VRAM_END (M3P_VRAM_BASE + M3P_VRAM_SIZE)

#define M3P_REG_BASE (0x1a60 << 2)
#define M3P_REG_END (0x1a70 << 2)

#define M3P_PALETTE_BASE (0x1a70 << 2)
#define M3P_PALETTE_END (0x1a80 << 2)

typedef struct {
	uint32_t ctrl;
	uint32_t op0;
	uint32_t op1;
	uint32_t op2;

	uint32_t op3;
	uint32_t op4;
	uint32_t op5;
	uint32_t op6;

	uint32_t op7;
	uint32_t op8;
	uint32_t op9;
	uint32_t op10;

	uint32_t op11;
	uint32_t op12;
	uint32_t op13;
	uint32_t op14;
} __attribute__((packed)) m3p_registers_t;

typedef struct {
	device_t device;
	bus_t * membus;
	SDL_Window * window;
	SDL_Renderer * renderer;
	SDL_Surface * surface;
	uint32_t * fb;
	void * vram;
	uint32_t palette[16];
	uint64_t ms_deadline;
	int width;
	int height;
	int bpp;
	int mode;
	int fbsize;
	uint32_t status;
	m3p_registers_t registers;
} m3p_device_t;

enum {
	M3P_MODE_RRISC32_COMPAT,
	M3P_MODE_ADVANCED_MODE,
};

enum {
	M3P_STATUS_WRITE_ADVANCED_MODE = 0xc0,
	M3P_STATUS_WRITE_COMPAT_MODE = 0x80,
};
