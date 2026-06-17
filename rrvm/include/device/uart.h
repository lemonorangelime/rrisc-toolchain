#pragma once

#include <device.h>
#include <bus.h>

#define UART_MMIO_BASE (0x1a7d << 2)
#define UART_MMIO_END (0x1a80 << 2)

#define UART_RX_OFFSET 0x00
#define UART_TX_OFFSET 0x04
#define UART_STAT_OFFSET 0x08

// CHANGE ME
#define UART_FIFO_PATH "./uart-fifo"

enum {
	UART_STAT_RX_READY = 0b00000001000000000000000000000000,
	UART_STAT_TX_READY = 0b00000010000000000000000000000000,

	UART_STAT_RX_FINISHED = 0b10000000000000000000000000000000,
	UART_STAT_TX_FINISHED = 0b01000000000000000000000000000000,
};

typedef struct {
	uint32_t rx_data;
	uint32_t tx_data;
	uint32_t status;
} __attribute__((packed)) uart_mmio_buffer_t;

typedef struct {
	device_t device;
	bus_t * membus;
	uart_mmio_buffer_t mmio_buffer;
	void * rx_buffer;
	int fifo;
	int offset;
} uart_device_t;