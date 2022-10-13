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

#include <Arduino.h>
#include "usb_desc.h"

#if F_CPU >= 20000000

#ifdef CDC_DATA_INTERFACE
#ifdef CDC_STATUS_INTERFACE
usb_serial_class Serial;
#endif
#endif

#ifdef CDC2_DATA_INTERFACE
#ifdef CDC2_STATUS_INTERFACE
usb_serial2_class SerialUSB1;
#endif
#endif

#ifdef CDC3_DATA_INTERFACE
#ifdef CDC3_STATUS_INTERFACE
usb_serial3_class SerialUSB2;
#endif
#endif

#ifdef MXU_SERIAL_INTERFACE
#if MXU_SERIAL_NUM_PORTS > 1
usb_serial2_class SerialUSB1;
#endif
#if MXU_SERIAL_NUM_PORTS > 2
usb_serial3_class SerialUSB2;
#endif
#if MXU_SERIAL_NUM_PORTS > 3
usb_serial4_class SerialUSB3;
#endif
#if MXU_SERIAL_NUM_PORTS > 4
usb_serial5_class SerialUSB4;
#endif
#if MXU_SERIAL_NUM_PORTS > 5
usb_serial6_class SerialUSB5;
#endif
#if MXU_SERIAL_NUM_PORTS > 6
usb_serial7_class SerialUSB6;
#endif
#if MXU_SERIAL_NUM_PORTS > 7
usb_serial8_class SerialUSB7;
#endif
#endif // MXU_SERIAL_INTERFACE

#ifdef MIDI_INTERFACE
usb_midi_class usbMIDI;
#endif

#ifdef KEYBOARD_INTERFACE
usb_keyboard_class Keyboard;
#endif

#ifdef MOUSE_INTERFACE
usb_mouse_class Mouse;
#endif

#ifdef RAWHID_INTERFACE
usb_rawhid_class RawHID;
#endif

#ifdef FLIGHTSIM_INTERFACE
FlightSimClass FlightSim;
#endif

#ifdef SEREMU_INTERFACE
usb_seremu_class Serial;
#endif

#ifdef JOYSTICK_INTERFACE
usb_joystick_class Joystick;
uint8_t usb_joystick_class::manual_mode = 0;
#endif

#ifdef USB_DISABLED
usb_serial_class Serial;
#endif


#else // F_CPU < 20 MHz

#if defined(USB_SERIAL) || defined(USB_DUAL_SERIAL) || \
    defined(USB_TRIPLE_SERIAL) || defined(USB_SERIAL_HID) || \
    defined(USB_MXU_SERIAL)
usb_serial_class Serial;
#elif (USB_DISABLED)
usb_serial_class Serial;
#else
usb_seremu_class Serial;
#endif

#endif // F_CPU

void serialEvent() __attribute__((weak));
void serialEvent() {}
void serialEventUSB1() __attribute__((weak));
void serialEventUSB1() {}
void serialEventUSB2() __attribute__((weak));
void serialEventUSB2() {}
void serialEventUSB3() __attribute__((weak));
void serialEventUSB3() {}
void serialEventUSB4() __attribute__((weak));
void serialEventUSB4() {}
void serialEventUSB5() __attribute__((weak));
void serialEventUSB5() {}
void serialEventUSB6() __attribute__((weak));
void serialEventUSB6() {}
void serialEventUSB7() __attribute__((weak));
void serialEventUSB7() {}
