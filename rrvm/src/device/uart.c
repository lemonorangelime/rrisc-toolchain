#include <device.h>
#include <stdlib.h>
#include <bus/mem.h>
#include <stdio.h>
#include <device/uart.h>
#include <options.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

int uart_read_handler(void * priv, bus_t * bus, uint32_t address, void * buffer, size_t bytes) {
	uart_device_t * uart = (uart_device_t *) priv;
	if (address < UART_MMIO_BASE || address >= UART_MMIO_END) {
		return 0;
	}

	uint32_t offset = address - UART_MMIO_BASE;
	memcpy(buffer, ((void*) &uart->mmio_buffer) + offset, 1);
	if (offset == (UART_RX_OFFSET + 3)) {
		uart->mmio_buffer.status |= UART_STAT_RX_FINISHED;
	}
	return bytes;
}

int uart_write_handler(void * priv, bus_t * bus, uint32_t address, void * buffer, size_t bytes) {
	uart_device_t * uart = (uart_device_t *) priv;
	if (address < UART_MMIO_BASE || address >= UART_MMIO_END) {
		return 0;
	}

	uint32_t offset = address - UART_MMIO_BASE;
	if (address >= UART_STAT_OFFSET || address <= (UART_STAT_OFFSET + 3)) {
		return 0;
	}

	memcpy(((void*) &uart->mmio_buffer) + offset, buffer, 1);
	if (offset == (UART_TX_OFFSET + 3)) {
		uart->mmio_buffer.status |= UART_STAT_TX_FINISHED;
	}
	return bytes;
}

void uart_clock(device_t * device) {
	uart_device_t * uart = (uart_device_t *) device;

	struct pollfd pd;
	pd.fd = uart->fifo;
	pd.events = POLLIN;
	
	int pstat = poll(&pd, 1, 0);
	if (pd.revents & POLLNVAL) {
		close(uart->fifo);
		uart->fifo = open(UART_FIFO_PATH, O_RDWR | O_NONBLOCK);
		if (uart->fifo < 0) {
			printf("UART - Could not re-open fifo\n");
		}
	}

	if ((uart->mmio_buffer.status & UART_STAT_RX_READY) != UART_STAT_RX_READY) {
		if ((pstat > 0) && (pd.revents & POLLIN)) {
			read(uart->fifo, ((void *) &uart->mmio_buffer.rx_data) + uart->offset, 1);
			uart->offset++;
			if (uart->offset == 4) {
				uart->mmio_buffer.status |= UART_STAT_RX_READY;
			}
		}
	} else {
		if (uart->mmio_buffer.status & UART_STAT_RX_FINISHED) {
			uart->offset = 0;
			uart->mmio_buffer.status ^= uart->mmio_buffer.status & UART_STAT_RX_READY;
			uart->mmio_buffer.status ^= UART_STAT_RX_FINISHED;
		}
	}
}

void uart_init(device_t * device) {
	uart_device_t * uart = (uart_device_t *) device;
	if (!uart->membus) {
		return; // erhm
	}

	mem_bus_attach_read(uart->membus, uart_read_handler, uart);
	mem_bus_attach_write(uart->membus, uart_write_handler, uart);
}

void uart_free(device_t * device) {
	uart_device_t * uart = (uart_device_t *) device;
	close(uart->fifo);
	free(device->ports);
	free(device);
}

int uart_devices_created = 0;
device_t * uart_create_device() {
	if (uart_devices_created++ == 1) {
		return NULL; // too many
	}

	uart_device_t * uart = malloc(sizeof(uart_device_t));
	uart->device.clock = uart_clock;
	uart->device.free = uart_free;
	uart->device.init = uart_init;
	uart->device.port_count = 1;
	uart->membus = NULL;

	memset(&uart->mmio_buffer, 0, sizeof(uart->mmio_buffer));
	uart->offset = 0;

	int r = mkfifo(UART_FIFO_PATH, 0664);
	if (r != 0) {
		return NULL;
	}

	uart->fifo = open(UART_FIFO_PATH, O_RDWR | O_NONBLOCK);
	if (uart->fifo < 0) {
		return NULL;
	}

	bus_port_t * port_list = malloc(sizeof(bus_port_t) * 1);
	port_list[0].type = BUS_MEMORY;
	port_list[0].bus = &uart->membus;
	uart->device.ports = port_list;

	return &uart->device;
}

device_interface_t DEVICE_INTERFACE uart_device_interface = {
	.name = "uart",
	.create = uart_create_device,
};