/* Teensyduino Core Library
 * http://www.pjrc.com/teensy/
 * Copyright (c) 2017 PJRC.COM, LLC.
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

#ifndef usb_serial_port_h_
#define usb_serial_port_h_

#if F_CPU >= 20000000 && !defined(USB_DISABLED)

#define USB_SERIAL_DTR  0x01
#define USB_SERIAL_RTS  0x02

// C language implementation
#ifdef __cplusplus
extern "C" {
#endif
struct usb_packet_struct;
struct usb_serial_port {
	/* public */
	uint32_t cdc_line_coding[2];
	volatile uint32_t cdc_line_rtsdtr_millis;
	volatile uint8_t cdc_line_rtsdtr;
	volatile uint8_t cdc_transmit_flush_timer;

	/* private */
	struct usb_packet_struct *rx_packet;
	struct usb_packet_struct *tx_packet;
	volatile uint8_t tx_noautoflush;
	const uint8_t cdc_rx_endpoint;
	const uint8_t cdc_tx_endpoint;
	const uint8_t cdc_tx_size;
};

extern struct usb_serial_port usb_serial_ports[];

extern volatile uint32_t systick_millis_count;
extern volatile uint8_t usb_configuration;

int __usb_serial_getchar(struct usb_serial_port *port);
int __usb_serial_peekchar(struct usb_serial_port *port);
int __usb_serial_available(struct usb_serial_port *port);
int __usb_serial_read(struct usb_serial_port *port, void *buffer,
		      uint32_t size);
void __usb_serial_flush_input(struct usb_serial_port *port);
int __usb_serial_putchar(struct usb_serial_port *port, uint8_t c);
int __usb_serial_write(struct usb_serial_port *port, const void *buffer,
		       uint32_t size);
int __usb_serial_write_buffer_free(struct usb_serial_port *port);
void __usb_serial_flush_output(struct usb_serial_port *port);
void __usb_serial_flush_callback(struct usb_serial_port *port);
#ifdef __cplusplus
}
#endif

#endif // F_CPU

#endif // usb_serial_port_h_
