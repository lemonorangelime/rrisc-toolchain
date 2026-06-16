#include <device.h>
#include <stdlib.h>
#include <bus/mem.h>
#include <stdio.h>
#include <device/vga.h>
#include <options.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_pixels.h>

SDL_Color vga_palette_4bpp[] = {
	{0x00, 0x00, 0x00, 255},
	{0x11, 0x11, 0x11, 255},
	{0x22, 0x22, 0x22, 255},
	{0x33, 0x33, 0x33, 255},
	{0x44, 0x44, 0x44, 255},
	{0x55, 0x55, 0x55, 255},
	{0x66, 0x66, 0x66, 255},
	{0x77, 0x77, 0x77, 255},
	{0x88, 0x88, 0x88, 255},
	{0x99, 0x99, 0x99, 255},
	{0xaa, 0xaa, 0xaa, 255},
	{0xbb, 0xbb, 0xbb, 255},
	{0xcc, 0xcc, 0xcc, 255},
	{0xdd, 0xdd, 0xdd, 255},
	{0xee, 0xee, 0xee, 255},
	{0xff, 0xff, 0xff, 255},
};

int vga_read_handler(void * priv, bus_t * bus, uint32_t address, void * buffer, size_t bytes) {
	vga_device_t * vga = (vga_device_t *) priv;
	if (address < VGA_MMIO_BASE || address >= VGA_MMIO_END) {
		return 0;
	}

	uint32_t pixel_offset = (address - VGA_MMIO_BASE) << 1;
	uint8_t * pixel = buffer;
	*pixel = vga->fb[pixel_offset + 1];
	*pixel |= (vga->fb[pixel_offset] << 4) & 0xf;
	return 1;
}

int vga_write_handler(void * priv, bus_t * bus, uint32_t address, void * buffer, size_t bytes) {
	vga_device_t * vga = (vga_device_t *) priv;
	if (address < VGA_MMIO_BASE || address >= VGA_MMIO_END) {
		return 0;
	}

	uint32_t pixel_offset = (address - VGA_MMIO_BASE) << 1;
	uint8_t * pixel = buffer;
	vga->fb[pixel_offset + 1] = *pixel & 0xf;
	vga->fb[pixel_offset] = (*pixel >> 4) & 0xf;
	return 1;
}

int counter = 0;
void vga_clock(device_t * device) {
	vga_device_t * vga = (vga_device_t *) device;
	counter += 1;
	if (counter >= vga->counter_deadline) {
		SDL_Texture * texture = SDL_CreateTextureFromSurface(vga->renderer, vga->surface);
		SDL_RenderClear(vga->renderer);
		SDL_RenderCopy(vga->renderer, texture, NULL, NULL);
		SDL_RenderPresent(vga->renderer);
		SDL_DestroyTexture(texture);
		counter = 0;
	}
}

void vga_init(device_t * device) {
	vga_device_t * vga = (vga_device_t *) device;
	if (!vga->membus) {
		return; // erhm
	}

	int clock_speed = atoi(option_lookup_sysdef_key("cpu.clock-speed"));
	vga->counter_deadline = clock_speed / 60;
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
	vga->surface = SDL_CreateRGBSurfaceWithFormat(SDL_SWSURFACE, VGA_DEFAULT_WIDTH, VGA_DEFAULT_HEIGHT, 8, SDL_PIXELFORMAT_INDEX8);
	SDL_SetPaletteColors(vga->surface->format->palette, vga_palette_4bpp, 0, 256);
	vga->fb = vga->surface->pixels;
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