//
// USB and Serial Event Wrapper
//
// © Copyright 2019-2020, 2022 Glider bv
//
// This file is subject to the terms and conditions of the GNU General Public
// License.
//

#include <usb_desc.h>

#include "event.h"

void serialEvent(void) { return usb_serial_event(); }
#if defined(USB_DUAL_SERIAL) || defined(USB_TRIPLE_SERIAL) || (defined (USB_MXU_SERIAL) && MXU_SERIAL_NUM_PORTS > 1)
void serialEventUSB1(void) { return usb_serial2_event(); }
#endif
#if defined(USB_TRIPLE_SERIAL) || (defined (USB_MXU_SERIAL) && MXU_SERIAL_NUM_PORTS > 2)
void serialEventUSB2(void) { return usb_serial3_event(); }
#endif
void serialEvent1(void) { return serial_event(); }
void serialEvent2(void) { return serial2_event(); }
