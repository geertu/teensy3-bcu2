/* Teensyduino Core Library
 * http://www.pjrc.com/teensy/
 * Copyright (c) 2017 PJRC.COM, LLC.
 * Copyright (c) 2022 Glider bv
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * 1. The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * 2. If the Software is incorporated into a build system that allows
 * selection among a list of target devices, then similar target
 * devices manufactured by PJRC.COM must be included in the list of
 * target devices and selectable in the same manner.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "usb_dev.h"
#include "core_pins.h" // for yield()
//#include "HardwareSerial.h"
#include <string.h> // for memcpy()

// defined by usb_dev.h -> usb_desc.h
#ifdef MXU_SERIAL_INTERFACE
#if F_CPU >= 20000000

#include "mxu_serial_port.h"

// FIXME debug start
#include "HardwareSerial.h"
extern __attribute__((__format__(printf, 1, 2))) int serial_printf(const char *fmt, ...);
#define printf			serial_printf
#define ESC_NORMAL		"\e[0m"
#define ESC_BLACK		"\e[31m"
#define ESC_RED			"\e[31m"
#define ESC_GREEN		"\e[32m"
#define ESC_YELLOW		"\e[33m"
#define ESC_BLUE		"\e[34m"
#define ESC_MAGENTA		"\e[35m"
#define ESC_CYAN		"\e[36m"
#define ESC_WHITE		"\e[37m"
#define pr_info(fmt, ...)	printf(ESC_GREEN fmt ESC_NORMAL, ##__VA_ARGS__)
#define pr_warn(fmt, ...)	printf(ESC_YELLOW fmt ESC_NORMAL, ##__VA_ARGS__)
#define pr_err(fmt, ...)	printf(ESC_RED fmt ESC_NORMAL, ##__VA_ARGS__)
static void pr_hex(const char *label, const void *p, size_t len)
{
	const uint8_t *data = p;

	if (!len)
		return;

	printf(ESC_BLUE "%s:", label);
	while (len--)
		printf(" %02x", *data++);
	printf(ESC_NORMAL "\n");
}
// FIXME debug end

#define TRANSMIT_FLUSH_TIMEOUT	5	/* in milliseconds */

#define MXU_HDR_SIZE		4

struct mxu_serial_port mxu_serial_ports[MXU_SERIAL_NUM_PORTS] = {
	{
		// FIXME
	},
#if MXU_SERIAL_NUM_PORTS > 1
	{
		// FIXME
	},
#endif
#if MXU_SERIAL_NUM_PORTS > 2
	{
		// FIXME
	},
#endif
#if MXU_SERIAL_NUM_PORTS > 3
	{
		// FIXME
	},
#endif
#if MXU_SERIAL_NUM_PORTS > 4
	{
		// FIXME
	},
#endif
#if MXU_SERIAL_NUM_PORTS > 5
	{
		// FIXME
	},
#endif
#if MXU_SERIAL_NUM_PORTS > 6
	{
		// FIXME
	},
#endif
#if MXU_SERIAL_NUM_PORTS > 7
	{
		// FIXME
	},
#endif
};

static struct usb_packet_struct *mxu_usb_rx(struct mxu_serial_port *port)
{
	struct usb_packet_struct *rx = usb_rx(MXU_RX_ENDPOINT);
	unsigned int idx = (port - mxu_serial_ports);
	unsigned int pidx, plen;

	if (!rx)
		return NULL;

pr_info("%s: port %u packet len %u index %u\n", __func__, idx, rx->len, rx->index);

	if (rx->len < MXU_HDR_SIZE) {
		pr_err("%s:%u: Short packet len %u\n", __func__, __LINE__, rx->len);
		usb_free(rx);
		return NULL;
	}

	pidx = rx->buf[0] << 8 | rx->buf[1];
	plen = rx->buf[2] << 8 | rx->buf[3];

if (pidx != idx) pr_warn("%s:%u: idx/pidx mismatch %u != %u\n", __func__, __LINE__, idx, pidx); // FIXME
	if (rx->len < MXU_HDR_SIZE + plen) {
		pr_err("%s:%u: Short packet len %u\n", __func__, __LINE__, rx->len);
		usb_free(rx);
		return NULL;

	}

	// While the Linux driver supports receiving multiple segments in a
	// single packet, it never sends packets containing more than one
	// segment.
	// Hence we keep the code simple by assuming there is only one segment.
	if (rx->len > MXU_HDR_SIZE + plen)
		pr_warn("%s:%u: Ignoring secondary segments\n", __func__, __LINE__);

	// Strip the header
	rx->index = MXU_HDR_SIZE;
	rx->len = MXU_HDR_SIZE + plen;

pr_info("%s: %u bytes for port %u\n", __func__, plen, pidx);
pr_hex("rx", rx->buf + MXU_HDR_SIZE, plen);

	return rx;
}

static void mxu_usb_tx(struct usb_packet_struct *tx)
{
	// Complete header
	tx->buf[2] = 0;
	tx->buf[3] = tx->len - MXU_HDR_SIZE;

pr_info("%s: port %u packet len %u\n", __func__, tx->buf[0] << 8 | tx->buf[1], tx->len);
pr_hex("tx", tx->buf, tx->len);
	usb_tx(MXU_TX_ENDPOINT, tx);
}

// get the next character, or -1 if nothing received
int __mxu_serial_getchar(struct mxu_serial_port *port)
{
	unsigned int i;
	int c;

	if (!port->rx_packet) {
		if (!usb_configuration)
			return -1;
		port->rx_packet = mxu_usb_rx(port);
		if (!port->rx_packet)
			return -1;
	}
	i = port->rx_packet->index;
	c = port->rx_packet->buf[i++];
	if (i >= port->rx_packet->len) {
		usb_free(port->rx_packet);
		port->rx_packet = NULL;
	} else {
		port->rx_packet->index = i;
	}
	return c;
}

// FIXME send events

// peek at the next character, or -1 if nothing received
int __mxu_serial_peekchar(struct mxu_serial_port *port)
{
	if (!port->rx_packet) {
		if (!usb_configuration)
			return -1;
		port->rx_packet = mxu_usb_rx(port);
		if (!port->rx_packet)
			return -1;
	}
	if (!port->rx_packet)
		return -1;
	return port->rx_packet->buf[port->rx_packet->index];
}

// number of bytes available in the receive buffer
int __mxu_serial_available(struct mxu_serial_port *port)
{
	int count;
	// FIXME Count the actual number of bytes destined for this port
	count = usb_rx_byte_count(MXU_RX_ENDPOINT);
	if (port->rx_packet)
		count += port->rx_packet->len - port->rx_packet->index;
	if (count == 0)
		yield();
	return count;
}

// read a block of bytes to a buffer
int __mxu_serial_read(struct mxu_serial_port *port, void *buffer,
		      uint32_t size)
{
	uint8_t *p = (uint8_t *)buffer;
	uint32_t qty, count=0;

	while (size) {
		if (!usb_configuration)
			break;
		if (!port->rx_packet) {
			rx:
			port->rx_packet = mxu_usb_rx(port);
			if (!port->rx_packet)
				break;
			if (port->rx_packet->len == 0) {
				usb_free(port->rx_packet);
				goto rx;
			}
		}
		qty = port->rx_packet->len - port->rx_packet->index;
		if (qty > size)
			qty = size;
		memcpy(p, port->rx_packet->buf + port->rx_packet->index, qty);
		p += qty;
		count += qty;
		size -= qty;
		port->rx_packet->index += qty;
		if (port->rx_packet->index >= port->rx_packet->len) {
			usb_free(port->rx_packet);
			port->rx_packet = NULL;
		}
	}
	return count;
}

// discard any buffered input
void __mxu_serial_flush_input(struct mxu_serial_port *port)
{
	usb_packet_t *rx;

	if (!usb_configuration)
		return;
	if (port->rx_packet) {
		usb_free(port->rx_packet);
		port->rx_packet = NULL;
	}
	while (1) {
		rx = mxu_usb_rx(port);
		if (!rx)
			break;
		usb_free(rx);
	}
}

// Maximum number of transmit packets to queue so we don't starve other endpoints for memory
#define TX_PACKET_LIMIT 8

// When the PC isn't listening, how long do we wait before discarding data?  If this is
// too short, we risk losing data during the stalls that are common with ordinary desktop
// software.  If it's too long, we stall the user's program when no software is running.
#define TX_TIMEOUT_MSEC 70
#if F_CPU == 256000000
  #define TX_TIMEOUT (TX_TIMEOUT_MSEC * 1706)
#elif F_CPU == 240000000
  #define TX_TIMEOUT (TX_TIMEOUT_MSEC * 1600)
#elif F_CPU == 216000000
  #define TX_TIMEOUT (TX_TIMEOUT_MSEC * 1440)
#elif F_CPU == 192000000
  #define TX_TIMEOUT (TX_TIMEOUT_MSEC * 1280)
#elif F_CPU == 180000000
  #define TX_TIMEOUT (TX_TIMEOUT_MSEC * 1200)
#elif F_CPU == 168000000
  #define TX_TIMEOUT (TX_TIMEOUT_MSEC * 1100)
#elif F_CPU == 144000000
  #define TX_TIMEOUT (TX_TIMEOUT_MSEC * 932)
#elif F_CPU == 120000000
  #define TX_TIMEOUT (TX_TIMEOUT_MSEC * 764)
#elif F_CPU == 96000000
  #define TX_TIMEOUT (TX_TIMEOUT_MSEC * 596)
#elif F_CPU == 72000000
  #define TX_TIMEOUT (TX_TIMEOUT_MSEC * 512)
#elif F_CPU == 48000000
  #define TX_TIMEOUT (TX_TIMEOUT_MSEC * 428)
#elif F_CPU == 24000000
  #define TX_TIMEOUT (TX_TIMEOUT_MSEC * 262)
#endif

// When we've suffered the transmit timeout, don't wait again until the computer
// begins accepting data.  If no software is running to receive, we'll just discard
// data as rapidly as Serial.print() can generate it, until there's something to
// actually receive it.
static uint8_t transmit_previous_timeout = 0;


// transmit a character.  0 returned on success, -1 on error
int __mxu_serial_putchar(struct mxu_serial_port *port, uint8_t c)
{
	return __mxu_serial_write(port, &c, 1);
}


int __mxu_serial_write(struct mxu_serial_port *port, const void *buffer,
		       uint32_t size)
{
	uint32_t ret = size;
	uint32_t len;
	uint32_t wait_count;
	const uint8_t *src = (const uint8_t *)buffer;
	uint8_t *dest;

	port->tx_noautoflush = 1;
	while (size > 0) {
		if (!port->tx_packet) {
			wait_count = 0;
			while (1) {
				if (!usb_configuration) {
					port->tx_noautoflush = 0;
					return -1;
				}
				if (usb_tx_packet_count(MXU_TX_ENDPOINT) < TX_PACKET_LIMIT) {
					port->tx_noautoflush = 1;
					port->tx_packet = usb_malloc();
					if (port->tx_packet) {
						// Add first part of header
						port->tx_packet->buf[0] = 0;
						port->tx_packet->buf[1] = port - mxu_serial_ports;
						port->tx_packet->index = MXU_HDR_SIZE;
						port->tx_packet->len = MXU_HDR_SIZE;
						break;
					}
					port->tx_noautoflush = 0;
				}
				if (++wait_count > TX_TIMEOUT || transmit_previous_timeout) {
					transmit_previous_timeout = 1;
					return -1;
				}
				yield();
			}
		}
		transmit_previous_timeout = 0;
		len = MXU_TX_SIZE - port->tx_packet->index;
		if (len > size)
			len = size;
		dest = port->tx_packet->buf + port->tx_packet->index;
		port->tx_packet->index += len;
		size -= len;
		while (len-- > 0)
			*dest++ = *src++;
		if (port->tx_packet->index >= MXU_TX_SIZE) {
			port->tx_packet->len = MXU_TX_SIZE;
			mxu_usb_tx(port->tx_packet);
			port->tx_packet = NULL;
		}
		port->transmit_flush_timer = TRANSMIT_FLUSH_TIMEOUT;
	}
	port->tx_noautoflush = 0;
	return ret;
}

int __mxu_serial_write_buffer_free(struct mxu_serial_port *port)
{
	uint32_t len;

	port->tx_noautoflush = 1;
	if (!port->tx_packet) {
		if (!usb_configuration ||
		  usb_tx_packet_count(MXU_TX_ENDPOINT) >= TX_PACKET_LIMIT ||
		  (port->tx_packet = usb_malloc()) == NULL) {
			port->tx_noautoflush = 0;
			return 0;
		}
		// Add first part of header
		port->tx_packet->buf[0] = 0;
		port->tx_packet->buf[1] = port - mxu_serial_ports;
		port->tx_packet->index = MXU_HDR_SIZE;
		port->tx_packet->len = MXU_HDR_SIZE;
	}
	len = MXU_TX_SIZE - port->tx_packet->index;
	// TODO: Perhaps we need "port->transmit_flush_timer = TRANSMIT_FLUSH_TIMEOUT"
	// added here, so the SOF interrupt can't take away the available buffer
	// space we just promised the user could write without blocking?
	// But does this come with other performance downsides?  Could it lead to
	// buffer data never actually transmitting in some usage cases?  More
	// investigation is needed.
	// https://github.com/PaulStoffregen/cores/issues/10#issuecomment-61514955
	port->tx_noautoflush = 0;
	return len;
}

void __mxu_serial_flush_output(struct mxu_serial_port *port)
{
	if (!usb_configuration)
		return;
	port->tx_noautoflush = 1;
	if (port->tx_packet) {
		port->transmit_flush_timer = 0;
		port->tx_packet->len = port->tx_packet->index;
		mxu_usb_tx(port->tx_packet);
		port->tx_packet = NULL;
	} else {
		usb_packet_t *tx = usb_malloc();
		if (tx) {
			port->transmit_flush_timer = 0;
			// FIXME Use mxu-specific flush
			usb_tx(MXU_TX_ENDPOINT, tx);
		} else {
			port->transmit_flush_timer = 1;
		}
	}
	port->tx_noautoflush = 0;
}

void __mxu_serial_flush_callback(struct mxu_serial_port *port)
{
	if (port->tx_noautoflush)
		return;
	if (port->tx_packet) {
		port->tx_packet->len = port->tx_packet->index;
		mxu_usb_tx(port->tx_packet);
		port->tx_packet = NULL;
	} else {
		usb_packet_t *tx = usb_malloc();
		if (tx) {
			// FIXME Use mxu-specific flush
			usb_tx(MXU_TX_ENDPOINT, tx);
		} else {
			port->transmit_flush_timer = 1;
		}
	}
}

#endif // F_CPU
#endif // MXU_SERIAL_INTERFACE
