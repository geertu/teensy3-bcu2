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

#ifndef USBserial7_h_
#define USBserial7_h_

#include "usb_desc.h"

#if defined(MXU_SERIAL_INTERFACE) && MXU_SERIAL_NUM_PORTS > 6

#include <inttypes.h>

#if F_CPU >= 20000000

#include "core_pins.h" // for millis()
#include "mxu_serial_port.h"

// C language implementation
#ifdef __cplusplus
extern "C" {
#endif

static inline int usb_serial7_getchar(void)
{
	return __mxu_serial_getchar(&mxu_serial_ports[6]);
}

static inline int usb_serial7_peekchar(void)
{
	return __mxu_serial_peekchar(&mxu_serial_ports[6]);
}

static inline int usb_serial7_available(void)
{
	return __mxu_serial_available(&mxu_serial_ports[6]);
}

static inline int usb_serial7_read(void *buffer, uint32_t size)
{
	return __mxu_serial_read(&mxu_serial_ports[6], buffer, size);
}

static inline void usb_serial7_flush_input(void)
{
	__mxu_serial_flush_input(&mxu_serial_ports[6]);
}

static inline int usb_serial7_putchar(uint8_t c)
{
	return __mxu_serial_putchar(&mxu_serial_ports[6], c);
}

static inline int usb_serial7_write(const void *buffer, uint32_t size)
{
	return __mxu_serial_write(&mxu_serial_ports[6], buffer, size);
}

static inline int usb_serial7_write_buffer_free(void)
{
	return __mxu_serial_write_buffer_free(&mxu_serial_ports[6]);
}

static inline void usb_serial7_flush_output(void)
{
	__mxu_serial_flush_output(&mxu_serial_ports[6]);
}

static inline void usb_serial7_flush_callback(void)
{
	__mxu_serial_flush_callback(&mxu_serial_ports[6]);
}

// FIXME Yeah, bad name, but kept for compatibility with standard usb_serial
#define usb_cdc7_transmit_flush_timer	mxu_serial_ports[6].transmit_flush_timer

static inline uint32_t usb_serial7_get_baud(void)
{
	return 0;	// FIXME
}

#ifdef __cplusplus
}
#endif

// C++ interface
#ifdef __cplusplus
#include "Stream.h"
class usb_serial7_class : public Stream
{
public:
	constexpr usb_serial7_class() {}
        void begin(long) {
		//uint32_t millis_begin = systick_millis_count;
		//disabled for now - causes more trouble than it solves?
		//while (!(*this)) {
			// wait up to 2.5 seconds for Arduino Serial Monitor
			// Yes, this is a long time, but some Windows systems open
			// the port very slowly.  This wait allows programs for
			// Arduino Uno to "just work" (without forcing a reboot when
			// the port is opened), and when no PC is connected the user's
			// sketch still gets to run normally after this wait time.
			//if ((uint32_t)(systick_millis_count - millis_begin) > 2500) break;
		//}
	}
        void end() { /* TODO: flush output and shut down USB port */ };
        virtual int available() { return usb_serial7_available(); }
        virtual int read() { return usb_serial7_getchar(); }
        virtual int peek() { return usb_serial7_peekchar(); }
        virtual void flush() { usb_serial7_flush_output(); }  // TODO: actually wait for data to leave USB...
        virtual void clear(void) { usb_serial7_flush_input(); }
        virtual size_t write(uint8_t c) { return usb_serial7_putchar(c); }
        virtual size_t write(const uint8_t *buffer, size_t size) { return usb_serial7_write(buffer, size); }
	size_t write(unsigned long n) { return write((uint8_t)n); }
	size_t write(long n) { return write((uint8_t)n); }
	size_t write(unsigned int n) { return write((uint8_t)n); }
	size_t write(int n) { return write((uint8_t)n); }
	virtual int availableForWrite() { return usb_serial7_write_buffer_free(); }
	using Print::write;
        void send_now(void) { usb_serial7_flush_output(); }
        uint32_t baud(void) { return usb_serial7_get_baud(); }
        uint8_t stopbits(void) { return 0; /* FIXME */ }
        uint8_t paritytype(void) { return 0; /* FIXME */ }
        uint8_t numbits(void) { return 0; /* FIXME */ }
        uint8_t dtr(void) { return 0; /* FIXME */ }
        uint8_t rts(void) { return 0; /* FIXME */ }
        operator bool() {
		yield();
		return true;	// FIXME
	}
	size_t readBytes(char *buffer, size_t length) {
		size_t count=0;
		unsigned long startMillis = millis();
		do {
			count += usb_serial7_read(buffer + count, length - count);
			if (count >= length) return count;
		} while(millis() - startMillis < _timeout);
		setReadError();
		return count;
	}

};
extern usb_serial7_class SerialUSB6;
extern void serialEventUSB6(void);
#endif // __cplusplus

#endif // F_CPU

#endif // MXU_SERIAL_INTERFACE && MXU_SERIAL_NUM_PORTS > 6

#endif // USBserial7_h_
