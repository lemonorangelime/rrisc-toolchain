#include <device.h>
#include <stdlib.h>
#include <bus/mem.h>
#include <stdio.h>
#include <device/vga.h>
#include <options.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_pixels.h>
#include <time.h>
#include <sys/time.h>

uint32_t vga_palette_4bpp[] = {
	0xff000000,
	0xff111111,
	0xff222222,
	0xff333333,
	0xff444444,
	0xff555555,
	0xff666666,
	0xff777777,
	0xff888888,
	0xff999999,
	0xffaaaaaa,
	0xffbbbbbb,
	0xffcccccc,
	0xffdddddd,
	0xffeeeeee,
	0xffffffff,
};

#define VGA_PALETTE_BASE (0x1a70 << 2)
#define VGA_PALETTE_END (0x1a80 << 2)

#define VGA_CTRL_REG (0x1a60 << 2)

uint8_t vga_palette_lookup_colour(vga_device_t * vga, uint32_t colour) {
	for (int i = 0; i < 16; i++) {
		if (vga->palette[i] == colour) {
			return i;
		}
	}
	return 0;
}

int vga_read_handler(void * priv, bus_t * bus, uint32_t address, void * b, size_t bytes) {
	vga_device_t * vga = (vga_device_t *) priv;
	uint8_t * buffer = b;
	if (address >= VGA_PALETTE_BASE && address < VGA_PALETTE_END) {
		uint32_t palette_offset = address - VGA_PALETTE_BASE;
		*buffer = ((uint8_t *) vga->palette)[palette_offset];
		return 0;
	}
	if (address < VGA_MMIO_BASE || address >= VGA_MMIO_END) {
		return 0;
	}

	uint32_t pixel_offset = (address - VGA_MMIO_BASE) << 1;
	*buffer = vga_palette_lookup_colour(vga, vga->fb[pixel_offset + 1]) & 0xf;
	*buffer |= (vga_palette_lookup_colour(vga, vga->fb[pixel_offset]) << 4) & 0xf0;
	return 1;
}

int vga_write_handler(void * priv, bus_t * bus, uint32_t address, void * b, size_t bytes) {
	vga_device_t * vga = (vga_device_t *) priv;
	uint8_t * buffer = b;
	if (address >= VGA_PALETTE_BASE && address < VGA_PALETTE_END) {
		if ((address % 4) == 0) {
			return 0;
		}
		uint32_t palette_offset = address - VGA_PALETTE_BASE;
		palette_offset = palette_offset ^ 0b11;
		((uint8_t *) vga->palette)[palette_offset] = *buffer;
		return 0;
	}

	if (address < VGA_MMIO_BASE || address >= VGA_MMIO_END) {
		return 0;
	}

	uint32_t pixel_offset = (address - VGA_MMIO_BASE) << 1;
	vga->fb[pixel_offset + 1] = vga->palette[*buffer & 0xf];
	vga->fb[pixel_offset] = vga->palette[(*buffer >> 4) & 0xf];
	return 1;
}

void vga_clock(device_t * device) {
	vga_device_t * vga = (vga_device_t *) device;
	struct timeval tv;
	uint64_t ms;

	gettimeofday(&tv, NULL);
	ms = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
	if ((ms >= vga->ms_deadline) || (vga->ms_deadline < ms)) {
		SDL_Texture * texture = SDL_CreateTextureFromSurface(vga->renderer, vga->surface);
		SDL_RenderClear(vga->renderer);
		SDL_RenderCopy(vga->renderer, texture, NULL, NULL);
		SDL_RenderPresent(vga->renderer);
		SDL_DestroyTexture(texture);
		vga->ms_deadline = ms + 16;
	}
}

void vga_init(device_t * device) {
	vga_device_t * vga = (vga_device_t *) device;
	if (!vga->membus) {
		return; // erhm
	}

	int clock_speed = atoi(option_lookup_sysdef_key("cpu.clock-speed"));

	struct timeval tv;
	gettimeofday(&tv, NULL);
	vga->ms_deadline = (tv.tv_sec * 1000) + (tv.tv_usec / 1000) + 16;

	mem_bus_attach_read(vga->membus, vga_read_handler, vga);
	mem_bus_attach_write(vga->membus, vga_write_handler, vga);
}

void vga_free(device_t * device) {
	vga_device_t * vga = (vga_device_t *) device;
	free(vga->fb);
	SDL_FreeSurface(vga->surface);
	SDL_DestroyRenderer(vga->renderer);
	SDL_DestroyWindow(vga->window);
	SDL_Quit();
	free(device->ports);
	free(device);
}

int vga_devices_created = 0;
device_t * vga_create_device() {
	if (vga_devices_created++ == 1) {
		return NULL; // too many
	}

	SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		return NULL; // init failed
	}

	vga_device_t * vga = malloc(sizeof(vga_device_t));
	vga->device.clock = vga_clock;
	vga->device.free = vga_free;
	vga->device.init = vga_init;
	vga->device.port_count = 1;
	vga->membus = NULL;
	vga->width = VGA_DEFAULT_WIDTH;
	vga->height = VGA_DEFAULT_HEIGHT;
	vga->window = SDL_CreateWindow("rrvm vga display", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, VGA_DEFAULT_WIDTH, VGA_DEFAULT_HEIGHT, SDL_WINDOW_SHOWN);
	vga->renderer = SDL_CreateRenderer(vga->window, -1, SDL_RENDERER_SOFTWARE);
	vga->surface = SDL_CreateRGBSurfaceWithFormat(SDL_SWSURFACE, VGA_DEFAULT_WIDTH, VGA_DEFAULT_HEIGHT, 32, SDL_PIXELFORMAT_ARGB8888);
	vga->fb = vga->surface->pixels;
	memcpy(vga->palette, vga_palette_4bpp, sizeof(vga->palette));
	vga->bpp = 4;

	memset(vga->fb, 0, VGA_DEFAULT_WIDTH * VGA_DEFAULT_HEIGHT);

	bus_port_t * port_list = malloc(sizeof(bus_port_t) * 1);
	port_list[0].type = BUS_MEMORY;
	port_list[0].bus = &vga->membus;
	vga->device.ports = port_list;

	return &vga->device;
}

device_interface_t DEVICE_INTERFACE vga_device_interface = {
	.name = "vga",
	.create = vga_create_device,
};