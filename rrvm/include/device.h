#pragma once

#include <bus.h>

typedef struct device device_t;

typedef void (* device_clock_t)(device_t * device);
typedef void (* device_free_t)(device_t * device);
typedef void (* device_init_t)(device_t * device);
typedef device_t * (* device_create_t)();

typedef struct device {
	device_clock_t clock;
	device_free_t free;
	device_init_t init;
	bus_port_t * ports;
	int port_count;
	int * interrupt_pin;
} device_t;

typedef struct {
	char * name;
	device_create_t create;
} device_interface_t;

#define DEVICE_INTERFACE __attribute__((section(".devinterfaces")))

device_interface_t * lookup_dev_interface(char * name);