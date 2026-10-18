#include <device.h>
#include <stdlib.h>
#include <bus/mem.h>
#include <stdio.h>
#include <device/m3p.h>
#include <options.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_pixels.h>
#include <time.h>
#include <sys/time.h>
#include <util.h>

uint32_t m3p_palette_4bpp[] = {
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

int m3p_min2(int x, int y) {
	return (x > y) ? y : x;
}

void m3p_modeset(m3p_device_t * m3p);

void m3p_process_command(m3p_device_t * m3p, int cmd) {
	switch (cmd) {
		case 0:
			m3p->width = betohl(m3p->registers.op0);
			m3p->height = betohl(m3p->registers.op1);
			m3p_modeset(m3p);
			break;
		case 0xff:
			m3p->width = M3P_DEFAULT_WIDTH;
			m3p->height = M3P_DEFAULT_HEIGHT;
			m3p_modeset(m3p);
			break;
	}
}

void m3p_process_register_write(m3p_device_t * m3p, int reg_off) {
	uint32_t statwrite = (betohl(m3p->registers.ctrl) >> 24) & 0xff;
	uint32_t cmdwrite = betohl(m3p->registers.ctrl) & 0xff;
	if (reg_off == 3) {
		m3p->registers.ctrl &= htobel(0x00ffffff);
		switch (statwrite) {
			case 0:
				m3p_process_command(m3p, cmdwrite);
				break;
			case M3P_STATUS_WRITE_ADVANCED_MODE:
				m3p->mode = M3P_MODE_ADVANCED_MODE;
				m3p->fbsize = (m3p->width * m3p->height) << 2;
				m3p->status |= 1;
				m3p_modeset(m3p);
				break;
			case M3P_STATUS_WRITE_COMPAT_MODE:
				m3p->mode = M3P_MODE_RRISC32_COMPAT;
				m3p->fbsize = (m3p->width >> 1) * m3p->height;
				m3p->status &= 0b11111110;
				m3p_modeset(m3p);
				break;
		}
		m3p->registers.ctrl |= htobel(m3p->status << 24);
	}
}

int m3p_read_handler(void * priv, bus_t * bus, uint32_t address, void * b, size_t bytes) {
	m3p_device_t * m3p = (m3p_device_t *) priv;
	uint8_t * buffer = b;

	if (address >= M3P_REG_BASE && address < M3P_REG_END) {
		uint32_t reg_offset = address - M3P_REG_BASE;
		*buffer = ((uint8_t *) &m3p->registers)[reg_offset];
		return 1;
	}

	if (m3p->mode != M3P_MODE_RRISC32_COMPAT)
		address ^= 0b11;

	if (address >= M3P_PALETTE_BASE && address < M3P_PALETTE_END) {
		uint32_t palette_offset = address - M3P_PALETTE_BASE;
		palette_offset ^= 0b11;
		*buffer = ((uint8_t *) m3p->palette)[palette_offset];
		return 1;
	}
	if (address < M3P_VRAM_BASE || address >= M3P_VRAM_END) {
		return 0;
	}

	if ((m3p->mode == M3P_MODE_RRISC32_COMPAT) && address >= M3P_COMPAT_MMIO_END) {
		return 0;
	}

	uint32_t vram_offset = (address - M3P_VRAM_BASE);
	int clipped = m3p_min2(M3P_VRAM_END, address + bytes) - address;
	memcpy(b, m3p->vram + vram_offset, clipped);
	return clipped;
}

int m3p_write_handler(void * priv, bus_t * bus, uint32_t address, void * b, size_t bytes) {
	m3p_device_t * m3p = (m3p_device_t *) priv;
	uint8_t * buffer = b;

	if (address >= M3P_REG_BASE && address < M3P_REG_END) {
		uint32_t reg_offset = address - M3P_REG_BASE;
		((uint8_t *) &m3p->registers)[reg_offset] = *buffer;
		m3p_process_register_write(m3p, reg_offset);
		return 1;
	}

	if (m3p->mode != M3P_MODE_RRISC32_COMPAT)
		address ^= 0b11;

	if (address >= M3P_PALETTE_BASE && address < M3P_PALETTE_END) {
		if ((address % 4) == 0) {
			return 0;
		}

		uint32_t palette_offset = address - M3P_PALETTE_BASE;
		palette_offset ^= 0b11;
		((uint8_t *) m3p->palette)[palette_offset] = *buffer;
		return 1;
	}

	if (address < M3P_VRAM_BASE || address >= M3P_VRAM_END) {
		return 0;
	}

	if ((m3p->mode == M3P_MODE_RRISC32_COMPAT) && address >= M3P_COMPAT_MMIO_END) {
		return 0;
	}

	uint32_t vram_offset = (address - M3P_VRAM_BASE);
	int clipped = m3p_min2(M3P_VRAM_END, address + bytes) - address;
	if (vram_offset < m3p->fbsize) {
		if (m3p->mode == M3P_MODE_RRISC32_COMPAT) {
			uint32_t * sdl_fb = m3p->fb + (vram_offset << 1);
			int n = clipped;
			uint8_t * p = buffer;
			while (n--) {
				*sdl_fb++ = m3p->palette[(*p >> 4) & 0xf];
				*sdl_fb++ = m3p->palette[*p++ & 0xf];
			}
		} else {
			memcpy(((void *) m3p->fb) + (vram_offset), b, clipped);
		}
	}
	memcpy(m3p->vram + vram_offset, b, clipped);
	return clipped;
}

void m3p_clock(device_t * device) {
	m3p_device_t * m3p = (m3p_device_t *) device;
	struct timeval tv;
	uint64_t ms;

	gettimeofday(&tv, NULL);
	ms = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
	if ((ms >= m3p->ms_deadline) || (m3p->ms_deadline < ms)) {
		SDL_Texture * texture = SDL_CreateTextureFromSurface(m3p->renderer, m3p->surface);
		SDL_RenderClear(m3p->renderer);
		SDL_RenderCopy(m3p->renderer, texture, NULL, NULL);
		SDL_RenderPresent(m3p->renderer);
		SDL_DestroyTexture(texture);
		m3p->ms_deadline = ms + 16;
	}
}

void m3p_init(device_t * device) {
	m3p_device_t * m3p = (m3p_device_t *) device;
	if (!m3p->membus) {
		return; // erhm
	}

	int clock_speed = atoi(option_lookup_sysdef_key("cpu.clock-speed"));

	struct timeval tv;
	gettimeofday(&tv, NULL);
	m3p->ms_deadline = (tv.tv_sec * 1000) + (tv.tv_usec / 1000) + 16;

	mem_bus_attach_read(m3p->membus, m3p_read_handler, m3p);
	mem_bus_attach_write(m3p->membus, m3p_write_handler, m3p);
}

void m3p_modeset(m3p_device_t * m3p) {
	SDL_FreeSurface(m3p->surface);
	SDL_DestroyRenderer(m3p->renderer);
	SDL_DestroyWindow(m3p->window);

	char * title = (m3p->mode == M3P_MODE_RRISC32_COMPAT) ? "rrvm vga display" : "rrvm m3p display";
	m3p->window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, m3p->width, m3p->height, SDL_WINDOW_SHOWN);
	m3p->renderer = SDL_CreateRenderer(m3p->window, -1, SDL_RENDERER_SOFTWARE);
	m3p->surface = SDL_CreateRGBSurfaceWithFormat(SDL_SWSURFACE, m3p->width, m3p->height, 32, SDL_PIXELFORMAT_ARGB8888);
	m3p->fb = m3p->surface->pixels;
	memcpy(m3p->palette, m3p_palette_4bpp, sizeof(m3p->palette));
	if (m3p->mode == M3P_STATUS_WRITE_COMPAT_MODE) {
		m3p->fbsize = (m3p->width >> 1) * m3p->height;
	} else {
		m3p->fbsize = (m3p->width * m3p->height) << 2;
	}

	memset(m3p->fb, 0, (m3p->width * m3p->height) * 4);
	memset(m3p->vram, 0, m3p->fbsize);
}

void m3p_free(device_t * device) {
	m3p_device_t * m3p = (m3p_device_t *) device;
	free(m3p->vram);
	SDL_FreeSurface(m3p->surface);
	SDL_DestroyRenderer(m3p->renderer);
	SDL_DestroyWindow(m3p->window);
	SDL_Quit();
	free(device->ports);
	free(device);
}

int m3p_devices_created = 0;
device_t * m3p_create_device() {
	if (m3p_devices_created++ == 1) {
		return NULL; // too many
	}

	SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		return NULL; // init failed
	}

	m3p_device_t * m3p = malloc(sizeof(m3p_device_t));
	m3p->device.clock = m3p_clock;
	m3p->device.free = m3p_free;
	m3p->device.init = m3p_init;
	m3p->device.port_count = 1;
	m3p->membus = NULL;
	m3p->width = M3P_DEFAULT_WIDTH;
	m3p->height = M3P_DEFAULT_HEIGHT;
	m3p->window = SDL_CreateWindow("rrvm vga display", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, m3p->width, m3p->height, SDL_WINDOW_SHOWN);
	m3p->renderer = SDL_CreateRenderer(m3p->window, -1, SDL_RENDERER_SOFTWARE);
	m3p->surface = SDL_CreateRGBSurfaceWithFormat(SDL_SWSURFACE, m3p->width, m3p->height, 32, SDL_PIXELFORMAT_ARGB8888);
	m3p->fb = m3p->surface->pixels;
	m3p->vram = malloc(M3P_VRAM_SIZE);
	m3p->bpp = 4;
	m3p->fbsize = (m3p->width >> 1) * m3p->height;
	m3p->mode = M3P_MODE_RRISC32_COMPAT;
	m3p->status = 0;
	memcpy(m3p->palette, m3p_palette_4bpp, sizeof(m3p->palette));

	memset(m3p->fb, 0, (m3p->width * m3p->height) * 4);
	memset(m3p->vram, 0, M3P_VRAM_SIZE);

	bus_port_t * port_list = malloc(sizeof(bus_port_t) * 1);
	port_list[0].type = BUS_MEMORY;
	port_list[0].bus = &m3p->membus;
	m3p->device.ports = port_list;

	return &m3p->device;
}

device_interface_t DEVICE_INTERFACE m3p_device_interface = {
	.name = "rrisc-m3p",
	.create = m3p_create_device,
};
