#pragma once

#include <device.h>
#include <bus.h>

typedef struct {
	device_t device;
	bus_t * iobus;
} iotrap_device_t;